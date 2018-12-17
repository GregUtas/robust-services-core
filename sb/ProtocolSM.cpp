//==============================================================================
//
//  ProtocolSM.cpp
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "ProtocolSM.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "Event.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "GlobalAddress.h"
#include "LocalAddress.h"
#include "Message.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "RootServiceSM.h"
#include "SbAppIds.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "Signal.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "Timer.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name ProtocolSM_ctor1 = "ProtocolSM.ctor(first)";

ProtocolSM::ProtocolSM(FactoryId fid) :
   fid_(fid),
   state_(Idle)
{
   Debug::ft(ProtocolSM_ctor1);

   Initialize(false);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_ctor2 = "ProtocolSM.ctor(subseq)";

ProtocolSM::ProtocolSM(FactoryId fid, ProtocolLayer& adj, bool upper) :
   ProtocolLayer(adj, upper),
   fid_(fid),
   state_(Idle)
{
   Debug::ft(ProtocolSM_ctor2);

   //  If the layer above created this PSM, queue it after others of equal
   //  priority.  This ensures that the PSM queue will be ordered from the
   //  uppermost to lowermost PSM, which is critical for proper processing
   //  of outgoing messages.
   //
   Initialize(!upper);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_dtor = "ProtocolSM.dtor";

ProtocolSM::~ProtocolSM()
{
   Debug::ft(ProtocolSM_dtor);

   //  Record the PSM's deletion if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = Clock::TicksNow();

      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         new PsmTrace(PsmTrace::Deletion, *this);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }

   //  Purge any items in the message queues or timer queue.
   //
   rcvdMsgq_.Purge();
   ogMsgq_.Purge();
   sentMsgq_.Purge();
   timerq_.Purge();

   //  Remove the PSM from the context's PSM queue and inform the root SSM
   //  of this PSM's deletion.
   //
   auto ctx = GetContext();

   if(ctx != nullptr)
   {
      ctx->ExqPsm(*this);

      auto ssm = ctx->RootSsm();
      if(ssm != nullptr) ssm->PsmDeleted(*this);
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_AddressesUnknown = "ProtocolSM.AddressesUnknown";

bool ProtocolSM::AddressesUnknown(const Message* msg) const
{
   Debug::ft(ProtocolSM_AddressesUnknown);

   if((msg != nullptr) && (msg->RxSbAddr().fid != NIL_ID)) return false;

   auto port = Port();
   if(port == nullptr) return true;
   return (!port->HasRcvdMsg() && !port->HasSentMsg());
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_AllocLower = "ProtocolSM.AllocLower";

ProtocolLayer* ProtocolSM::AllocLower(const Message* msg)
{
   Debug::ft(ProtocolSM_AllocLower);

   return new MsgPort(*this);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_Cleanup = "ProtocolSM.Cleanup";

void ProtocolSM::Cleanup()
{
   Debug::ft(ProtocolSM_Cleanup);

   SendFinal();
   ProtocolLayer::Cleanup();
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_Destroy = "ProtocolSM.Destroy";

void ProtocolSM::Destroy()
{
   Debug::ft(ProtocolSM_Destroy);

   SendFinal();
   delete this;
}

//------------------------------------------------------------------------------

void ProtocolSM::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ProtocolLayer::Display(stream, prefix, options);

   stream << prefix << "rcvdMsgq : " << CRLF;
   rcvdMsgq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "ogMsgq   : " << CRLF;
   ogMsgq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "sentMsgq : " << CRLF;
   sentMsgq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "timerq   : " << CRLF;
   timerq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "fid      : " << fid_ << CRLF;
   stream << prefix << "state    : " << int(state_) << CRLF;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_DropPeer = "ProtocolSM.DropPeer";

bool ProtocolSM::DropPeer(const GlobalAddress& peerPrevRemAddr)
{
   Debug::ft(ProtocolSM_DropPeer);

   return EnsurePort()->DropPeer(peerPrevRemAddr);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_EndOfTransaction = "ProtocolSM.EndOfTransaction";

void ProtocolSM::EndOfTransaction()
{
   Debug::ft(ProtocolSM_EndOfTransaction);

   //  Prepare and send outgoing messages.
   //
   PrepareOgMsgq();

   for(auto m = ogMsgq_.First(); m != nullptr; m = ogMsgq_.First())
   {
      auto rc = ProcessOgMsg(*m);

      switch(rc)
      {
      case SendMessage:
         SendToLower(*m);
         break;

      case PurgeMessage:
         delete m;
         break;

      case SkipMessage:
         if(ogMsgq_.First() == m)
         {
            //  The message was not deleted or moved as claimed.  Delete it
            //  to prevent what will probably end up being an infinite loop.
            //
            Debug::SwLog(ProtocolSM_EndOfTransaction,
               pack2(fid_, state_), m->GetSignal());
            delete m;
         }
         break;

      default:
         Context::Kill(ProtocolSM_EndOfTransaction, fid_, rc);
         return;
      }
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_EnqOgMsg = "ProtocolSM.EnqOgMsg";

void ProtocolSM::EnqOgMsg(Message& msg)
{
   Debug::ft(ProtocolSM_EnqOgMsg);

   msg.Enqueue(ogMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_FindTimer = "ProtocolSM.FindTimer";

Timer* ProtocolSM::FindTimer(const Base& owner, TimerId tid) const
{
   Debug::ft(ProtocolSM_FindTimer);

   //  Search for a timer that is owned by OWNER and identified by TID.
   //
   for(auto t = timerq_.First(); t != nullptr; timerq_.Next(t))
   {
      if((t->Tid() == tid) && (t->Owner() == &owner)) return t;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_GetProtocol = "ProtocolSM.GetProtocol";

ProtocolId ProtocolSM::GetProtocol() const
{
   Debug::ft(ProtocolSM_GetProtocol);

   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(fid_);

   if(fac != nullptr) return fac->GetProtocol();

   Debug::SwLog(ProtocolSM_GetProtocol, fid_, 0);
   return NIL_ID;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_GetSubtended = "ProtocolSM.GetSubtended";

void ProtocolSM::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(ProtocolSM_GetSubtended);

   ProtocolLayer::GetSubtended(objects, count);

   for(auto m = rcvdMsgq_.First(); m != nullptr; rcvdMsgq_.Next(m))
   {
      m->GetSubtended(objects, count);
   }

   for(auto m = ogMsgq_.First(); m != nullptr; ogMsgq_.Next(m))
   {
      m->GetSubtended(objects, count);
   }

   for(auto m = sentMsgq_.First(); m != nullptr; sentMsgq_.Next(m))
   {
      m->GetSubtended(objects, count);
   }

   for(auto t = timerq_.First(); t != nullptr; timerq_.Next(t))
   {
      t->GetSubtended(objects, count);
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_HenqOgMsg = "ProtocolSM.HenqOgMsg";

void ProtocolSM::HenqOgMsg(Message& msg)
{
   Debug::ft(ProtocolSM_HenqOgMsg);

   msg.Henqueue(ogMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_HenqReceivedMsg = "ProtocolSM.HenqReceivedMsg";

void ProtocolSM::HenqReceivedMsg(Message& msg)
{
   Debug::ft(ProtocolSM_HenqReceivedMsg);

   msg.Henqueue(rcvdMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_HenqSentMsg = "ProtocolSM.HenqSentMsg";

void ProtocolSM::HenqSentMsg(Message& msg)
{
   Debug::ft(ProtocolSM_HenqSentMsg);

   msg.Henqueue(sentMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_Initialize = "ProtocolSM.Initialize";

void ProtocolSM::Initialize(bool henq)
{
   Debug::ft(ProtocolSM_Initialize);

   rcvdMsgq_.Init(Pooled::LinkDiff());
   ogMsgq_.Init(Pooled::LinkDiff());
   sentMsgq_.Init(Pooled::LinkDiff());
   timerq_.Init(Pooled::LinkDiff());

   auto ctx = GetContext();

   if(henq)
      ctx->HenqPsm(*this);
   else
      ctx->EnqPsm(*this);

   //  Record the PSM's creation if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(ctx->TraceOn(trans))
   {
      auto warp = Clock::TicksNow();

      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         new PsmTrace(PsmTrace::Creation, *this);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_InjectFinalMsg = "ProtocolSM.InjectFinalMsg";

void ProtocolSM::InjectFinalMsg()
{
   Debug::ft(ProtocolSM_InjectFinalMsg);

   //  This is inelegant.  A PSM that can communicate with another node
   //  should therefore override it to inject a session takedown message.
   //
   Kill();
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_JoinPeer = "ProtocolSM.JoinPeer";

ProtocolLayer* ProtocolSM::JoinPeer
   (const LocalAddress& peer, GlobalAddress& peerPrevRemAddr)
{
   Debug::ft(ProtocolSM_JoinPeer);

   auto port = EnsurePort()->JoinPeer(peer, peerPrevRemAddr);
   if(port == nullptr) return nullptr;
   auto prid = GetProtocol();

   for(auto layer = port->Upper(); layer != nullptr; layer = layer->Upper())
   {
      auto psm = static_cast< ProtocolSM* >(layer);
      if(psm->GetProtocol() == prid) return psm;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_Kill = "ProtocolSM.Kill";

void ProtocolSM::Kill()
{
   Debug::ft(ProtocolSM_Kill);

   //  Queue a timeout message on the PSM and set its kill flag.
   //
   auto msg = new Message(this, 0);
   if(msg == nullptr) return;

   msg->SetProtocol(TimerProtocolId);
   msg->SetSignal(Signal::Timeout);
   msg->SetPriority(Message::Progress);
   msg->Header()->kill = true;

   if(!msg->SendToSelf())
   {
      Debug::SwLog(ProtocolSM_Kill, fid_, 0);
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_new = "ProtocolSM.operator new";

void* ProtocolSM::operator new(size_t size)
{
   Debug::ft(ProtocolSM_new);

   return Singleton< ProtocolSMPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void ProtocolSM::Patch(sel_t selector, void* arguments)
{
   ProtocolLayer::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_PeerFactory = "ProtocolSM.PeerFactory";

FactoryId ProtocolSM::PeerFactory() const
{
   Debug::ft(ProtocolSM_PeerFactory);

   auto port = Port();
   if(port == nullptr) return NIL_ID;
   return port->RemAddr().Fid();
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_Port = "ProtocolSM.Port";

MsgPort* ProtocolSM::Port() const
{
   Debug::ft(ProtocolSM_Port);

   auto lower = Lower();
   if(lower == nullptr) return nullptr;
   return lower->Port();
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_ProcessIcMsg = "ProtocolSM.ProcessIcMsg";

ProtocolSM::IncomingRc ProtocolSM::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft(ProtocolSM_ProcessIcMsg);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolSM_ProcessIcMsg, fid_, 0);
   return DiscardMessage;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_ProcessOgMsg = "ProtocolSM.ProcessOgMsg";

ProtocolSM::OutgoingRc ProtocolSM::ProcessOgMsg(Message& msg)
{
   Debug::ft(ProtocolSM_ProcessOgMsg);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolSM_ProcessOgMsg, fid_, 0);
   return PurgeMessage;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_ReceiveMsg = "ProtocolSM.ReceiveMsg";

Event* ProtocolSM::ReceiveMsg(Message& msg)
{
   Debug::ft(ProtocolSM_ReceiveMsg);

   HenqReceivedMsg(msg);

   Event* evt = nullptr;

   auto rc = ProcessIcMsg(msg, evt);

   switch(rc)
   {
   case EventRaised:
      if(evt == nullptr)
         Debug::SwLog(ProtocolSM_ReceiveMsg, fid_, rc);
      break;
   case DiscardMessage:
      if(evt != nullptr)
         Debug::SwLog(ProtocolSM_ReceiveMsg, fid_, rc);
      break;
   case ReceiveMessage:
      if(evt != nullptr)
         Debug::SwLog(ProtocolSM_ReceiveMsg, fid_, rc);
      return SendToUpper(msg);
   }

   return evt;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_SendFinal = "ProtocolSM.SendFinal";

void ProtocolSM::SendFinal()
{
   Debug::ft(ProtocolSM_SendFinal);

   if((state_ != Idle) && (Port() != nullptr))
   {
      SendFinalMsg();
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_SendFinalMsg = "ProtocolSM.SendFinalMsg";

void ProtocolSM::SendFinalMsg()
{
   Debug::ft(ProtocolSM_SendFinalMsg);

   //  This is a pure virtual function.
   //
   Debug::SwLog(ProtocolSM_SendFinalMsg, fid_, 0);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_SendMsg = "ProtocolSM.SendMsg";

bool ProtocolSM::SendMsg(Message& msg)
{
   Debug::ft(ProtocolSM_SendMsg);

   //  Queue the message on our outgoing message queue.  It will not be sent
   //  to the layer below until our ProcessOgMsgq and ProcessOgMsg functions
   //  have been invoked.  This allows more than one message to be queued so
   //  that they can be bundled.  To make this work, the layers in a protocol
   //  stack must be queued (on the PSM queue) in uppermost to lowermost order
   //  so that EndOfTransaction's loop encounters a lower layer after it has
   //  queued outgoing messages on itself.
   //
   EnqOgMsg(msg);
   return true;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_SetState = "ProtocolSM.SetState";

void ProtocolSM::SetState(StateId stid)
{
   Debug::ft(ProtocolSM_SetState);

   state_ = stid;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_StartTimer = "ProtocolSM.StartTimer";

bool ProtocolSM::StartTimer
   (secs_t duration, Base& owner, TimerId tid, bool repeat)
{
   Debug::ft(ProtocolSM_StartTimer);

   //  Prevent a duplicate timer from being started.
   //
   if(FindTimer(owner, tid) != nullptr)
   {
      Debug::SwLog(ProtocolSM_StartTimer, fid_, tid);
      return false;
   }

   auto timer = new Timer(*this, owner, tid, duration, repeat);

   return (timer != nullptr);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_StopTimer = "ProtocolSM.StopTimer";

void ProtocolSM::StopTimer(const Base& owner, TimerId tid)
{
   Debug::ft(ProtocolSM_StopTimer);

   //  Search for a timer that is owned by OWNER and identified by TID.
   //  Stop the timer if it is found.
   //
   auto timer = FindTimer(owner, tid);

   if(timer != nullptr)
   {
      delete timer;
      return;
   }

   //  The timer wasn't found.  There is a possibility that it expired and
   //  that a timeout message is sitting in our context's message queue.
   //  Delete such a message if it exists.
   //
   auto ctx = Context::RunningContext();

   if(ctx != nullptr) ctx->StopTimer(owner, tid);
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_UnwrapMsg = "ProtocolSM.UnwrapMsg";

Message* ProtocolSM::UnwrapMsg(Message& msg)
{
   Debug::ft(ProtocolSM_UnwrapMsg);

   return &msg;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_UppermostPsm = "ProtocolSM.UppermostPsm";

ProtocolSM* ProtocolSM::UppermostPsm() const
{
   Debug::ft(ProtocolSM_UppermostPsm);

   auto upper = Upper();
   if(upper == nullptr) return const_cast< ProtocolSM* >(this);
   return upper->UppermostPsm();
}
}
