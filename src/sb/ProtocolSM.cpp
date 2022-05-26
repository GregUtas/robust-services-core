//==============================================================================
//
//  ProtocolSM.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "MsgHeader.h"
#include "MsgPort.h"
#include "RootServiceSM.h"
#include "SbAppIds.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "Signal.h"
#include "Singleton.h"
#include "SteadyTime.h"
#include "SysTypes.h"
#include "Timer.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
ProtocolSM::ProtocolSM(FactoryId fid) :
   fid_(fid),
   state_(Idle)
{
   Debug::ft("ProtocolSM.ctor(first)");

   Initialize(false);
}

//------------------------------------------------------------------------------

ProtocolSM::ProtocolSM(FactoryId fid, ProtocolLayer& adj, bool upper) :
   ProtocolLayer(adj, upper),
   fid_(fid),
   state_(Idle)
{
   Debug::ft("ProtocolSM.ctor(subseq)");

   //  If the layer above created this PSM, queue it after others of equal
   //  priority.  This ensures that the PSM queue will be ordered from the
   //  uppermost to lowermost PSM, which is critical for proper processing
   //  of outgoing messages.
   //
   Initialize(!upper);
}

//------------------------------------------------------------------------------

ProtocolSM::~ProtocolSM()
{
   Debug::ftnt("ProtocolSM.dtor");

   //  Record the PSM's deletion if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = SteadyTime::Now();
      auto buff = Singleton<TraceBuffer>::Extant();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new PsmTrace(PsmTrace::Deletion, *this);
         buff->Insert(rec);
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

bool ProtocolSM::AddressesUnknown(const Message* msg) const
{
   Debug::ft("ProtocolSM.AddressesUnknown");

   if((msg != nullptr) && (msg->RxSbAddr().fid != NIL_ID)) return false;

   auto port = Port();
   if(port == nullptr) return true;
   return (!port->HasRcvdMsg() && !port->HasSentMsg());
}

//------------------------------------------------------------------------------

ProtocolLayer* ProtocolSM::AllocLower(const Message* msg)
{
   Debug::ft("ProtocolSM.AllocLower");

   return new MsgPort(*this);
}

//------------------------------------------------------------------------------

void ProtocolSM::Cleanup()
{
   Debug::ft("ProtocolSM.Cleanup");

   SendFinal();
   ProtocolLayer::Cleanup();
}

//------------------------------------------------------------------------------

void ProtocolSM::Destroy()
{
   Debug::ft("ProtocolSM.Destroy");

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

bool ProtocolSM::DropPeer(const GlobalAddress& peerPrevRemAddr)
{
   Debug::ft("ProtocolSM.DropPeer");

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
               "message not moved", pack3(fid_, state_, m->GetSignal()));
            delete m;
         }
         break;

      default:
         Context::Kill("invalid result", pack2(fid_, rc));
         return;
      }
   }
}

//------------------------------------------------------------------------------

