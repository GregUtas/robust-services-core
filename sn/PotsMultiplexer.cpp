//==============================================================================
//
//  PotsMultiplexer.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "PotsMultiplexer.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "CliText.h"
#include "Context.h"
#include "Debug.h"
#include "Formatters.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "LocalAddress.h"
#include "MsgPort.h"
#include "NwTypes.h"
#include "PotsCircuit.h"
#include "PotsProfile.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NetworkBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsMuxNull : public PotsMuxState
{
   friend class Singleton< PotsMuxNull >;

   PotsMuxNull();
   ~PotsMuxNull() = default;
};

class PotsMuxPassive : public PotsMuxState
{
   friend class Singleton< PotsMuxPassive >;

   PotsMuxPassive();
   ~PotsMuxPassive() = default;
};

class PotsMuxInitiateEvent : public PotsMuxEvent
{
public:
   explicit PotsMuxInitiateEvent(ServiceSM& owner);
   ~PotsMuxInitiateEvent();
};

class PotsMuxEventHandler : public EventHandler
{
public:
   static const Id NuAnalyzeNetworkMessage = NextId + 0;
   static const Id NuInitiate              = NextId + 1;
   static const Id PaAnalyzeUserMessage    = NextId + 2;
   static const Id PaAnalyzeNetworkMessage = NextId + 3;
   static const Id PaRelay                 = NextId + 4;
protected:
   PotsMuxEventHandler() = default;
   virtual ~PotsMuxEventHandler() = default;
};

class PotsMuxNuAnalyzeNetworkMessage : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxNuAnalyzeNetworkMessage >;

   PotsMuxNuAnalyzeNetworkMessage() = default;
   ~PotsMuxNuAnalyzeNetworkMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxNuInitiate : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxNuInitiate >;

   PotsMuxNuInitiate() = default;
   ~PotsMuxNuInitiate() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxPaAnalyzeUserMessage : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxPaAnalyzeUserMessage >;

   PotsMuxPaAnalyzeUserMessage() = default;
   ~PotsMuxPaAnalyzeUserMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxPaAnalyzeNetworkMessage : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxPaAnalyzeNetworkMessage >;

   PotsMuxPaAnalyzeNetworkMessage() = default;
   ~PotsMuxPaAnalyzeNetworkMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxPaRelay : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxPaRelay >;

   PotsMuxPaRelay() = default;
   ~PotsMuxPaRelay() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------

PotsMuxFactory::PotsMuxFactory() :
   SsmFactory(PotsMuxFactoryId, PotsProtocolId, "POTS Multiplexer")
{
   Debug::ft("PotsMuxFactory.ctor");

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(PotsSignal::Supervise);
   AddIncomingSignal(PotsSignal::Lockout);
   AddIncomingSignal(PotsSignal::Release);
   AddIncomingSignal(PotsSignal::Facility);
   AddIncomingSignal(PotsSignal::Progress);

   AddOutgoingSignal(PotsSignal::Offhook);
   AddOutgoingSignal(PotsSignal::Alerting);
   AddOutgoingSignal(PotsSignal::Digits);
   AddOutgoingSignal(PotsSignal::Flash);
   AddOutgoingSignal(PotsSignal::Onhook);
   AddOutgoingSignal(PotsSignal::Facility);
   AddOutgoingSignal(PotsSignal::Progress);
   AddOutgoingSignal(PotsSignal::Release);
}

//------------------------------------------------------------------------------

PotsMuxFactory::~PotsMuxFactory()
{
   Debug::ftnt("PotsMuxFactory.dtor");
}

//------------------------------------------------------------------------------

Message* PotsMuxFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("PotsMuxFactory.AllocIcMsg");

   return new Pots_NU_Message(buff);
}

//------------------------------------------------------------------------------

ProtocolSM* PotsMuxFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft("PotsMuxFactory.AllocIcPsm");

   auto& pmsg = static_cast< const PotsMessage& >(msg);
   auto phi = pmsg.FindType< PotsHeaderInfo >(PotsParameter::Header);

   return new PotsMuxPsm(phi->port);
}

//------------------------------------------------------------------------------

Message* PotsMuxFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft("PotsMuxFactory.AllocOgMsg");

   return new Pots_UN_Message(nullptr, 12);
}

//------------------------------------------------------------------------------

