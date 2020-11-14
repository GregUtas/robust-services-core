//==============================================================================
//
//  SbTrace.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "SbTrace.h"
#include <iomanip>
#include <ios>
#include <sstream>
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "GlobalAddress.h"
#include "InvokerThread.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "ProtocolSM.h"
#include "Restart.h"
#include "RootServiceSM.h"
#include "SbEvents.h"
#include "SbIpBuffer.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "Signal.h"
#include "Singleton.h"
#include "State.h"
#include "Timer.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"
#include "TraceDump.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
TransTrace::TransTrace(const Message& msg, const Factory& fac) :
   TimedRecord(TransTracer),
   rcvr_(&fac),
   buff_(msg.Buffer()),
   cid_(fac.Fid()),
   service_(false),
   type_(fac.GetType()),
   prio_(msg.Header()->priority),
   prid_(msg.GetProtocol()),
   sid_(msg.GetSignal())
{
   rid_ = RxNet;
   time0_ = msg.Buffer()->RxTime();
   time1_ = time0_;
}

//------------------------------------------------------------------------------

TransTrace::TransTrace
   (const Context& ctx, const Message& msg, const InvokerThread* inv) :
   TimedRecord(TransTracer),
   rcvr_(&ctx),
   buff_(msg.Buffer()),
   cid_(NIL_ID),
   service_(false),
   type_(ctx.Type()),
   prio_(msg.Header()->priority),
   prid_(msg.GetProtocol()),
   sid_(msg.GetSignal())
{
   rid_ = Trans;
   auto fac = msg.RxFactory();
   cid_ = fac->Fid();

   switch(type_)
   {
   case SingleMsg:
      rcvr_ = fac;
      break;

   case MultiPort:
      auto root = ctx.RootSsm();

      if(root != nullptr)
      {
         cid_ = root->Sid();
         service_ = true;
      }
   }

   time0_ = inv->Time0();
   time1_ = time0_;
}

//------------------------------------------------------------------------------

bool TransTrace::Display(ostream& stream, const string& opts)
{
   if(!TimedRecord::Display(stream, opts)) return false;

   auto delta = time1_ - time0_;
   stream << setw(TraceDump::TotWidth) << delta.To(uSECS) << TraceDump::Tab();

   stream << rcvr_ << TraceDump::Tab();

   stream << type_ << SPACE;

   if(rid_ == Trans)
      stream << "prio=" << int(prio_);
   else
      stream << spaces(TraceDump::IdRcWidth - 4);

   stream << TraceDump::Tab();

   if(rid_ == RxNet)
   {
      if(service_)
      {
         auto reg = Singleton< ServiceRegistry >::Instance();
         stream << strClass(reg->GetService(ServiceId(cid_)), false);
      }
      else
      {
         auto reg = Singleton< FactoryRegistry >::Instance();
         stream << strClass(reg->GetFactory(FactoryId(cid_)), false);
      }
   }
   else
   {
      auto reg = Singleton< ProtocolRegistry >::Instance();
      auto pro = reg->GetProtocol(prid_);

      if(pro != nullptr) stream << strClass(pro->GetSignal(sid_), false);
   }

   return true;
}

//------------------------------------------------------------------------------

void TransTrace::EndOfTransaction()
{
   //  Set the time at which this transaction ended.
   //
   time1_ = TimePoint::Now();
}

//------------------------------------------------------------------------------

fixed_string RxNetEventStr = "RXNET";
fixed_string TransEventStr = "TRANS";

c_string TransTrace::EventString() const
{
   switch(rid_)
   {
   case RxNet: return RxNetEventStr;
   case Trans: return TransEventStr;
   }

   return ERROR_STR;
}

//------------------------------------------------------------------------------

void TransTrace::ResumeTime(const TimePoint& then)
{
   //  Adjust this transaction's elapsed time so that the time spent since
   //  THEN is excluded.
   //
   auto warp = TimePoint::Now() - then;
   time0_ += warp;
   time1_ = time0_;
}

//------------------------------------------------------------------------------

