//==============================================================================
//
//  Message.cpp
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
#include "Message.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
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
#include "Registry.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "SbTracer.h"
#include "Signal.h"
#include "Singleton.h"
#include "SteadyTime.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NetworkBase;
using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
Message::Message(SbIpBufferPtr& buff) :
   buff_(buff.release()),
   bt_(nullptr),
   handled_(false),
   saves_(0),
   psm_(nullptr),
   whichq_(nullptr)
{
   Debug::ft("Message.ctor(i/c)");
}

//------------------------------------------------------------------------------

Message::Message(ProtocolSM* psm, size_t size) :
   buff_(nullptr),
   bt_(nullptr),
   handled_(false),
   saves_(0),
   psm_(psm),
   whichq_(nullptr)
{
   Debug::ft("Message.ctor(o/g)");

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
      auto warp = SteadyTime::Now();
      auto buff = Singleton<TraceBuffer>::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new MsgTrace(MsgTrace::Creation, *this, Internal);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

Message::~Message()
{
   Debug::ftnt("Message.dtor");

   //  Record the message's deletion if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = SteadyTime::Now();
      auto buff = Singleton<TraceBuffer>::Extant();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new MsgTrace(MsgTrace::Deletion, *this, Internal);
         buff->Insert(rec);
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

void Message::Capture(Route route) const
{
   Debug::ft("Message.Capture");

   auto warp = SteadyTime::Now();
   auto sbt = Singleton<SbTracer>::Instance();
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
      auto buff = Singleton<TraceBuffer>::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new MsgTrace(MsgTrace::Transmission, *this, route);
         buff->Insert(rec);
      }

      if(buff->ToolIsOn(BufferTracer))
      {
         auto rec = new BuffTrace(BuffTrace::OgMsg, *buff_);
         buff->Insert(rec);
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
      Debug::SwLog(Message_ChangeDir, "direction already set", currDir);
      return;
   }

   Handled(true);
   buff_->SetDir(nextDir);
}

//------------------------------------------------------------------------------

void Message::ClearContext() const
{
   Debug::ft("Message.ClearContext");

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

void Message::Enqueue(Q1Way<Message>& whichq)
{
   Debug::ft(Message_Enqueue);

   //  If the message is currently queued, dequeue it.  If enqueueing it
   //  fails, generate a log and delete it, else save its new location.
   //
   if(whichq_ != nullptr) Exqueue();

   if(!whichq.Enq(*this))
   {
      Debug::SwLog(Message_Enqueue,
         "Enq failed", pack2(GetProtocol(), GetSignal()));
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
      Debug::SwLog(Message_Exqueue,
         "Exq failed", pack2(GetProtocol(), GetSignal()));
      return;
   }

   whichq_ = nullptr;
}

//------------------------------------------------------------------------------

Message* Message::FindSignal(SignalId sid) const
{
   Debug::ft("Message.FindSignal");

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

GlobalAddress Message::GetReceiver() const
{
   Debug::ft("Message.GetReceiver");

   const auto& ipaddr = buff_->RxAddr();
   auto sbaddr = buff_->Header()->rxAddr;

   return GlobalAddress(ipaddr, sbaddr);
}

//------------------------------------------------------------------------------

GlobalAddress Message::GetSender() const
{
   Debug::ft("Message.GetSender");

   const auto& ipaddr = buff_->TxAddr();
   auto sbaddr = buff_->Header()->txAddr;

   return GlobalAddress(ipaddr, sbaddr);
}

//------------------------------------------------------------------------------

SignalId Message::GetSignal() const
{
   return buff_->Header()->signal;
}

//------------------------------------------------------------------------------

void Message::GetSubtended(std::vector<Base*>& objects) const
{
   Debug::ft("Message.GetSubtended");

   Pooled::GetSubtended(objects);

   buff_->GetSubtended(objects);
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
      Debug::SwLog(Message_Handled,
         "message already handled", pack2(GetProtocol(), GetSignal()));
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

void Message::Henqueue(Q1Way<Message>& whichq)
{
   Debug::ft("Message.Henqueue");

   //  If the message is currently queued, dequeue it.  If enqueueing it
   //  fails, generate a log and delete it, else save its new location.
   //
   if(whichq_ != nullptr) Exqueue();

   if(!whichq.Henq(*this))
   {
      Debug::SwLog(Message_Enqueue,
         "Henq failed", pack2(GetProtocol(), GetSignal()));
      delete this;
      return;
   }

   whichq_ = &whichq;
}

//------------------------------------------------------------------------------

Message::InspectRc Message::InspectMsg(debug64_t& errval) const
{
   Debug::ft("Message.InspectMsg");

   //e Support message inspection.

   return Ok;
}

//------------------------------------------------------------------------------

void Message::InvalidDiscarded() const
{
   Debug::ft("Message.InvalidDiscarded");

   buff_->InvalidDiscarded();
}

//------------------------------------------------------------------------------

Message* Message::NextMsg() const
{
   Debug::ft("Message.NextMsg");

   if(whichq_ == nullptr) return nullptr;

   return whichq_->Next(*this);
}

//------------------------------------------------------------------------------

void* Message::operator new(size_t size)
{
   Debug::ft("Message.operator new");

   return Singleton<MessagePool>::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

bool Message::Passes(uint32_t selector) const
{
   if(selector == 0) return true;
   auto pid = selector >> 8;
   if(pid == 0) return true;
   if(GetProtocol() != pid) return false;
   auto sid = selector & 0xff;
   if(sid == 0) return true;
   return (GetSignal() == sid);
}

//------------------------------------------------------------------------------

void Message::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

size_t Message::Payload(byte_t*& bytes) const
{
   Debug::ft("Message.Payload");

   return buff_->Payload(bytes);
}

//------------------------------------------------------------------------------

void Message::Refresh()
{
   Debug::ft("Message.Refresh");

   if(psm_ != nullptr) psm_->RefreshMsg(*this);
}

//------------------------------------------------------------------------------

fn_name Message_Relay = "Message.Relay";

bool Message::Relay(ProtocolSM& ogPsm)
{
   Debug::ft(Message_Relay);

   debug64_t error = 0;

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

   Debug::SwLog(Message_Relay, "invalid operation", error);
   return false;
}

//------------------------------------------------------------------------------

void Message::Replace(SbIpBufferPtr& buff)
{
   Debug::ft("Message.Replace");

   if(buff_ == buff) return;
   buff_.reset(buff.release());
   Refresh();
}

//------------------------------------------------------------------------------

fn_name Message_Restore = "Message.Restore";

bool Message::Restore()
{
   Debug::ft(Message_Restore);

   debug64_t error = 0;

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

   Debug::SwLog(Message_Restore, "invalid operation", error);
   return false;
}

//------------------------------------------------------------------------------

fn_name Message_Retrieve = "Message.Retrieve";

bool Message::Retrieve(ProtocolSM* psm)
{
   Debug::ft(Message_Retrieve);

   debug64_t error = 0;

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

   Debug::SwLog(Message_Retrieve, "invalid operation", error);
   return false;
}

//------------------------------------------------------------------------------

Factory* Message::RxFactory() const
{
   Debug::ft("Message.RxFactory");

   auto fid = buff_->Header()->rxAddr.fid;

   return Singleton<FactoryRegistry>::Instance()->Factories().At(fid);
}

//------------------------------------------------------------------------------

const SysIpL3Addr& Message::RxIpAddr() const
{
   Debug::ft("Message.RxIpAddr");

   return buff_->RxAddr();
}

//------------------------------------------------------------------------------

const LocalAddress& Message::RxSbAddr() const
{
   Debug::ft("Message.RxSbAddr");

   return buff_->Header()->rxAddr;
}

//------------------------------------------------------------------------------

void Message::Save()
{
   Debug::ft("Message.Save");

   ++saves_;
}

//------------------------------------------------------------------------------

fn_name Message_Send = "Message.Send";

bool Message::Send(Route route)
{
   Debug::ft(Message_Send);

   if(buff_->Dir() != MsgOutgoing)
   {
      return SendFailure(pack2(GetProtocol(), GetSignal()));
   }

   auto header = buff_->Header();
   auto facreg = Singleton<FactoryRegistry>::Instance();
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
         Debug::SwLog(Message_Send,
            "MsgPort not adjacent", pack2(GetProtocol(), GetSignal()));
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
      auto ippreg = Singleton<IpPortRegistry>::Instance();
      local = ippreg->CanBypassStack(buff_->TxAddr(), buff_->RxAddr());
   }

   if(route != External)
   {
      switch(header->priority)
      {
      case IMMEDIATE:
         //
         //  An interprocessor message cannot use immediate priority, nor can
         //  a message sent by a MsgFactory.  Generate a log.
         //
         if(!local || (txpsm == nullptr))
         {
            Debug::SwLog(Message_Send,
               "invalid priority", pack2(GetProtocol(), GetSignal()));
            header->priority = PROGRESS;
         }
         break;

      case INGRESS:
      case EGRESS:
         //
         //  Promote the following to progress priority:
         //  (a) a message to a known PSM
         //  (b) a local ingress message (creating a session
         //      for a user who already has one)
         //  (c) a subsequent message
         //
         if((header->rxAddr.bid != NIL_ID) ||               // case (a)
            (local && (header->priority == INGRESS)) ||     // case (b)
            ((txport != nullptr) && txport->HasSentMsg()))  // case (c)
         {
            header->priority = PROGRESS;
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
      auto fac = facreg->Factories().At(header->rxAddr.fid);

      if(fac == nullptr)
      {
         return SendFailure(pack2(GetProtocol(), GetSignal()));
      }

      auto faction = fac->GetFaction();
      auto invreg = Singleton<InvokerPoolRegistry>::Instance();
      auto pool = invreg->Pool(faction);

      if(pool == nullptr)
      {
         return SendFailure(pack2(GetProtocol(), GetSignal()));
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

      auto fac = facreg->Factories().At(header->txAddr.fid);

      if(fac == nullptr)
         Debug::SwLog(Message_Send,
            "factory not found", pack2(GetProtocol(), GetSignal()));
      else
         fac->RecordMsg(false, !local, header->length);

      if(Debug::TraceOn()) Capture(route);
   }

   //  If the message was moved to its destination, ChangeDir already invoked
   //  Handled.  However, the message needs to clear its PSM now that it is
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

bool Message::SendFailure(debug64_t errval)
{
   Debug::ft(Message_SendFailure);

   Debug::SwLog(Message_SendFailure, "send failed", errval);
   Handled(false);
   return false;
}

//------------------------------------------------------------------------------

fn_name Message_SendToSelf = "Message.SendToSelf";

bool Message::SendToSelf()
{
   Debug::ft(Message_SendToSelf);

   debug64_t error = 0;

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

   Debug::SwLog(Message_SendToSelf,
      "failed to send message", pack2(GetProtocol(), GetSignal()));
   return false;
}

//------------------------------------------------------------------------------

void Message::SetJoin(bool join)
{
   Debug::ft("Message.SetJoin");

   buff_->Header()->join = join;
}

//------------------------------------------------------------------------------

void Message::SetPriority(MsgPriority prio)
{
   Debug::ft("Message.SetPriority");

   buff_->Header()->priority = prio;
}

//------------------------------------------------------------------------------

void Message::SetProtocol(ProtocolId prid)
{
   Debug::ft("Message.SetProtocol");

   buff_->Header()->protocol = prid;
}

//------------------------------------------------------------------------------

void Message::SetPsm(ProtocolSM* psm)
{
   Debug::ft("Message.SetPsm");

   psm_ = psm;
}

//------------------------------------------------------------------------------

void Message::SetReceiver(const GlobalAddress& receiver)
{
   Debug::ft("Message.SetReceiver");

   if(buff_->Dir() == MsgOutgoing)
   {
      buff_->SetRxAddr(receiver);
      buff_->Header()->rxAddr = receiver.SbAddr();
   }
}

//------------------------------------------------------------------------------

void Message::SetRxAddr(const LocalAddress& rxaddr)
{
   Debug::ft("Message.SetRxAddr");

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
         auto bt = dynamic_cast<const BuffTrace*>(bt_);
         if(bt != nullptr) bt->Header()->rxAddr = rxaddr;
      }
   }
}

//------------------------------------------------------------------------------

void Message::SetSender(const GlobalAddress& sender)
{
   Debug::ft("Message.SetSender");

   if(buff_->Dir() == MsgOutgoing)
   {
      buff_->SetTxAddr(sender);
      buff_->Header()->txAddr = sender.SbAddr();
   }
}

//------------------------------------------------------------------------------

void Message::SetSignal(SignalId sid)
{
   Debug::ft("Message.SetSignal");

   buff_->Header()->signal = sid;
}

//------------------------------------------------------------------------------

const SysIpL3Addr& Message::TxIpAddr() const
{
   Debug::ft("Message.TxIpAddr");

   return buff_->TxAddr();
}

//------------------------------------------------------------------------------

const LocalAddress& Message::TxSbAddr() const
{
   Debug::ft("Message.TxSbAddr");

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
      Debug::SwLog(Message_Unsave,
         "underflow", pack2(GetProtocol(), GetSignal()));

   if((saves_ == 0) && handled_) delete this;
}
}