RootServiceSM* PotsMuxFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft("PotsMuxFactory.AllocRoot");

   return new PotsMuxSsm(msg, psm);
}

//------------------------------------------------------------------------------

fixed_string PotsMuxFactoryStr = "PM";
fixed_string PotsMuxFactoryExpl = "POTS Multiplexer (network side)";

CliText* PotsMuxFactory::CreateText() const
{
   Debug::ft("PotsMuxFactory.CreateText");

   return new CliText(PotsMuxFactoryExpl, PotsMuxFactoryStr);
}

//------------------------------------------------------------------------------

SsmContext* PotsMuxFactory::FindContext(const Message& msg) const
{
   Debug::ft("PotsMuxFactory.FindContext");

   //  Find the root SSM for this POTS subscriber.  If it's the POTS
   //  multiplexer, then join its context.
   //
   auto& pmsg = static_cast< const PotsMessage& >(msg);
   auto phi = pmsg.FindType< PotsHeaderInfo >(PotsParameter::Header);
   auto tsw = Singleton< Switch >::Instance();
   auto cct = static_cast< PotsCircuit* >(tsw->GetCircuit(phi->port));
   auto prof = cct->Profile();
   auto addr = prof->ObjAddr();

   auto port = MsgPort::Find(addr);
   if(port == nullptr) return nullptr;

   auto psm = port->UppermostPsm();
   if(psm == nullptr) return nullptr;

   auto ssm = psm->RootSsm();
   if(ssm == nullptr) return nullptr;

   if(ssm->Sid() == PotsMuxServiceId) return ssm->GetContext();
   return nullptr;
}

//------------------------------------------------------------------------------

Message* PotsMuxFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("PotsMuxFactory.ReallocOgMsg");

   return new Pots_UN_Message(buff);
}

//==============================================================================

PotsMuxPsm::PotsMuxPsm(Switch::PortId port) : MediaPsm(PotsMuxFactoryId),
   remSid_(NIL_ID),
   ogMsg_(nullptr),
   sendCause_(false)
{
   Debug::ft("PotsMuxPsm.ctor(first)");

   header_.port = port;
}

//------------------------------------------------------------------------------

PotsMuxPsm::PotsMuxPsm(ProtocolLayer& adj, bool upper, Switch::PortId port) :
   MediaPsm(PotsMuxFactoryId, adj, upper),
   remSid_(NIL_ID),
   ogMsg_(nullptr),
   sendCause_(false)
{
   Debug::ft("PotsMuxPsm.ctor(subseq)");

   header_.port = port;
}

//------------------------------------------------------------------------------

PotsMuxPsm::~PotsMuxPsm()
{
   Debug::ftnt("PotsMuxPsm.dtor");
}

//------------------------------------------------------------------------------

void PotsMuxPsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MediaPsm::Display(stream, prefix, options);

   stream << prefix << "remSid        : " << remSid_ << CRLF;
   stream << prefix << "ogMsg         : " << ogMsg_ << CRLF;
   stream << prefix << "sendCause : " << sendCause_ << CRLF;
   stream << prefix << "header : " << CRLF;
   header_.Display(stream, prefix + spaces(2));
   stream << prefix << "facility : " << CRLF;
   facility_.Display(stream, prefix + spaces(2));
   stream << prefix << "progress : " << CRLF;
   progress_.Display(stream, prefix + spaces(2));
   stream << prefix << "cause : " << CRLF;
   cause_.Display(stream, prefix + spaces(2));
}

//------------------------------------------------------------------------------

void PotsMuxPsm::EnsureMediaMsg()
{
   Debug::ft("PotsMuxPsm.EnsureMediaMsg");

   //  A media update can be included in any message, so an outgoing
   //  message only needs to be created if one doesn't already exist.
   //
   if((FirstOgMsg() == nullptr) && (GetState() != Idle))
   {
      progress_.progress = Progress::MediaUpdate;
      SendSignal(PotsSignal::Progress);
   }
}

//------------------------------------------------------------------------------