void TransTrace::SetContext(const void* ctx)
{
   //  This should only be invoked on an RxNet record once the context is
   //  known.  If the context is a MsgFactory, retain the factory as the
   //  receiver.
   //
   if((rid_ == RxNet) && (type_ != SingleMsg))
   {
      rcvr_ = ctx;
   }
}

//------------------------------------------------------------------------------

void TransTrace::SetService(ServiceId sid)
{
   cid_ = sid;
   service_ = true;
}

//==============================================================================

BuffTrace::BuffTrace(Id rid, const SbIpBuffer& buff) :
   TimedRecord(BufferTracer),
   buff_(nullptr),
   verified_(false),
   corrupt_(false)
{
   rid_ = rid;
   buff_ = new (ToolUser) SbIpBuffer(buff);
}

//------------------------------------------------------------------------------

BuffTrace::~BuffTrace()
{
   //  If a StTestData::lastMsg_ or TestSession::lastMsg_ points to this record,
   //  it will probably lead to a trap.  The odds of this are remote because it
   //  means that the trace buffer wrapped around and caught up with the last
   //  message verified by the factory or PSM.
   //
   //  If our SbIpBuffer is corrupt, we will trap, and we must not trap again
   //  during cleanup.  Flag the buffer as corrupt before deleting it and
   //  clear the flag afterwards.  If it flagged as corrupt when we come in
   //  here, we know that it was bad, so skip it and let the audit find it.
   //
   if((buff_ != nullptr) && !buff_->IsInvalid())
   {
      if(!corrupt_)
      {
         corrupt_ = true;
         delete buff_;
      }

      buff_ = nullptr;
      corrupt_ = false;
   }
}

//------------------------------------------------------------------------------

FactoryId BuffTrace::ActiveFid() const
{
   auto header = Header();
   if(header == nullptr) return NIL_ID;
   if(rid_ == IcMsg) return header->rxAddr.fid;
   return header->txAddr.fid;
}

//------------------------------------------------------------------------------

void BuffTrace::ClaimBlocks()
{
   Debug::ft("BuffTrace.ClaimBlocks");

   if((buff_ != nullptr) && !corrupt_ && !buff_->IsInvalid())
   {
      buff_->ClaimBlocks();
   }
}

//------------------------------------------------------------------------------

bool BuffTrace::Display(ostream& stream, const string& opts)
{
   if(!TimedRecord::Display(stream, opts)) return false;

   stream << CRLF;
   stream << string(COUT_LENGTH_MAX, '-') << CRLF;

   if(buff_ == nullptr)
   {
      stream << "No buffer found." << CRLF;
      stream << string(COUT_LENGTH_MAX, '-');
      return true;
   }

   auto fid = ActiveFid();
   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(fid);
   stream << "factory=" << int(fid);
   stream << " (" << strClass(fac, false) << ')' << CRLF;

   if(!buff_->IsInvalid()) buff_->Display(stream, spaces(2), VerboseOpt);
   stream << string(COUT_LENGTH_MAX, '-');
   return true;
}

//------------------------------------------------------------------------------

fixed_string IcMsgEventStr = "icmsg";
fixed_string OgMsgEventStr = "ogmsg";

c_string BuffTrace::EventString() const
{
   switch(rid_)
   {
   case IcMsg: return IcMsgEventStr;
   case OgMsg: return OgMsgEventStr;
   }

   return ERROR_STR;
}

//------------------------------------------------------------------------------

MsgHeader* BuffTrace::Header() const
{
   if(buff_ == nullptr) return nullptr;
   return buff_->Header();
}

//------------------------------------------------------------------------------

fn_name BuffTrace_NextIcMsg = "BuffTrace.NextIcMsg";

