//==============================================================================
//
//  Message.cpp
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
#include "Message.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Clock.h"
#include "Context.h"
#include "Debug.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "GlobalAddress.h"
#include "InvokerPool.h"
#include "InvokerPoolRegistry.h"
#include "IpPortRegistry.h"
#include "LocalAddress.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "Protocol.h"
#include "ProtocolSM.h"
#include "Q1Way.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "SbTracer.h"
#include "Signal.h"
#include "Singleton.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name Message_ctor1 = "Message.ctor(i/c)";

Message::Message(SbIpBufferPtr& buff) :
   buff_(buff.release()),
   bt_(nullptr),
   handled_(false),
   saves_(0),
   psm_(nullptr),
   whichq_(nullptr)
{
   Debug::ft(Message_ctor1);
}

//------------------------------------------------------------------------------

fn_name Message_ctor2 = "Message.ctor(o/g)";

Message::Message(ProtocolSM* psm, size_t size) :
   buff_(nullptr),
   bt_(nullptr),
   handled_(false),
   saves_(0),
   psm_(psm),
   whichq_(nullptr)
{
   Debug::ft(Message_ctor2);

   buff_.reset(new SbIpBuffer(MsgOutgoing, size));

   //  Create an empty outgoing message with a buffer size of LENGTH and
   //  queue it on PSM.
   //
   if(psm_ != nullptr)
   {
      psm_->EnqOgMsg(*this);
      SetProtocol(psm_->GetProtocol());
   }

   //  Record the message's creation if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = Clock::TicksNow();

      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         new MsgTrace(MsgTrace::Creation, *this, Internal);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

fn_name Message_dtor = "Message.dtor";

Message::~Message()
{
   Debug::ft(Message_dtor);

   //  Record the message's deletion if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = Clock::TicksNow();

      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         new MsgTrace(MsgTrace::Deletion, *this, Internal);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }

   //  Dequeue the message.  If the message is the context message, remove
   //  it from context.
   //
   Exqueue();
   ClearContext();
}

//------------------------------------------------------------------------------

fn_name Message_Capture = "Message.Capture";