ProtocolSM::IncomingRc PotsMuxPsm::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft("PotsMuxPsm.ProcessIcMsg");

   auto& pmsg = static_cast< Pots_UN_Message& >(msg);
   auto sid = pmsg.GetSignal();

   MediaPsm::UpdateIcMedia(pmsg, PotsParameter::Media);

   switch(sid)
   {
   case PotsSignal::Lockout:
   case PotsSignal::Release:
      SetState(Idle);
      break;

   case PotsSignal::Facility:
      //
      //  This is the first incoming message to a new multiplexer NPSM.
      //
      SetState(Active);
      break;

   case Signal::Timeout:
   case PotsSignal::Supervise:
      break;

   case PotsSignal::Progress:
   {
      //  If this is only a media update, do not raise the AnalyzeMsgEvent.
      //
      auto ppi = pmsg.FindType< ProgressInfo >(PotsParameter::Progress);
      if(ppi->progress == Progress::MediaUpdate) return DiscardMessage;
      break;
   }

   default:
      Context::Kill("invalid signal", sid);
      return DiscardMessage;
   }

   event = new AnalyzeMsgEvent(msg);
   return EventRaised;
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_ProcessOgMsg = "PotsMuxPsm.ProcessOgMsg";

ProtocolSM::OutgoingRc PotsMuxPsm::ProcessOgMsg(Message& msg)
{
   Debug::ft(PotsMuxPsm_ProcessOgMsg);

   SetState(Active);

   //  Send all messages from the multiplexer NPSM with immediate priority.
   //
   auto& pmsg = static_cast< Pots_UN_Message& >(msg);
   msg.SetPriority(IMMEDIATE);

   if(&msg != ogMsg_)
   {
      //  If we didn't create an outgoing message, we are relaying MSG.
      //  If we did create an outgoing message, generate a log: we're
      //  about to send two messages in a row, which is undesirable if
      //  not an outright error.
      //
      if(ogMsg_ != nullptr)
      {
         Debug::SwLog(PotsMuxPsm_ProcessOgMsg,
            "sending second message", msg.GetSignal());
      }

      return SendMessage;
   }

   ogMsg_ = nullptr;

   switch(header_.signal)
   {
   case PotsSignal::Alerting:
   case PotsSignal::Offhook:
      pmsg.AddHeader(header_);
      break;

   case PotsSignal::Facility:
      pmsg.AddHeader(header_);
      pmsg.AddFacility(facility_);

      if(sendCause_)
      {
         pmsg.AddCause(cause_);
         sendCause_ = false;
         cause_.cause = Cause::NilInd;
      }
      break;

   case PotsSignal::Progress:
      pmsg.AddHeader(header_);
      pmsg.AddProgress(progress_);
      break;

   case PotsSignal::Release:
      pmsg.AddHeader(header_);
      pmsg.AddCause(cause_);
      SetState(Idle);
      return SendMessage;

   default:
      //
      //  Other signals should only occur when being relayed (see SetSignal).
      //
      Debug::SwLog(PotsMuxPsm_ProcessOgMsg,
         "unexpected signal", header_.signal);
      return PurgeMessage;
   }

   header_.signal = NIL_ID;
   MediaPsm::UpdateOgMedia(pmsg, PotsParameter::Media);

   //  If this message is the first in a dialog, it must provide the
   //  source and destination addresses.
   //
   if(AddressesUnknown(&msg))
   {
      auto& host = IpPortRegistry::HostAddress();
      GlobalAddress locAddr(host, NilIpPort, PotsMuxFactoryId);
      GlobalAddress remAddr(host, NilIpPort, PotsCallFactoryId);

      msg.SetSender(locAddr);
      msg.SetReceiver(remAddr);
   }

   return SendMessage;
}

//------------------------------------------------------------------------------

Message::Route PotsMuxPsm::Route() const
{
   Debug::ft("PotsMuxPsm.Route");

   return Message::Internal;
}

//------------------------------------------------------------------------------

void PotsMuxPsm::SendCause(Cause::Ind cause)
{
   Debug::ft("PotsMuxPsm.SendCause");

   cause_.cause = cause;
   sendCause_ = true;
}

//------------------------------------------------------------------------------

void PotsMuxPsm::SendFacility(ServiceId sid, Facility::Ind ind)
{
   Debug::ft("PotsMuxPsm.SendFacility(sid)");

   facility_.sid = sid;
   facility_.ind = ind;
}

void PotsMuxPsm::SendFacility(Facility::Ind ind)
{
   Debug::ft("PotsMuxPsm.SendFacility");

   SendFacility(remSid_, ind);
}

//------------------------------------------------------------------------------