void ProtocolSM::EnqOgMsg(Message& msg)
{
   Debug::ft("ProtocolSM.EnqOgMsg");

   msg.Enqueue(ogMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

Timer* ProtocolSM::FindTimer(const Base& owner, TimerId tid) const
{
   Debug::ft("ProtocolSM.FindTimer");

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

   auto fac = Singleton<FactoryRegistry>::Instance()->GetFactory(fid_);
   if(fac != nullptr) return fac->GetProtocol();

   Debug::SwLog(ProtocolSM_GetProtocol, "factory not found", fid_);
   return NIL_ID;
}

//------------------------------------------------------------------------------

void ProtocolSM::GetSubtended(std::vector<Base*>& objects) const
{
   Debug::ft("ProtocolSM.GetSubtended");

   ProtocolLayer::GetSubtended(objects);

   for(auto m = rcvdMsgq_.First(); m != nullptr; rcvdMsgq_.Next(m))
   {
      m->GetSubtended(objects);
   }

   for(auto m = ogMsgq_.First(); m != nullptr; ogMsgq_.Next(m))
   {
      m->GetSubtended(objects);
   }

   for(auto m = sentMsgq_.First(); m != nullptr; sentMsgq_.Next(m))
   {
      m->GetSubtended(objects);
   }

   for(auto t = timerq_.First(); t != nullptr; timerq_.Next(t))
   {
      t->GetSubtended(objects);
   }
}

//------------------------------------------------------------------------------

void ProtocolSM::HenqOgMsg(Message& msg)
{
   Debug::ft("ProtocolSM.HenqOgMsg");

   msg.Henqueue(ogMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

void ProtocolSM::HenqReceivedMsg(Message& msg)
{
   Debug::ft("ProtocolSM.HenqReceivedMsg");

   msg.Henqueue(rcvdMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

void ProtocolSM::HenqSentMsg(Message& msg)
{
   Debug::ft("ProtocolSM.HenqSentMsg");

   msg.Henqueue(sentMsgq_);
   msg.SetPsm(this);
}

//------------------------------------------------------------------------------

void ProtocolSM::Initialize(bool henq)
{
   Debug::ft("ProtocolSM.Initialize");

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
      auto warp = SteadyTime::Now();
      auto buff = Singleton<TraceBuffer>::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new PsmTrace(PsmTrace::Creation, *this);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

void ProtocolSM::InjectFinalMsg()
{
   Debug::ft("ProtocolSM.InjectFinalMsg");

   //  This is inelegant.  A PSM that can communicate with another node
   //  should therefore override it to inject a session takedown message.
   //
   Kill();
}

//------------------------------------------------------------------------------

ProtocolSM* ProtocolSM::JoinPeer
   (const LocalAddress& peer, GlobalAddress& peerPrevRemAddr)
{
   Debug::ft("ProtocolSM.JoinPeer");

   auto port = EnsurePort()->JoinPeer(peer, peerPrevRemAddr);
   if(port == nullptr) return nullptr;
   auto prid = GetProtocol();

   for(auto layer = port->Upper(); layer != nullptr; layer = layer->Upper())
   {
      auto psm = static_cast<ProtocolSM*>(layer);
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
   msg->SetProtocol(TimerProtocolId);
   msg->SetSignal(Signal::Timeout);
   msg->SetPriority(PROGRESS);
   msg->Header()->kill = true;

   if(!msg->SendToSelf())
   {
      Debug::SwLog(ProtocolSM_Kill, "send failed", fid_);
   }
}

//------------------------------------------------------------------------------

void* ProtocolSM::operator new(size_t size)
{
   Debug::ft("ProtocolSM.operator new");

   return Singleton<ProtocolSMPool>::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void ProtocolSM::Patch(sel_t selector, void* arguments)
{
   ProtocolLayer::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

FactoryId ProtocolSM::PeerFactory() const
{
   Debug::ft("ProtocolSM.PeerFactory");

   auto port = Port();
   if(port == nullptr) return NIL_ID;
   return port->RemAddr().Fid();
}

//------------------------------------------------------------------------------

MsgPort* ProtocolSM::Port() const
{
   Debug::ft("ProtocolSM.Port");

   auto lower = Lower();
   if(lower == nullptr) return nullptr;
   return lower->Port();
}

//------------------------------------------------------------------------------

ProtocolSM::IncomingRc ProtocolSM::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft("ProtocolSM.ProcessIcMsg");

   Context::Kill(strOver(this), fid_);
   return DiscardMessage;
}

//------------------------------------------------------------------------------

ProtocolSM::OutgoingRc ProtocolSM::ProcessOgMsg(Message& msg)
{
   Debug::ft("ProtocolSM.ProcessOgMsg");

   Context::Kill(strOver(this), fid_);
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
         Debug::SwLog(ProtocolSM_ReceiveMsg, "null event", fid_);
      break;
   case DiscardMessage:
      if(evt != nullptr)
         Debug::SwLog(ProtocolSM_ReceiveMsg, "non-null event", fid_);
      break;
   case ReceiveMessage:
      if(evt != nullptr)
         Debug::SwLog(ProtocolSM_ReceiveMsg, "non-null event", fid_);
      return SendToUpper(msg);
   }

   return evt;
}

//------------------------------------------------------------------------------

void ProtocolSM::SendFinal()
{
   Debug::ft("ProtocolSM.SendFinal");

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

   Debug::SwLog(ProtocolSM_SendFinalMsg, strOver(this), 0);
}

//------------------------------------------------------------------------------

bool ProtocolSM::SendMsg(Message& msg)
{
   Debug::ft("ProtocolSM.SendMsg");

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

void ProtocolSM::SetState(StateId stid)
{
   Debug::ft("ProtocolSM.SetState");

   state_ = stid;
}

//------------------------------------------------------------------------------

fn_name ProtocolSM_StartTimer = "ProtocolSM.StartTimer";

bool ProtocolSM::StartTimer(int secs, Base& owner, TimerId tid, bool repeat)
{
   Debug::ft(ProtocolSM_StartTimer);

   //  Prevent a duplicate timer from being started.
   //
   if(FindTimer(owner, tid) != nullptr)
   {
      Debug::SwLog(ProtocolSM_StartTimer,
         "TimerId already in use", pack2(fid_, tid));
      return false;
   }

   new Timer(*this, owner, tid, secs, repeat);
   return true;
}

//------------------------------------------------------------------------------

void ProtocolSM::StopTimer(const Base& owner, TimerId tid)
{
   Debug::ft("ProtocolSM.StopTimer");

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

Message* ProtocolSM::UnwrapMsg(Message& msg)
{
   Debug::ft("ProtocolSM.UnwrapMsg");

   return &msg;
}

//------------------------------------------------------------------------------

ProtocolSM* ProtocolSM::UppermostPsm() const
{
   Debug::ft("ProtocolSM.UppermostPsm");

   auto upper = Upper();
   if(upper == nullptr) return const_cast<ProtocolSM*>(this);
   return upper->UppermostPsm();
}
}
