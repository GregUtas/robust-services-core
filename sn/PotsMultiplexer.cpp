//==============================================================================
//
//  PotsMultiplexer.cpp
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
#include "PotsMultiplexer.h"
#include "CliText.h"
#include <ostream>
#include <string>
#include "Context.h"
#include "Debug.h"
#include "Formatters.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "MsgPort.h"
#include "NwTypes.h"
#include "PotsCircuit.h"
#include "PotsProfile.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsMuxFactoryText : public CliText
{
public:
   PotsMuxFactoryText();
};

class PotsMuxNull : public PotsMuxState
{
   friend class Singleton< PotsMuxNull >;
private:
   PotsMuxNull();
};

class PotsMuxPassive : public PotsMuxState
{
   friend class Singleton< PotsMuxPassive >;
private:
   PotsMuxPassive();
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
   PotsMuxEventHandler();
   virtual ~PotsMuxEventHandler();
};

class PotsMuxNuAnalyzeNetworkMessage : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxNuAnalyzeNetworkMessage >;
private:
   PotsMuxNuAnalyzeNetworkMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxNuInitiate : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxNuInitiate >;
private:
   PotsMuxNuInitiate() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxPaAnalyzeUserMessage : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxPaAnalyzeUserMessage >;
private:
   PotsMuxPaAnalyzeUserMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxPaAnalyzeNetworkMessage : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxPaAnalyzeNetworkMessage >;
private:
   PotsMuxPaAnalyzeNetworkMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsMuxPaRelay : public PotsMuxEventHandler
{
   friend class Singleton< PotsMuxPaRelay >;
private:
   PotsMuxPaRelay() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------

fixed_string PotsMuxFactoryStr = "PM";
fixed_string PotsMuxFactoryExpl = "POTS Multiplexer (network side)";

PotsMuxFactoryText::PotsMuxFactoryText() :
   CliText(PotsMuxFactoryExpl, PotsMuxFactoryStr) { }

//------------------------------------------------------------------------------

fn_name PotsMuxFactory_ctor = "PotsMuxFactory.ctor";

PotsMuxFactory::PotsMuxFactory() :
   SsmFactory(PotsMuxFactoryId, PotsProtocolId, "POTS Multiplexer")
{
   Debug::ft(PotsMuxFactory_ctor);

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

fn_name PotsMuxFactory_dtor = "PotsMuxFactory.dtor";

PotsMuxFactory::~PotsMuxFactory()
{
   Debug::ft(PotsMuxFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxFactory_AllocIcMsg = "PotsMuxFactory.AllocIcMsg";

Message* PotsMuxFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(PotsMuxFactory_AllocIcMsg);

   return new Pots_NU_Message(buff);
}

//------------------------------------------------------------------------------

fn_name PotsMuxFactory_AllocIcPsm = "PotsMuxFactory.AllocIcPsm";

ProtocolSM* PotsMuxFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft(PotsMuxFactory_AllocIcPsm);

   auto& pmsg = static_cast< const PotsMessage& >(msg);
   auto phi = pmsg.FindType< PotsHeaderInfo >(PotsParameter::Header);

   return new PotsMuxPsm(phi->port);
}

//------------------------------------------------------------------------------

fn_name PotsMuxFactory_AllocOgMsg = "PotsMuxFactory.AllocOgMsg";

Message* PotsMuxFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft(PotsMuxFactory_AllocOgMsg);

   return new Pots_UN_Message(nullptr, 12);
}

//------------------------------------------------------------------------------

fn_name PotsMuxFactory_AllocRoot = "PotsMuxFactory.AllocRoot";

RootServiceSM* PotsMuxFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft(PotsMuxFactory_AllocRoot);

   return new PotsMuxSsm(msg, psm);
}

//------------------------------------------------------------------------------

fn_name PotsMuxFactory_CreateText = "PotsMuxFactory.CreateText";