BuffTrace* BuffTrace::NextIcMsg
   (BuffTrace* bt, FactoryId fid, SignalId sid, SkipInfo& skip)
{
   Debug::ft(BuffTrace_NextIcMsg);

   auto buff = Singleton< TraceBuffer >::Instance();

   buff->Lock();
   {
      TraceRecord* rec = bt;
      auto max = 200;
      Flags mask(1 << BufferTracer);

      for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
      {
         //  Skip messages that were already verified or that were injected.
         //
         bt = static_cast< BuffTrace* >(rec);
         if(bt->verified_) continue;

         auto header = bt->Header();
         if(header->injected) continue;

         //  When a message bypasses the IP stack, the trace only captures
         //  the outgoing message, so that is what we must use.  But when a
         //  message arrives over the IP stack, we need to use the incoming
         //  message because
         //  o if the message was interprocessor, only another processor
         //    could have captured the outgoing message;
         //  o if the message was an *initial* intraprocessor message, only
         //    the incoming message contains the recipient's address (which
         //    is not known until a MsgPort is allocated), and we need that
         //    address in order to find the test SSM and PSM.
         //
         auto rid = rec->Rid();
         if((rid == OgMsg) && (header->route != Message::Internal)) continue;

         //  Skip messages that do not match FID and SID, but note the first
         //  one's signal and count them.
         //
         if(header->rxAddr.fid == fid)
         {
            if(header->signal == sid)
            {
               buff->Unlock();
               return bt;
            }

            if(header->signal != Signal::Timeout)
            {
               if(skip.count == 0)
               {
                  skip.first = header->signal;
               }

               ++skip.count;
            }
         }

         if(--max <= 0)
         {
            Debug::SwLog(BuffTrace_NextIcMsg,
               "message not found", pack2(fid, sid));
            break;
         }
      }
   }
   buff->Unlock();
   return nullptr;
}

//------------------------------------------------------------------------------

Message* BuffTrace::Rewrap()
{
   Debug::ft("BuffTrace.Rewrap");

   if(buff_ == nullptr) return nullptr;

   auto reg = Singleton< FactoryRegistry >::Instance();
   auto fac = reg->GetFactory(Header()->rxAddr.fid);
   SbIpBufferPtr ipb(new (ToolUser) SbIpBuffer(*buff_));

   verified_ = true;
   if(ipb == nullptr) return nullptr;
   return fac->ReallocOgMsg(ipb);
}

//------------------------------------------------------------------------------

void BuffTrace::Shutdown(RestartLevel level)
{
   Debug::ft("BuffTrace.Shutdown");

   if(buff_ != nullptr)
   {
      if(!Restart::ClearsMemory(buff_->MemType())) return;
   }

   Nullify();
}

//==============================================================================

SboTrace::SboTrace(const Pooled& sbo) :
   TimedRecord(ContextTracer),
   sbo_(&sbo)
{
}

//------------------------------------------------------------------------------

bool SboTrace::Display(ostream& stream, const string& opts)
{
   if(!TimedRecord::Display(stream, opts)) return false;

   stream << spaces(TraceDump::EvtToObj) << sbo_ << TraceDump::Tab();
   return true;
}

//------------------------------------------------------------------------------

string SboTrace::OutputId(const string& label, id_t id)
{
   auto width = TraceDump::IdRcWidth + TraceDump::TabWidth;
   if(id == NIL_ID) return spaces(width);
   width -= col_t(label.size());

   std::ostringstream stream;
   stream << label;
   stream << std::left << setw(width) << std::setfill(SPACE) << id;
   return stream.str();
}

//==============================================================================

SsmTrace::SsmTrace(Id rid, const ServiceSM& ssm) :
   SboTrace(ssm),
   sid_(ssm.Sid())
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

bool SsmTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   auto reg = Singleton< ServiceRegistry >::Instance();

   stream << spaces(TraceDump::ObjToDesc);
   stream << strClass(reg->GetService(sid_), false);

   return true;
}

//------------------------------------------------------------------------------

fixed_string SsmCreationEventStr = " +ssm";
fixed_string SsmDeletionEventStr = " -ssm";

c_string SsmTrace::EventString() const
{
   switch(rid_)
   {
   case Creation: return SsmCreationEventStr;
   case Deletion: return SsmDeletionEventStr;
   }

   return SboTrace::EventString();
}

//==============================================================================

PsmTrace::PsmTrace(Id rid, const ProtocolSM& psm) :
   SboTrace(psm),
   fid_(psm.GetFactory()),
   bid_(NIL_ID)
{
   rid_ = rid;
   auto port = psm.Port();
   if(port != nullptr) bid_ = port->LocAddr().SbAddr().bid;
}

//------------------------------------------------------------------------------