void PotsMuxPsm::SendFinalMsg()
{
   Debug::ft("PotsMuxPsm.SendFinalMsg");

   if(GetState() == Idle) return;
   auto msg = new Pots_UN_Message(this, 20);

   header_.signal = PotsSignal::Release;
   cause_.cause = Cause::TemporaryFailure;
   msg->AddHeader(header_);
   msg->AddCause(cause_);
   SendToLower(*msg);
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_SendSignal = "PotsMuxPsm.SendSignal";

void PotsMuxPsm::SendSignal(PotsSignal::Id signal)
{
   Debug::ft(PotsMuxPsm_SendSignal);

   if(ogMsg_ == nullptr)
   {
      ogMsg_ = new Pots_UN_Message(this, 16);
   }

   switch(signal)
   {
   case PotsSignal::Progress:
      //
      //  Any other signal takes precedence.  But if there isn't one,
      //  continue on and send this as the signal.
      //
      if(header_.signal != NIL_ID) return;
      //  [[fallthrough]]
   case PotsSignal::Offhook:
   case PotsSignal::Alerting:
   case PotsSignal::Facility:
   case PotsSignal::Release:
      header_.signal = signal;
      break;

   default:
      //
      //  Other messages are relayed instead of being built explicitly.
      //
      Debug::SwLog(PotsMuxPsm_SendSignal, "unexpected signal", signal);
   }
}

//==============================================================================

fixed_string PotsMuxInitiateEventStr = "PotsMuxInitiateEvent";
fixed_string PotsMuxRelayEventStr    = "PotsMuxRelayEvent";

//------------------------------------------------------------------------------

PotsMuxService::PotsMuxService() : Service(PotsMuxServiceId, true)
{
   Debug::ft("PotsMuxService.ctor");

   Singleton< PotsMuxNull >::Instance();
   Singleton< PotsMuxPassive >::Instance();

   BindHandler(*Singleton< PotsMuxNuAnalyzeNetworkMessage >::Instance(),
      PotsMuxEventHandler::NuAnalyzeNetworkMessage);
   BindHandler(*Singleton< PotsMuxNuInitiate >::Instance(),
      PotsMuxEventHandler::NuInitiate);

   BindHandler(*Singleton< PotsMuxPaAnalyzeUserMessage >::Instance(),
      PotsMuxEventHandler::PaAnalyzeUserMessage);
   BindHandler(*Singleton< PotsMuxPaAnalyzeNetworkMessage >::Instance(),
      PotsMuxEventHandler::PaAnalyzeNetworkMessage);
   BindHandler(*Singleton< PotsMuxPaRelay >::Instance(),
      PotsMuxEventHandler::PaRelay);

   BindEventName(PotsMuxInitiateEventStr, PotsMuxEvent::Initiate);
   BindEventName(PotsMuxRelayEventStr, PotsMuxEvent::Relay);
}

//------------------------------------------------------------------------------

PotsMuxService::~PotsMuxService()
{
   Debug::ftnt("PotsMuxService.dtor");
}

//==============================================================================

PotsMuxState::PotsMuxState(Id stid) : State(PotsMuxServiceId, stid)
{
   Debug::ft("PotsMuxState.ctor");
}

//------------------------------------------------------------------------------

PotsMuxState::~PotsMuxState()
{
   Debug::ftnt("PotsMuxState.dtor");
}

//------------------------------------------------------------------------------

PotsMuxNull::PotsMuxNull() : PotsMuxState(PotsMuxState::Null)
{
   Debug::ft("PotsMuxNull.ctor");

   BindMsgAnalyzer
      (PotsMuxEventHandler::NuAnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (PotsMuxEventHandler::NuInitiate, PotsMuxEvent::Initiate);
}

//------------------------------------------------------------------------------

PotsMuxPassive::PotsMuxPassive() : PotsMuxState(PotsMuxState::Passive)
{
   Debug::ft("PotsMuxPassive.ctor");

   BindMsgAnalyzer
      (PotsMuxEventHandler::PaAnalyzeUserMessage, Service::UserPort);
   BindMsgAnalyzer
      (PotsMuxEventHandler::PaAnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (PotsMuxEventHandler::PaRelay, PotsMuxEvent::Relay);
}

//==============================================================================

PotsMuxEvent::PotsMuxEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft("PotsMuxEvent.ctor");
}

//------------------------------------------------------------------------------

PotsMuxEvent::~PotsMuxEvent()
{
   Debug::ftnt("PotsMuxEvent.dtor");
}

//------------------------------------------------------------------------------

PotsMuxInitiateEvent::PotsMuxInitiateEvent(ServiceSM& owner) :
   PotsMuxEvent(Initiate, owner)
{
   Debug::ft("PotsMuxInitiateEvent.ctor");
}

//------------------------------------------------------------------------------

PotsMuxInitiateEvent::~PotsMuxInitiateEvent()
{
   Debug::ftnt("PotsMuxInitiateEvent.dtor");
}

//------------------------------------------------------------------------------

PotsMuxRelayEvent::PotsMuxRelayEvent(ServiceSM& owner) :
   PotsMuxEvent(Relay, owner)
{
   Debug::ft("PotsMuxRelayEvent.ctor");
}

//------------------------------------------------------------------------------

PotsMuxRelayEvent::~PotsMuxRelayEvent()
{
   Debug::ftnt("PotsMuxRelayEvent.dtor");
}

//==============================================================================

PotsMuxSsm::PotsMuxSsm(const Message& msg, ProtocolSM& psm) :
   MediaSsm(PotsMuxServiceId),
   prof_(nullptr),
   uPsm_(nullptr)
{
   Debug::ft("PotsMuxSsm.ctor");

   for(auto i = 0; i <= MaxCallId; ++i) nPsm_[i] = nullptr;

   auto& npsm = static_cast< PotsMuxPsm& >(psm);
   auto port = npsm.TsPort();
   auto tsw = Singleton< Switch >::Instance();
   auto cct = static_cast< PotsCircuit* >(tsw->GetCircuit(port));
   auto prof = cct->Profile();

   SetProfile(prof);
}

//------------------------------------------------------------------------------

PotsMuxSsm::~PotsMuxSsm()
{
   Debug::ftnt("PotsMuxSsm.dtor");

   if((uPsm_ != nullptr) && (prof_ != nullptr))
   {
      //  This occurs during error recovery, when PsmDeleted has yet to
      //  be invoked because the context is being cleaned up top-down.
      //
      prof_->ClearObjAddr(uPsm_);
   }
}

//------------------------------------------------------------------------------

ServicePortId PotsMuxSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsMuxSsm.CalcPort");

   auto psm = ame.Msg()->Psm();

   if(UPsm() == psm) return Service::UserPort;
   return Service::NetworkPort;
}

//------------------------------------------------------------------------------

size_t PotsMuxSsm::CountCalls() const
{
   Debug::ft("PotsMuxSsm.CountCalls");

   size_t n = 0;

   if(nPsm_[0] != nullptr) ++n;
   if(nPsm_[1] != nullptr) ++n;
   return n;
}

//------------------------------------------------------------------------------

void PotsMuxSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ServiceSM::Display(stream, prefix, options);

   stream << prefix << "prof    : " << prof_ << CRLF;
   stream << prefix << "uPsm    : " << uPsm_ << CRLF;
   stream << prefix << "nPsm[0] : " << nPsm_[0] << CRLF;
   stream << prefix << "nPsm[1] : " << nPsm_[1] << CRLF;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsMuxSsm::Initiate(Event*& nextEvent)
{
   Debug::ft("PotsMuxSsm.Initiate");

   auto pmsg = static_cast< PotsMessage* >(Context::ContextMsg());
   auto pfi = pmsg->FindType< PotsFacilityInfo >(PotsParameter::Facility);

   if(pfi != nullptr)
   {
      if(pfi->ind == Facility::InitiationReq)
      {
         nextEvent = new InitiationReqEvent(*this, pfi->sid);
         SetNextState(PotsMuxState::Passive);
         return EventHandler::Initiate;
      }

      Context::Kill("invalid facility indicator", pack2(pfi->sid, pfi->ind));
      return EventHandler::Suspend;
   }

   Context::Kill("facility parameter not found", 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

PotsMuxPsm* PotsMuxSsm::NPsm() const
{
   Debug::ft("PotsMuxSsm.NPsm");

   if(nPsm_[0] == nullptr) return nPsm_[1];
   return nPsm_[0];
}

//------------------------------------------------------------------------------

void PotsMuxSsm::PsmDeleted(ProtocolSM& exPsm)
{
   Debug::ft("PotsMuxSsm.PsmDeleted");

   if(uPsm_ == &exPsm)
   {
      prof_->ClearObjAddr(uPsm_);
      uPsm_ = nullptr;
   }
   else
   {
      for(auto i = 0; i <= MaxCallId; ++i)
      {
         if(nPsm_[i] == &exPsm)
         {
            nPsm_[i] = nullptr;
            break;
         }
      }
   }

   MediaSsm::PsmDeleted(exPsm);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsMuxSsm::RelayMsg()
{
   Debug::ft("PotsMuxSsm.RelayMsg");

   auto pmsg = static_cast< PotsMessage* >(Context::ContextMsg());
   auto sid = pmsg->GetSignal();

   //  There should be exactly one call.
   //
   if(CountCalls() != 1)
   {
      Context::Kill("invalid call count", CountCalls());
   }

   switch(sid)
   {
   case PotsSignal::Onhook:
   case PotsSignal::Offhook:
   case PotsSignal::Flash:
   case PotsSignal::Supervise:
      break;

   case PotsSignal::Lockout:
   case PotsSignal::Release:
      if(sid == PotsSignal::Release)
         prof_->SetState(uPsm_, PotsProfile::Idle);
      else
         prof_->SetState(uPsm_, PotsProfile::Lockout);

      SetNextState(Null);
      break;

   default:
      Context::Kill("invalid signal", sid);
      return EventHandler::Suspend;
   }

   auto icPsm = pmsg->Psm();
   ProtocolSM* ogPsm;

   if(icPsm == uPsm_)
      ogPsm = NPsm();
   else
      ogPsm = uPsm_;

   if(!pmsg->Relay(*ogPsm))
   {
      Context::Kill("failed to relay message", sid);
   }

   //d If our UPSM doesn't have addresses yet, supply them.  Don't pass PMSG to
   //  AddressesUnknown, because its remote factory is wrong (mux, not shelf).
   //
   if((ogPsm == uPsm_) && ogPsm->AddressesUnknown(nullptr))
   {
      auto& host = IpPortRegistry::HostAddress();
      auto& peer = IpPortRegistry::HostAddress();
      GlobalAddress locAddr(host, PotsCallIpPort, PotsCallFactoryId);
      GlobalAddress remAddr(peer, PotsShelfIpPort, PotsShelfFactoryId);

      pmsg->SetSender(locAddr);
      pmsg->SetReceiver(remAddr);
   }

   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsMuxSsm_SetNPsm = "PotsMuxSsm.SetNPsm";

void PotsMuxSsm::SetNPsm(CallId cid, PotsMuxPsm& psm)
{
   Debug::ft(PotsMuxSsm_SetNPsm);

   if(nPsm_[cid] != nullptr)
   {
      Debug::SwLog(PotsMuxSsm_SetNPsm, "nPSM already exists", cid);
      return;
   }

   nPsm_[cid] = &psm;
}

//------------------------------------------------------------------------------

void PotsMuxSsm::SetUPsm(PotsCallPsm& psm)
{
   Debug::ft("PotsMuxSsm.SetUPsm");

   uPsm_ = &psm;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsMuxNuAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsMuxNuAnalyzeNetworkMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto pmsg = static_cast< Pots_NU_Message* >(ame.Msg());
   auto sid = pmsg->GetSignal();

   if(sid == PotsSignal::Facility)
   {
      nextEvent = new PotsMuxInitiateEvent(ssm);
      return Continue;
   }

   Context::Kill("invalid signal", sid);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsMuxNuInitiate::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsMuxNuInitiate.ProcessEvent");

   auto& mux = static_cast< PotsMuxSsm& >(ssm);

   return mux.Initiate(nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsMuxPaAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsMuxPaAnalyzeUserMessage.ProcessEvent");

   nextEvent = new PotsMuxRelayEvent(ssm);
   return Continue;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsMuxPaAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsMuxPaAnalyzeNetworkMessage.ProcessEvent");

   //  Message received from NPSM while in Passive state.
   //
   auto& mux = static_cast< PotsMuxSsm& >(ssm);
   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();

   //  Relay anything other than a Facility message.
   //
   if(sid == PotsSignal::Facility)
   {
      return mux.Initiate(nextEvent);
   }

   nextEvent = new PotsMuxRelayEvent(mux);
   return Continue;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsMuxPaRelay::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsMuxPaRelay.ProcessEvent");

   auto& mux = static_cast< PotsMuxSsm& >(ssm);

   return mux.RelayMsg();
}
}