void Message::Capture(Route route) const
{
   Debug::ft(Message_Capture);

   auto warp = Clock::TicksNow();
   auto sbt = Singleton< SbTracer >::Instance();
   auto ctx = Context::RunningContext();
   bool trace;
   TransTrace* trans = nullptr;

   if(ctx == nullptr)
   {
      //  If a thread other than an invoker thread (e.g. timer, CLI)
      //  sends a message, there is no running context.
      //
      if(psm_ != nullptr) ctx = psm_->GetContext();
   }

   if(ctx == nullptr)
   {
      trace = (sbt->MsgStatus(*this, MsgOutgoing) == TraceIncluded);
   }
   else
   {
      if(!ctx->TraceOn())
      {
         ctx->SetTrace(sbt->MsgStatus(*this, MsgOutgoing) == TraceIncluded);
      }

      trace = ctx->TraceOn(trans);
   }

   if(trace)
   {
      auto buff = Singleton< TraceBuffer >::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         new MsgTrace(MsgTrace::Transmission, *this, route);
      }

      if(buff->ToolIsOn(BufferTracer))
      {
         auto pool = Singleton< BtIpBufferPool >::Instance();

         if(pool->AvailCount() > 0)
         {
            new BuffTrace(BuffTrace::OgMsg, *buff_);
         }
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

fn_name Message_ChangeDir = "Message.ChangeDir";

void Message::ChangeDir(MsgDirection nextDir)
{
   Debug::ft(Message_ChangeDir);

   //  Verify that the message's direction is actually changing.  If so,
   //  change its direction after flagging it as handled in its current
   //  direction.
   //
   auto currDir = buff_->Dir();

   if(currDir == nextDir)
   {
      Debug::SwLog(Message_ChangeDir, currDir, 0);
      return;
   }

   Handled(true);
   buff_->SetDir(nextDir);
}

//------------------------------------------------------------------------------

fn_name Message_ClearContext = "Message.ClearContext";

void Message::ClearContext() const
{
   Debug::ft(Message_ClearContext);

   //  If this is the context (incoming) message, remove it from context.
   //
   if(Context::ContextMsg() == this)
   {
      Context::SetContextMsg(nullptr);
   }
}

//------------------------------------------------------------------------------

MsgDirection Message::Dir() const
{
   return buff_->Dir();
}

//------------------------------------------------------------------------------

void Message::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "buff    : " << buff_.get() << CRLF;
   stream << prefix << "bt      : " << bt_ << CRLF;
   stream << prefix << "handled : " << handled_ << CRLF;
   stream << prefix << "saves   : " << int(saves_) << CRLF;
   stream << prefix << "psm     : " << psm_ << CRLF;
   stream << prefix << "whichq  : " << whichq_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Message_Enqueue = "Message.Enqueue";

void Message::Enqueue(Q1Way< Message >& whichq)
{
   Debug::ft(Message_Enqueue);

   //  If the message is currently queued, dequeue it.  If enqueueing it
   //  fails, generate a log and delete it, else save its new location.
   //
   if(whichq_ != nullptr) Exqueue();

   if(!whichq.Enq(*this))
   {
      Debug::SwLog(Message_Enqueue, pack2(GetProtocol(), GetSignal()), 0);
      delete this;
      return;
   }

   whichq_ = &whichq;
}

//------------------------------------------------------------------------------

fn_name Message_Exqueue = "Message.Exqueue";

void Message::Exqueue()
{
   Debug::ft(Message_Exqueue);

   if(whichq_ == nullptr) return;

   if(!whichq_->Exq(*this))
   {
      //  The message wasn't where it claimed to be.
      //
      Debug::SwLog(Message_Exqueue, 0, 0);
      return;
   }

   whichq_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name Message_FindSignal = "Message.FindSignal";

Message* Message::FindSignal(SignalId sid) const
{
   Debug::ft(Message_FindSignal);

   //  This is only supported for messages queued against PSMs.
   //
   if((psm_ != nullptr) && (whichq_ != nullptr))
   {
      for(auto m = whichq_->Next(*this); m != nullptr; whichq_->Next(m))
      {
         if(m->GetSignal() == sid) return m;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

ProtocolId Message::GetProtocol() const
{
   return buff_->Header()->protocol;
}

//------------------------------------------------------------------------------

fn_name Message_GetReceiver = "Message.GetReceiver";

GlobalAddress Message::GetReceiver() const
{
   Debug::ft(Message_GetReceiver);

   auto ipaddr = buff_->RxAddr();
   auto sbaddr = buff_->Header()->rxAddr;

   return GlobalAddress(ipaddr, sbaddr);
}

//------------------------------------------------------------------------------

fn_name Message_GetSender = "Message.GetSender";

GlobalAddress Message::GetSender() const
{
   Debug::ft(Message_GetSender);

   auto ipaddr = buff_->TxAddr();
   auto sbaddr = buff_->Header()->txAddr;

   return GlobalAddress(ipaddr, sbaddr);
}

//------------------------------------------------------------------------------

SignalId Message::GetSignal() const
{
   return buff_->Header()->signal;
}

//------------------------------------------------------------------------------

fn_name Message_GetSubtended = "Message.GetSubtended";

void Message::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(Message_GetSubtended);

   Pooled::GetSubtended(objects, count);

   buff_->GetSubtended(objects, count);
}

//------------------------------------------------------------------------------

fn_name Message_Handled = "Message.Handled";

void Message::Handled(bool retain)
{
   Debug::ft(Message_Handled);

   //  An incoming message has been processed or an outgoing message has
   //  been sent.
   //  o Generate a log and return if the message was already handled.
   //  o Notify the PSM (if any) to which the message belongs.
   //  o If the message was the context message, it no longer is.
   //  o If the message was saved (only allowed if it is owned by a PSM),
   //    move it to its PSM's sent message queue if outgoing, then return.
   //  o Delete the message unless told not to.
   //
   if(handled_)
   {
      Debug::SwLog(Message_Handled, pack2(GetProtocol(), GetSignal()), 0);
      return;
   }

   handled_ = !retain;
   if(psm_ != nullptr) psm_->MsgHandled(*this);
   ClearContext();

   if((saves_ > 0) && (psm_ != nullptr))
   {
      if(buff_->Dir() == MsgOutgoing) psm_->HenqSentMsg(*this);
      return;
   }

   if(handled_) delete this;
}

//------------------------------------------------------------------------------

MsgHeader* Message::Header() const
{
   return buff_->Header();
}

//------------------------------------------------------------------------------

fn_name Message_Henqueue = "Message.Henqueue";

void Message::Henqueue(Q1Way< Message >& whichq)
{
   Debug::ft(Message_Henqueue);

   //  If the message is currently queued, dequeue it.  If enqueueing it
   //  fails, generate a log and delete it, else save its new location.
   //
   if(whichq_ != nullptr) Exqueue();

   if(!whichq.Henq(*this))
   {
      Debug::SwLog(Message_Enqueue, pack2(GetProtocol(), GetSignal()), 0);
      delete this;
      return;
   }

   whichq_ = &whichq;
}

//------------------------------------------------------------------------------

fn_name Message_InspectMsg = "Message.InspectMsg";

Message::InspectRc Message::InspectMsg(debug32_t& errval) const
{
   Debug::ft(Message_InspectMsg);

   //e Support message inspection.

   return Ok;
}

//------------------------------------------------------------------------------

fn_name Message_InvalidDiscarded = "Message.InvalidDiscarded";

void Message::InvalidDiscarded() const
{
   Debug::ft(Message_InvalidDiscarded);

   buff_->InvalidDiscarded();
}

//------------------------------------------------------------------------------

fn_name Message_NextMsg = "Message.NextMsg";

Message* Message::NextMsg() const
{
   Debug::ft(Message_NextMsg);

   if(whichq_ == nullptr) return nullptr;

   return whichq_->Next(*this);
}

//------------------------------------------------------------------------------

fn_name Message_new = "Message.operator new";

void* Message::operator new(size_t size)
{
   Debug::ft(Message_new);

   return Singleton< MessagePool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void Message::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Message_Payload = "Message.Payload";

size_t Message::Payload(byte_t*& bytes) const
{
   Debug::ft(Message_Payload);

   return buff_->Payload(bytes);
}

//------------------------------------------------------------------------------

fn_name Message_Refresh = "Message.Refresh";

void Message::Refresh()
{
   Debug::ft(Message_Refresh);

   if(psm_ != nullptr) psm_->RefreshMsg(*this);
}

//------------------------------------------------------------------------------

fn_name Message_Relay = "Message.Relay";

bool Message::Relay(ProtocolSM& ogPsm)
{
   Debug::ft(Message_Relay);

   debug32_t error = 0;

   //  Only an incoming message that is queued on a PSM can be relayed.
   //
   if(buff_->Dir() != MsgIncoming) error |= 0x01;
   if(psm_ == nullptr) error |= 0x04;
   if(!Protocol::Understands(ogPsm.GetProtocol(), GetProtocol())) error |= 0x08;

   if(error == 0)
   {
      //  Move the message to its new PSM, mark it as unhandled, change its
      //  direction, and nullify its transmit and receive addresses.  Because
      //  it is older, it goes at the front of the outgoing message queue.
      //
      ChangeDir(MsgOutgoing);
      ogPsm.HenqOgMsg(*this);
      SetReceiver(GlobalAddress());
      SetSender(GlobalAddress());
      Header()->injected = false;
      handled_ = false;
      return true;
   }

   Debug::SwLog(Message_Relay, pack2(GetProtocol(), GetSignal()), error);
   return false;
}

//------------------------------------------------------------------------------

fn_name Message_Replace = "Message.Replace";

void Message::Replace(SbIpBufferPtr& buff)
{
   Debug::ft(Message_Replace);

   if(buff_ == buff) return;
   buff_.reset(buff.release());
   Refresh();
}

//------------------------------------------------------------------------------

fn_name Message_Restore = "Message.Restore";

bool Message::Restore()
{
   Debug::ft(Message_Restore);

   debug32_t error = 0;

   //  Only an incoming message that is queued on a PSM and that has been
   //  handled (already processed) can be brought into context.
   //
   if(buff_->Dir() != MsgIncoming) error |= 0x01;
   if(!handled_) error |= 0x02;
   if(psm_ == nullptr) error |= 0x04;

   if(error == 0)
   {
      //  Flag the context message as handled.  Put this message at the
      //  head of its PSM's incoming message queue, make that PSM the
      //  context PSM, and tell the PSM to restore this message as the
      //  context message.
      //
      auto msg = Context::ContextMsg();
      if(msg != nullptr) msg->Handled(false);

      psm_->HenqReceivedMsg(*this);
      handled_ = false;
      Context::SetContextMsg(this);
      psm_->RestoreIcMsg(*this);
      return true;
   }

   Debug::SwLog(Message_Restore, pack2(GetProtocol(), GetSignal()), error);
   return false;
}

//------------------------------------------------------------------------------

fn_name Message_Retrieve = "Message.Retrieve";

bool Message::Retrieve(ProtocolSM* psm)
{
   Debug::ft(Message_Retrieve);

   debug32_t error = 0;

   //  Only an outgoing message that is queued on a PSM and that has been
   //  sent (handled) may be retrieved.
   //
   if(buff_->Dir() != MsgOutgoing) error |= 0x01;
   if(!handled_) error |= 0x02;
   if(psm_ == nullptr) error |= 0x04;

   if(error == 0)
   {
      if(psm == nullptr) psm = psm_;
      psm->EnqOgMsg(*this);
      handled_ = false;
      return true;
   }

   Debug::SwLog(Message_Retrieve, pack2(GetProtocol(), GetSignal()), error);
   return false;
}

//------------------------------------------------------------------------------

fn_name Message_RxFactory = "Message.RxFactory";

Factory* Message::RxFactory() const
{
   Debug::ft(Message_RxFactory);

   auto fid = buff_->Header()->rxAddr.fid;

   return Singleton< FactoryRegistry >::Instance()->GetFactory(fid);
}

//------------------------------------------------------------------------------

fn_name Message_RxIpAddr = "Message.RxIpAddr";

const SysIpL3Addr& Message::RxIpAddr() const
{
   Debug::ft(Message_RxIpAddr);

   return buff_->RxAddr();
}

//------------------------------------------------------------------------------

fn_name Message_RxSbAddr = "Message.RxSbAddr";

const LocalAddress& Message::RxSbAddr() const
{
   Debug::ft(Message_RxSbAddr);

   return buff_->Header()->rxAddr;
}

//------------------------------------------------------------------------------

fn_name Message_Save = "Message.Save";

void Message::Save()
{
   Debug::ft(Message_Save);

   ++saves_;
}

//------------------------------------------------------------------------------

fn_name Message_Send = "Message.Send";

bool Message::Send(Route route)
{
   Debug::ft(Message_Send);

   if(buff_->Dir() != MsgOutgoing)
   {
      return SendFailure(pack2(GetProtocol(), GetSignal()), 0);
   }

   auto header = buff_->Header();
   auto facreg = Singleton< FactoryRegistry >::Instance();
   auto txpsm = psm_;
   auto txport = (psm_ == nullptr ? nullptr : psm_->Port());
   bool local = false;
   bool sent = false;
   bool moved = false;

   //  If the message is queued on a PSM but its port does not exist or is
   //  not directly below it, the PSM should have invoked SendToLower.
   //
   if(txpsm != nullptr)
   {
      if((txport == nullptr) || (txpsm->Lower() != txport))
      {
         Debug::SwLog(Message_Send, txpsm->GetFactory(), 1);
         return txpsm->SendToLower(*this);
      }
   }

   //  If the message is on a PSM's outgoing message queue, remove it.  This
   //  prevents trying to send the message again.  TXPSM is the PSM (if any)
   //  on which the message was queued.  Even if the message wasn't removed,
   //  the local TXPSM would be safest because an intraprocessor message can
   //  be moved to its destination, which changes its PSM.
   //
   Exqueue();

   if(route == Internal)
   {
      //  See if the message is local (intraprocessor to a known IP port).
      //
      auto ippreg = Singleton< IpPortRegistry >::Instance();
      local = ippreg->CanBypassStack(buff_->TxAddr(), buff_->RxAddr());
   }

   if(route != External)
   {
      switch(header->priority)
      {
      case Immediate:
         //
         //  An interprocessor message cannot use immediate priority, nor can
         //  a message sent by a MsgFactory.  Generate a log.
         //
         if(!local || (txpsm == nullptr))
         {
            Debug::SwLog(Message_Send, pack2(GetProtocol(), GetSignal()), 2);
            header->priority = Progress;
         }
         break;

      case Ingress:
      case Egress:
         //
         //  Promote the following to progress priority:
         //  (a) a message to a known PSM
         //  (b) a local ingress message (creating a session
         //      for a user who already has one)
         //  (c) a subsequent message
         //
         if((header->rxAddr.bid != NIL_ID) ||               // case (a)
            (local && (header->priority == Ingress)) ||     // case (b)
            ((txport != nullptr) && txport->HasSentMsg()))  // case (c)
         {
            header->priority = Progress;
         }
         break;
      }

      if(txport != nullptr)
      {
         //  If the PSM has neither sent nor received a message, this is an
         //  initial message.  If the PSM is in the idle state, this is a
         //  final message.
         //
         header->initial = (!txport->HasRcvdMsg() && !txport->HasSentMsg());
         header->final = (txpsm->GetState() == ProtocolSM::Idle);
      }
   }

   //  If the receiver is also located on this node, bypass the IP stack and
   //  pass the message directly to the receiver's context.  If the message
   //  has been saved, clone and deliver its buffer so that the sender will
   //  still be able to access the original message.
   //    If the receiver is not located on this processor, send the message
   //  via the IP stack.
   //
   if(local)
   {
      auto fac = facreg->GetFactory(header->rxAddr.fid);

      if(fac == nullptr)
      {
         return SendFailure
            (pack2(GetProtocol(), GetSignal()), pack2(header->rxAddr.fid, 2));
      }

      auto faction = fac->GetFaction();
      auto invreg = Singleton< InvokerPoolRegistry >::Instance();
      auto pool = invreg->Pool(faction);

      if(pool == nullptr)
      {
         return SendFailure
            (pack2(GetProtocol(), GetSignal()), pack2(faction, 3));
      }

      header->route = route;

      if(saves_ == 0)
      {
         ChangeDir(MsgIncoming);
         sent = pool->ReceiveMsg(*this, false);
         moved = true;
      }
      else
      {
         SbIpBufferPtr buff(new SbIpBuffer(*buff_));
         buff->SetDir(MsgIncoming);
         sent = pool->ReceiveBuff(buff, false);
      }
   }
   else
   {
      if(route == Internal) route = IpStack;
      header->route = route;
      sent = buff_->Send(route == External);
   }

   //  If the message was successfully sent, then
   //  o record it in the running context,
   //  o update the sending factory's statistics, and
   //  o allow trace tools to record it.
   //
   if(sent)
   {
      auto ctx = Context::RunningContext();

      if(ctx != nullptr)
      {
         ctx->TraceMsg(header->protocol, header->signal, MsgOutgoing);
      }

      auto fac = facreg->GetFactory(header->txAddr.fid);

      if(fac == nullptr)
         Debug::SwLog(Message_Send, pack2(GetProtocol(), GetSignal()), 3);
      else
         fac->RecordMsg(false, !local, header->length);

      if(Debug::TraceOn()) Capture(route);
   }

   //  If the message was moved to its destination, ChangeDir already invoked
   //  Handled.  However, the messsage needs to clear its PSM now that it is
   //  queued on the destination context.  This was deferred until now so that
   //  trace tools could record the message's PSM.
   //
   if(!moved)
      Handled(false);
   else
      psm_ = nullptr;
   return sent;
}

//------------------------------------------------------------------------------

fn_name Message_SendFailure = "Message.SendFailure";

bool Message::SendFailure(debug64_t errval, debug32_t offset)
{
   Debug::ft(Message_SendFailure);

   Debug::SwLog(Message_SendFailure, errval, offset);
   Handled(false);
   return false;
}

//------------------------------------------------------------------------------

fn_name Message_SendToSelf = "Message.SendToSelf";

bool Message::SendToSelf()
{
   Debug::ft(Message_SendToSelf);

   debug32_t error = 0;

   //  Only an outgoing message that is queued on the PSM and that has not
   //  been sent (handled) may be sent to the same PSM.
   //
   if(buff_->Dir() != MsgOutgoing) error |= 0x01;
   if(handled_) error |= 0x02;
   if(psm_ == nullptr) error |= 0x04;

   if(error == 0)
   {
      //  The message's sending and receiving addresses will both be the
      //  PSM itself.  The OOB flag must be set so that the message won't
      //  cause ProtocolSM::SetRcvd to be invoked.
      //
      auto& addr = psm_->EnsurePort()->LocAddr();
      SetReceiver(addr);
      SetSender(addr);

      auto header = buff_->Header();
      header->self = true;

      if(header->signal != Signal::Timeout)
      {
         header->protocol = psm_->GetProtocol();
      }

      return Send(Internal);
   }

   Debug::SwLog(Message_SendToSelf, pack2(GetProtocol(), GetSignal()), error);
   return false;
}

//------------------------------------------------------------------------------

fn_name Message_SetJoin = "Message.SetJoin";

void Message::SetJoin(bool join)
{
   Debug::ft(Message_SetJoin);

   buff_->Header()->join = join;
}

//------------------------------------------------------------------------------

fn_name Message_SetPriority = "Message.SetPriority";

void Message::SetPriority(Priority prio)
{
   Debug::ft(Message_SetPriority);

   buff_->Header()->priority = prio;
}

//------------------------------------------------------------------------------

fn_name Message_SetProtocol = "Message.SetProtocol";

void Message::SetProtocol(ProtocolId prid)
{
   Debug::ft(Message_SetProtocol);

   buff_->Header()->protocol = prid;
}

//------------------------------------------------------------------------------

fn_name Message_SetPsm = "Message.SetPsm";

void Message::SetPsm(ProtocolSM* psm)
{
   Debug::ft(Message_SetPsm);

   psm_ = psm;
}

//------------------------------------------------------------------------------

fn_name Message_SetReceiver = "Message.SetReceiver";

void Message::SetReceiver(const GlobalAddress& receiver)
{
   Debug::ft(Message_SetReceiver);

   if(buff_->Dir() == MsgOutgoing)
   {
      buff_->SetRxAddr(receiver);
      buff_->Header()->rxAddr = receiver.SbAddr();
   }
}

//------------------------------------------------------------------------------

fn_name Message_SetRxAddr = "Message.SetRxAddr";

void Message::SetRxAddr(const LocalAddress& rxaddr)
{
   Debug::ft(Message_SetRxAddr);

   if(buff_->Dir() == MsgIncoming)
   {
      buff_->Header()->rxAddr = rxaddr;

      if(bt_ != nullptr)
      {
         //  A buffer that captures a message is freed when TraceBuffer wraps
         //  around.  Under heavy load, this could occur before its ingress
         //  message actually gets processed, in which case bt_ now points to
         //  a trace record that has been overwritten.
         //
         auto bt = dynamic_cast< const BuffTrace* >(bt_);
         if(bt != nullptr) bt->Header()->rxAddr = rxaddr;
      }
   }
}

//------------------------------------------------------------------------------

fn_name Message_SetSender = "Message.SetSender";

void Message::SetSender(const GlobalAddress& sender)
{
   Debug::ft(Message_SetSender);

   if(buff_->Dir() == MsgOutgoing)
   {
      buff_->SetTxAddr(sender);
      buff_->Header()->txAddr = sender.SbAddr();
   }
}

//------------------------------------------------------------------------------

fn_name Message_SetSignal = "Message.SetSignal";

void Message::SetSignal(SignalId sid)
{
   Debug::ft(Message_SetSignal);

   buff_->Header()->signal = sid;
}

//------------------------------------------------------------------------------

fixed_string PriorityStrings[Message::MaxPriority + 2] =
{
   "ingress",
   "egress",
   "progress",
   "immediate",
   ERROR_STR
};

const char* Message::strPriority(Priority prio)
{
   if((prio >= 0) && (prio <= MaxPriority)) return PriorityStrings[prio];
   return PriorityStrings[MaxPriority + 1];
}

//------------------------------------------------------------------------------

fn_name Message_TxIpAddr = "Message.TxIpAddr";

const SysIpL3Addr& Message::TxIpAddr() const
{
   Debug::ft(Message_TxIpAddr);

   return buff_->TxAddr();
}

//------------------------------------------------------------------------------

fn_name Message_TxSbAddr = "Message.TxSbAddr";

const LocalAddress& Message::TxSbAddr() const
{
   Debug::ft(Message_TxSbAddr);

   return buff_->Header()->txAddr;
}

//------------------------------------------------------------------------------

fn_name Message_Unsave = "Message.Unsave";

void Message::Unsave()
{
   Debug::ft(Message_Unsave);

   //  If the save count drops to zero, delete the message unless it has
   //  yet to be handled.
   //
   if(saves_ > 0)
      --saves_;
   else
      Debug::SwLog(Message_Unsave, pack2(GetProtocol(), GetSignal()), 0);

   if((saves_ == 0) && handled_) delete this;
}
}