bool PsmTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   auto reg = Singleton< FactoryRegistry >::Instance();

   stream << OutputId("port=", bid_);
   stream << strClass(reg->GetFactory(fid_), false);

   return true;
}

//------------------------------------------------------------------------------

fixed_string PsmCreationEventStr = " +psm";
fixed_string PsmDeletionEventStr = " -psm";

c_string PsmTrace::EventString() const
{
   switch(rid_)
   {
   case Creation: return PsmCreationEventStr;
   case Deletion: return PsmDeletionEventStr;
   }

   return SboTrace::EventString();
}

//==============================================================================

PortTrace::PortTrace(Id rid, const MsgPort& port) :
   SboTrace(port),
   fid_(port.ObjAddr().fid),
   bid_(port.ObjAddr().bid)
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

bool PortTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   auto reg = Singleton< FactoryRegistry >::Instance();

   stream << OutputId("port=", bid_);
   stream << strClass(reg->GetFactory(fid_), false);

   return true;
}

//------------------------------------------------------------------------------

fixed_string PortCreationEventStr = "+port";
fixed_string PortDeletionEventStr = "-port";

c_string PortTrace::EventString() const
{
   switch(rid_)
   {
   case Creation: return PortCreationEventStr;
   case Deletion: return PortDeletionEventStr;
   }

   return SboTrace::EventString();
}

//==============================================================================

MsgTrace::MsgTrace(Id rid, const Message& msg, Message::Route route) :
   SboTrace(msg),
   prid_(msg.GetProtocol()),
   sid_(msg.GetSignal()),
   route_(route),
   noCtx_(Context::RunningContext() == nullptr),
   self_(msg.Header()->self)
{
   rid_ = rid;

   ProtocolSM* psm;

   switch(rid)
   {
   case Creation:
   case Deletion:
      psm = msg.Psm();
      if(psm != nullptr)
      {
         auto port = psm->Port();
         if(port != nullptr) locAddr_.bid = port->ObjAddr().bid;
      }
      break;

   case Reception:
   case Transmission:
      if((rid == Reception) || self_)
      {
         locAddr_ = msg.RxSbAddr();
         remAddr_ = msg.TxSbAddr();
      }
      else
      {
         locAddr_ = msg.TxSbAddr();
         remAddr_ = msg.RxSbAddr();
      }
   }
}

//------------------------------------------------------------------------------

bool MsgTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   stream << OutputId("port=", locAddr_.bid);

   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid_);
   Signal* sig = nullptr;

   if(pro != nullptr) sig = pro->GetSignal(sid_);

   if(sig != nullptr)
   {
      stream << strClass(sig, false);
   }
   else
   {
      if(pro != nullptr)
         stream << strClass(pro, false);
      else
         stream << "pro=" << prid_;

      stream << " sig=" << sid_;
   }

   return true;
}

//------------------------------------------------------------------------------

fixed_string MsgCreationEventStr     = " +msg";
fixed_string MsgDeletionEventStr     = " -msg";
fixed_string MsgReceptionEventStr    = ">>msg";
fixed_string MsgTransmissionEventStr = "<<msg";

c_string MsgTrace::EventString() const
{
   switch(rid_)
   {
   case Creation: return MsgCreationEventStr;
   case Deletion: return MsgDeletionEventStr;
   case Reception: return MsgReceptionEventStr;
   case Transmission: return MsgTransmissionEventStr;
   }

   return SboTrace::EventString();
}

//==============================================================================

TimerTrace::TimerTrace(Id rid, const Timer& tmr) :
   SboTrace(tmr),
   tid_(tmr.Tid()),
   secs_(tmr.duration_),
   psm_(tmr.Psm())
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

bool TimerTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   stream << OutputId("id=", tid_);
   stream << "secs=" << secs_ << " psm=" << psm_;
   return true;
}

//------------------------------------------------------------------------------

fixed_string TimerCreationEventStr = " +tmr";
fixed_string TimerDeletionEventStr = " -tmr";

c_string TimerTrace::EventString() const
{
   switch(rid_)
   {
   case Creation: return TimerCreationEventStr;
   case Deletion: return TimerDeletionEventStr;
   }

   return SboTrace::EventString();
}

//==============================================================================