CliText* PotsMuxFactory::CreateText() const
{
   Debug::ft(PotsMuxFactory_CreateText);

   return new PotsMuxFactoryText;
}

//------------------------------------------------------------------------------

fn_name PotsMuxFactory_FindContext = "PotsMuxFactory.FindContext";

SsmContext* PotsMuxFactory::FindContext(const Message& msg) const
{
   Debug::ft(PotsMuxFactory_FindContext);

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

fn_name PotsMuxFactory_ReallocOgMsg = "PotsMuxFactory.ReallocOgMsg";

Message* PotsMuxFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(PotsMuxFactory_ReallocOgMsg);

   return new Pots_UN_Message(buff);
}

//==============================================================================

fn_name PotsMuxPsm_ctor1 = "PotsMuxPsm.ctor(first)";

PotsMuxPsm::PotsMuxPsm(Switch::PortId port) : MediaPsm(PotsMuxFactoryId),
   remSid_(NIL_ID),
   ogMsg_(nullptr),
   sendCause_(false)
{
   Debug::ft(PotsMuxPsm_ctor1);

   header_.port = port;
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_ctor2 = "PotsMuxPsm.ctor(subseq)";

PotsMuxPsm::PotsMuxPsm(ProtocolLayer& adj, bool upper, Switch::PortId port) :
   MediaPsm(PotsMuxFactoryId, adj, upper),
   remSid_(NIL_ID),
   ogMsg_(nullptr),
   sendCause_(false)
{
   Debug::ft(PotsMuxPsm_ctor2);

   header_.port = port;
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_dtor = "PotsMuxPsm.dtor";

PotsMuxPsm::~PotsMuxPsm()
{
   Debug::ft(PotsMuxPsm_dtor);
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

fn_name PotsMuxPsm_EnsureMediaMsg = "PotsMuxPsm.EnsureMediaMsg";

void PotsMuxPsm::EnsureMediaMsg()
{
   Debug::ft(PotsMuxPsm_EnsureMediaMsg);

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

fn_name PotsMuxPsm_ProcessIcMsg = "PotsMuxPsm.ProcessIcMsg";

ProtocolSM::IncomingRc PotsMuxPsm::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft(PotsMuxPsm_ProcessIcMsg);

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
      //
      //  If this is only a media update, do not raise the AnalyzeMsgEvent.
      {
         auto ppi = pmsg.FindType< ProgressInfo >(PotsParameter::Progress);
         if(ppi->progress == Progress::MediaUpdate) return DiscardMessage;
      }
      break;

   default:
      Context::Kill(PotsMuxPsm_ProcessIcMsg, sid, 0);
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
   msg.SetPriority(Message::Immediate);

   if(&msg != ogMsg_)
   {
      //  If we didn't create an outgoing message, we are relaying MSG.
      //  If we did create an outgoing message, generate a log: we're
      //  about to send two messages in a row, which is undesirable if
      //  not an outright error.
      //
      if(ogMsg_ != nullptr)
      {
         Debug::SwLog(PotsMuxPsm_ProcessOgMsg, msg.GetSignal(), 0);
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
      Debug::SwLog(PotsMuxPsm_ProcessOgMsg, header_.signal, 1);
      return PurgeMessage;
   }

   header_.signal = NIL_ID;
   MediaPsm::UpdateOgMedia(pmsg, PotsParameter::Media);

   //  If this message is the first in a dialog, it must provide the
   //  source and destination addresses.
   //
   if(AddressesUnknown(&msg))
   {
      auto host = IpPortRegistry::HostAddress();
      auto locAddr = GlobalAddress(host, NilIpPort, PotsMuxFactoryId);
      auto remAddr = GlobalAddress(host, NilIpPort, PotsCallFactoryId);

      msg.SetSender(locAddr);
      msg.SetReceiver(remAddr);
   }

   return SendMessage;
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_Route = "PotsMuxPsm.Route";

Message::Route PotsMuxPsm::Route() const
{
   Debug::ft(PotsMuxPsm_Route);

   return Message::Internal;
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_SendCause = "PotsMuxPsm.SendCause";

void PotsMuxPsm::SendCause(Cause::Ind cause)
{
   Debug::ft(PotsMuxPsm_SendCause);

   cause_.cause = cause;
   sendCause_ = true;
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_SendFacility = "PotsMuxPsm.SendFacility";

void PotsMuxPsm::SendFacility(ServiceId sid, Facility::Ind ind)
{
   Debug::ft(PotsMuxPsm_SendFacility);

   facility_.sid = sid;
   facility_.ind = ind;
}

void PotsMuxPsm::SendFacility(Facility::Ind ind)
{
   Debug::ft(PotsMuxPsm_SendFacility);

   SendFacility(remSid_, ind);
}

//------------------------------------------------------------------------------

fn_name PotsMuxPsm_SendFinalMsg = "PotsMuxPsm.SendFinalMsg";

void PotsMuxPsm::SendFinalMsg()
{
   Debug::ft(PotsMuxPsm_SendFinalMsg);

   if(GetState() == Idle) return;
   auto msg = new Pots_UN_Message(this, 20);
   if(msg == nullptr) return;

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
      Debug::SwLog(PotsMuxPsm_SendSignal, signal, 0);
   }
}

//==============================================================================

fixed_string PotsMuxInitiateEventStr = "PotsMuxInitiateEvent";
fixed_string PotsMuxRelayEventStr    = "PotsMuxRelayEvent";

//------------------------------------------------------------------------------

fn_name PotsMuxService_ctor = "PotsMuxService.ctor";

PotsMuxService::PotsMuxService() : Service(PotsMuxServiceId, true)
{
   Debug::ft(PotsMuxService_ctor);

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

fn_name PotsMuxService_dtor = "PotsMuxService.dtor";

PotsMuxService::~PotsMuxService()
{
   Debug::ft(PotsMuxService_dtor);
}

//==============================================================================

fn_name PotsMuxState_ctor = "PotsMuxState.ctor";

PotsMuxState::PotsMuxState(Id stid) : State(PotsMuxServiceId, stid)
{
   Debug::ft(PotsMuxState_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxState_dtor = "PotsMuxState.dtor";

PotsMuxState::~PotsMuxState()
{
   Debug::ft(PotsMuxState_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxNull_ctor = "PotsMuxNull.ctor";

PotsMuxNull::PotsMuxNull() : PotsMuxState(PotsMuxState::Null)
{
   Debug::ft(PotsMuxNull_ctor);

   BindMsgAnalyzer
      (PotsMuxEventHandler::NuAnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (PotsMuxEventHandler::NuInitiate, PotsMuxEvent::Initiate);
}

//------------------------------------------------------------------------------

fn_name PotsMuxPassive_ctor = "PotsMuxPassive.ctor";

PotsMuxPassive::PotsMuxPassive() : PotsMuxState(PotsMuxState::Passive)
{
   Debug::ft(PotsMuxPassive_ctor);

   BindMsgAnalyzer
      (PotsMuxEventHandler::PaAnalyzeUserMessage, Service::UserPort);
   BindMsgAnalyzer
      (PotsMuxEventHandler::PaAnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (PotsMuxEventHandler::PaRelay, PotsMuxEvent::Relay);
}

//==============================================================================

fn_name PotsMuxEvent_ctor = "PotsMuxEvent.ctor";

PotsMuxEvent::PotsMuxEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft(PotsMuxEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxEvent_dtor = "PotsMuxEvent.dtor";

PotsMuxEvent::~PotsMuxEvent()
{
   Debug::ft(PotsMuxEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxInitiateEvent_ctor = "PotsMuxInitiateEvent.ctor";

PotsMuxInitiateEvent::PotsMuxInitiateEvent(ServiceSM& owner) :
   PotsMuxEvent(Initiate, owner)
{
   Debug::ft(PotsMuxInitiateEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxInitiateEvent_dtor = "PotsMuxInitiateEvent.dtor";

PotsMuxInitiateEvent::~PotsMuxInitiateEvent()
{
   Debug::ft(PotsMuxInitiateEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxRelayEvent_ctor = "PotsMuxRelayEvent.ctor";

PotsMuxRelayEvent::PotsMuxRelayEvent(ServiceSM& owner) :
   PotsMuxEvent(Relay, owner)
{
   Debug::ft(PotsMuxRelayEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsMuxRelayEvent_dtor = "PotsMuxRelayEvent.dtor";

PotsMuxRelayEvent::~PotsMuxRelayEvent()
{
   Debug::ft(PotsMuxRelayEvent_dtor);
}

//==============================================================================

fn_name PotsMuxSsm_ctor = "PotsMuxSsm.ctor";

PotsMuxSsm::PotsMuxSsm(const Message& msg, ProtocolSM& psm) :
   MediaSsm(PotsMuxServiceId),
   prof_(nullptr),
   uPsm_(nullptr)
{
   Debug::ft(PotsMuxSsm_ctor);

   for(auto i = 0; i <= MaxCallId; ++i) nPsm_[i] = nullptr;

   auto& npsm = static_cast< PotsMuxPsm& >(psm);
   auto port = npsm.TsPort();
   auto tsw = Singleton< Switch >::Instance();
   auto cct = static_cast< PotsCircuit* >(tsw->GetCircuit(port));
   auto prof = cct->Profile();

   SetProfile(prof);
}

//------------------------------------------------------------------------------

fn_name PotsMuxSsm_dtor = "PotsMuxSsm.dtor";

PotsMuxSsm::~PotsMuxSsm()
{
   Debug::ft(PotsMuxSsm_dtor);

   if((uPsm_ != nullptr) && (prof_ != nullptr))
   {
      //  This occurs during error recovery, when PsmDeleted has yet to
      //  be invoked because the context is being cleaned up top-down.
      //
      prof_->ClearObjAddr(uPsm_);
   }
}

//------------------------------------------------------------------------------

fn_name PotsMuxSsm_CalcPort = "PotsMuxSsm.CalcPort";

ServicePortId PotsMuxSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsMuxSsm_CalcPort);

   auto psm = ame.Msg()->Psm();

   if(UPsm() == psm) return Service::UserPort;
   return Service::NetworkPort;
}

//------------------------------------------------------------------------------

fn_name PotsMuxSsm_CountCalls = "PotsMuxSsm.CountCalls";

size_t PotsMuxSsm::CountCalls() const
{
   Debug::ft(PotsMuxSsm_CountCalls);

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

fn_name PotsMuxSsm_Initiate = "PotsMuxSsm.Initiate";

EventHandler::Rc PotsMuxSsm::Initiate(Event*& nextEvent)
{
   Debug::ft(PotsMuxSsm_Initiate);

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

      Context::Kill(PotsMuxSsm_Initiate, pfi->sid, pfi->ind);
      return EventHandler::Suspend;
   }

   Context::Kill(PotsMuxSsm_Initiate, 0, 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsMuxSsm_NPsm = "PotsMuxSsm.NPsm";

PotsMuxPsm* PotsMuxSsm::NPsm() const
{
   Debug::ft(PotsMuxSsm_NPsm);

   if(nPsm_[0] == nullptr) return nPsm_[1];
   return nPsm_[0];
}

//------------------------------------------------------------------------------

fn_name PotsMuxSsm_PsmDeleted = "PotsMuxSsm.PsmDeleted";

void PotsMuxSsm::PsmDeleted(ProtocolSM& exPsm)
{
   Debug::ft(PotsMuxSsm_PsmDeleted);

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

fn_name PotsMuxSsm_RelayMsg = "PotsMuxSsm.RelayMsg";

EventHandler::Rc PotsMuxSsm::RelayMsg()
{
   Debug::ft(PotsMuxSsm_RelayMsg);

   auto pmsg = static_cast< PotsMessage* >(Context::ContextMsg());
   auto sid = pmsg->GetSignal();

   //  There should be exactly one call.
   //
   if(CountCalls() != 1)
   {
      Context::Kill(PotsMuxSsm_RelayMsg, sid, 0);
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
      Context::Kill(PotsMuxSsm_RelayMsg, sid, 1);
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
      Context::Kill(PotsMuxSsm_RelayMsg, sid, 0);
   }

   //d If our UPSM doesn't have addresses yet, supply them.  Don't pass PMSG to
   //  AddressesUnknown, because its remote factory is wrong (mux, not shelf).
   //
   if((ogPsm == uPsm_) && ogPsm->AddressesUnknown(nullptr))
   {
      auto host = IpPortRegistry::HostAddress();
      auto peer = IpPortRegistry::HostAddress();
      auto locAddr = GlobalAddress(host, PotsCallIpPort, PotsCallFactoryId);
      auto remAddr = GlobalAddress(peer, PotsShelfIpPort, PotsShelfFactoryId);

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
      Debug::SwLog(PotsMuxSsm_SetNPsm, cid, 0);
      return;
   }

   nPsm_[cid] = &psm;
}

//------------------------------------------------------------------------------

fn_name PotsMuxSsm_SetUPsm = "PotsMuxSsm.SetUPsm";

void PotsMuxSsm::SetUPsm(PotsCallPsm& psm)
{
   Debug::ft(PotsMuxSsm_SetUPsm);

   uPsm_ = &psm;
}

//------------------------------------------------------------------------------

PotsMuxEventHandler::PotsMuxEventHandler() { }

PotsMuxEventHandler::~PotsMuxEventHandler() { }

//------------------------------------------------------------------------------

fn_name PotsMuxNuAnalyzeNetworkMessage_ProcessEvent =
   "PotsMuxNuAnalyzeNetworkMessage.ProcessEvent";

EventHandler::Rc PotsMuxNuAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsMuxNuAnalyzeNetworkMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto pmsg = static_cast< Pots_NU_Message* >(ame.Msg());
   auto sid = pmsg->GetSignal();

   if(sid == PotsSignal::Facility)
   {
      nextEvent = new PotsMuxInitiateEvent(ssm);
      return Continue;
   }

   Context::Kill(PotsMuxNuAnalyzeNetworkMessage_ProcessEvent, sid, 0);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsMuxNuInitiate_ProcessEvent = "PotsMuxNuInitiate.ProcessEvent";

EventHandler::Rc PotsMuxNuInitiate::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsMuxNuInitiate_ProcessEvent);

   auto& mux = static_cast< PotsMuxSsm& >(ssm);

   return mux.Initiate(nextEvent);
}

//------------------------------------------------------------------------------

fn_name PotsMuxPaAnalyzeUserMessage_ProcessEvent =
   "PotsMuxPaAnalyzeUserMessage.ProcessEvent";

EventHandler::Rc PotsMuxPaAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsMuxPaAnalyzeUserMessage_ProcessEvent);

   nextEvent = new PotsMuxRelayEvent(ssm);
   return Continue;
}

//------------------------------------------------------------------------------

fn_name PotsMuxPaAnalyzeNetworkMessage_ProcessEvent =
   "PotsMuxPaAnalyzeNetworkMessage.ProcessEvent";

EventHandler::Rc PotsMuxPaAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsMuxPaAnalyzeNetworkMessage_ProcessEvent);

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

fn_name PotsMuxPaRelay_ProcessEvent = "PotsMuxPaRelay.ProcessEvent";

EventHandler::Rc PotsMuxPaRelay::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsMuxPaRelay_ProcessEvent);

   auto& mux = static_cast< PotsMuxSsm& >(ssm);

   return mux.RelayMsg();
}
}