EventTrace::EventTrace(Id rid, const Event& evt) :
   SboTrace(evt),
   owner_(NIL_ID),
   eid_(evt.Eid())
{
   rid_ = rid;
   auto ssm = evt.Owner();
   if(ssm != nullptr) owner_ = ssm->Sid();
}

//------------------------------------------------------------------------------

EventTrace::EventTrace(const Event& evt) :
   SboTrace(evt),
   owner_(NIL_ID),
   eid_(evt.Eid())
{
   auto ssm = evt.Owner();
   if(ssm != nullptr) owner_ = ssm->Sid();
}

//------------------------------------------------------------------------------

bool EventTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   stream << spaces(TraceDump::ObjToDesc);
   DisplayEvent(stream, owner_, eid_);
   return true;
}

//------------------------------------------------------------------------------

void EventTrace::DisplayEvent(ostream& stream, ServiceId sid, EventId eid)
{
   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);

   if(svc != nullptr)
   {
      auto name = svc->EventName(eid);

      if(name != nullptr)
      {
         stream << name;
         return;
      }

      stream << strClass(svc, true) << ", ";
   }
   else if(sid != NIL_ID)
   {
      stream << "svc=" << sid << ", ";
   }

   stream << "evt=" << eid;
}

//------------------------------------------------------------------------------

fixed_string EventCreationEventStr = " +evt";
fixed_string EventDeletionEventStr = " -evt";
fixed_string EventHandlerEventStr  = ">>evt";

c_string EventTrace::EventString() const
{
   switch(rid_)
   {
   case Creation: return EventCreationEventStr;
   case Deletion: return EventDeletionEventStr;
   }

   return EventHandlerEventStr;
}

//==============================================================================

HandlerTrace::HandlerTrace
   (ServiceId sid, const State& state, const Event& evt, EventHandler::Rc rc) :
   EventTrace(evt),
   sid_(sid),
   stid_(state.Stid()),
   rc_(rc)
{
   rid_ = Handler;
}

//------------------------------------------------------------------------------

bool HandlerTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   stream << rc_ << spaces(4);
   DisplayEvent(stream, sid_, eid_);
   stream << " >> ";
   DisplayState(stream);
   return true;
}

//------------------------------------------------------------------------------

void HandlerTrace::DisplayState(ostream& stream) const
{
   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid_);

   if(svc != nullptr)
      stream << strClass(svc->GetState(stid_), false);
   else
      stream << "state=" << stid_;
}

//==============================================================================

SxpTrace::SxpTrace
   (ServiceId sid, const State& state, const Event& sxp, EventHandler::Rc rc) :
   HandlerTrace(sid, state, sxp, rc),
   curr_(0)
{
   rid_ = SxpEvent;

   switch(sxp.Eid())
   {
   case Event::AnalyzeSap:
      curr_ = static_cast< const AnalyzeSapEvent& >(sxp).CurrEvent()->Eid();
      break;
   case Event::AnalyzeSnp:
      curr_ = static_cast< const AnalyzeSnpEvent& >(sxp).CurrEvent()->Eid();
   }
}

//------------------------------------------------------------------------------

bool SxpTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   stream << rc_ << spaces(4);
   DisplayEvent(stream, sid_, eid_);

   stream << '(';
   DisplayEvent(stream, owner_, curr_);
   stream << ')';

   stream << " >> ";
   DisplayState(stream);
   return true;
}

//==============================================================================

SipTrace::SipTrace
   (ServiceId sid, const State& state, const Event& sip, EventHandler::Rc rc) :
   HandlerTrace(sid, state, sip, rc),
   mod_((static_cast< const InitiationReqEvent& >(sip)).GetModifier())
{
   rid_ = SipEvent;
}

//------------------------------------------------------------------------------

bool SipTrace::Display(ostream& stream, const string& opts)
{
   if(!SboTrace::Display(stream, opts)) return false;

   stream << rc_ << spaces(4);
   DisplayEvent(stream, sid_, eid_);

   stream << '(';
   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(mod_);
   if(svc != nullptr)
      stream << strClass(svc, false);
   else
      stream << "mod=" << mod_;
   stream << ')';

   stream << " >> ";
   DisplayState(stream);
   return true;
}
}
