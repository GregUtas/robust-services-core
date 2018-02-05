//==============================================================================
//
//  PotsWmlService.cpp
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
#include "PotsWmlService.h"
#include "Event.h"
#include "EventHandler.h"
#include "ServiceSM.h"
#include "State.h"
#include <ostream>
#include <string>
#include "BcAddress.h"
#include "BcCause.h"
#include "BcRouting.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "Parameter.h"
#include "PotsFeatures.h"
#include "PotsProfile.h"
#include "PotsProtocol.h"
#include "PotsSessions.h"
#include "PotsWmlFeature.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Signal.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TimerProtocol.h"
#include "TlvMessage.h"
#include "Tones.h"

using namespace CallBase;
using namespace MediaBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsWmlState : public State
{
public:
   static const Id FWMLS = ServiceSM::Null;

   static const Id Null       = FWMLS + 0;  // just created
   static const Id Activating = FWMLS + 1;  // programming new target DN
   static const Id Timing     = FWMLS + 2;  // will auto-dial on timeout
protected:
   explicit PotsWmlState(Id stid);
   virtual ~PotsWmlState();
};

class PotsWmlNull : public PotsWmlState
{
   friend class Singleton< PotsWmlNull >;
private:
   PotsWmlNull();
};

class PotsWmlActivating : public PotsWmlState
{
   friend class Singleton< PotsWmlActivating >;
private:
   PotsWmlActivating();
};

class PotsWmlTiming : public PotsWmlState
{
   friend class Singleton< PotsWmlTiming >;
private:
   PotsWmlTiming();
};

class PotsWmlEvent : public Event
{
public:
   static const Id Timeout = NextId + 0;
   virtual ~PotsWmlEvent();
protected:
   PotsWmlEvent(Id eid, ServiceSM& owner);
};

class PotsWmlTimeoutEvent : public PotsWmlEvent
{
public:
   explicit PotsWmlTimeoutEvent(ServiceSM& owner);
   ~PotsWmlTimeoutEvent();
};

class PotsWmlEventHandler : public EventHandler
{
public:
   static const Id AcAnalyzeMessage = NextId + 0;
   static const Id TiAnalyzeMessage = NextId + 1;
   static const Id TiTimeout        = NextId + 2;
protected:
   PotsWmlEventHandler();
   virtual ~PotsWmlEventHandler();
};

class PotsWmlAcAnalyzeMessage : public PotsWmlEventHandler
{
   friend class Singleton< PotsWmlAcAnalyzeMessage >;
private:
   PotsWmlAcAnalyzeMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsWmlTiAnalyzeMessage : public PotsWmlEventHandler
{
   friend class Singleton< PotsWmlTiAnalyzeMessage >;
private:
   PotsWmlTiAnalyzeMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsWmlTiTimeout : public PotsWmlEventHandler
{
   friend class Singleton< PotsWmlTiTimeout >;
private:
   PotsWmlTiTimeout() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsWmlSsm : public ServiceSM
{
public:
   PotsWmlSsm();
   ~PotsWmlSsm();
   PotsWmlFeatureProfile* Profile() const { return wmlp_; }
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   void Cancel();
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   virtual EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessSap
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessSip
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessSnp
      (Event& currEvent, Event*& nextEvent) override;

   PotsWmlFeatureProfile* wmlp_;
   bool timer_;
};

//==============================================================================

fn_name PotsWmlInitiator_ctor = "PotsWmlInitiator.ctor";

PotsWmlInitiator::PotsWmlInitiator() : Initiator(PotsWmlServiceId,
   PotsCallServiceId, BcTrigger::CollectInformationSap,
   PotsCollectInformationSap::PotsWmlPriority)
{
   Debug::ft(PotsWmlInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlInitiator_ProcessEvent = "PotsWmlInitiator.ProcessEvent";

EventHandler::Rc PotsWmlInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsWmlInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();
   auto wmlp = static_cast< PotsWmlFeatureProfile* >(prof->FindFeature(WML));

   if((wmlp != nullptr) && wmlp->IsActive() && pssm.DialedDigits().Empty())
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsWmlServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

fixed_string PotsWmlTimeoutEventStr = "PotsWmlTimeoutEvent";

fn_name PotsWmlService_ctor = "PotsWmlService.ctor";

PotsWmlService::PotsWmlService() : Service(PotsWmlServiceId, false, true)
{
   Debug::ft(PotsWmlService_ctor);

   Singleton< PotsWmlNull >::Instance();
   Singleton< PotsWmlActivating >::Instance();
   Singleton< PotsWmlTiming >::Instance();

   BindHandler(*Singleton< PotsWmlAcAnalyzeMessage >::Instance(),
      PotsWmlEventHandler::AcAnalyzeMessage);
   BindHandler(*Singleton< PotsWmlTiAnalyzeMessage >::Instance(),
      PotsWmlEventHandler::TiAnalyzeMessage);
   BindHandler(*Singleton< PotsWmlTiTimeout >::Instance(),
      PotsWmlEventHandler::TiTimeout);

   BindEventName(PotsWmlTimeoutEventStr, PotsWmlEvent::Timeout);
}

//------------------------------------------------------------------------------

fn_name PotsWmlService_dtor = "PotsWmlService.dtor";

PotsWmlService::~PotsWmlService()
{
   Debug::ft(PotsWmlService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlService_AllocModifier = "PotsWmlService.AllocModifier";

ServiceSM* PotsWmlService::AllocModifier() const
{
   Debug::ft(PotsWmlService_AllocModifier);

   return new PotsWmlSsm;
}

//==============================================================================

fn_name PotsWmlActivate_ctor = "PotsWmlActivate.ctor";

PotsWmlActivate::PotsWmlActivate() : Service(PotsWmlActivation, false, true)
{
   Debug::ft(PotsWmlActivate_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlActivate_dtor = "PotsWmlActivate.dtor";

PotsWmlActivate::~PotsWmlActivate()
{
   Debug::ft(PotsWmlActivate_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlActivate_AllocModifier = "PotsWmlActivate.AllocModifier";

ServiceSM* PotsWmlActivate::AllocModifier() const
{
   Debug::ft(PotsWmlActivate_AllocModifier);

   return new PotsWmlSsm;
}

//==============================================================================

fn_name PotsWmlDeactivate_ctor = "PotsWmlDeactivate.ctor";

PotsWmlDeactivate::PotsWmlDeactivate() :
   Service(PotsWmlDeactivation, false, true)
{
   Debug::ft(PotsWmlDeactivate_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlDeactivate_dtor = "PotsWmlDeactivate.dtor";

PotsWmlDeactivate::~PotsWmlDeactivate()
{
   Debug::ft(PotsWmlDeactivate_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlDeactivate_AllocModifier = "PotsWmlDeactivate.AllocModifier";

ServiceSM* PotsWmlDeactivate::AllocModifier() const
{
   Debug::ft(PotsWmlDeactivate_AllocModifier);

   return new PotsWmlSsm;
}

//==============================================================================

fn_name PotsWmlEvent_ctor = "PotsWmlEvent.ctor";

PotsWmlEvent::PotsWmlEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft(PotsWmlEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlEvent_dtor = "PotsWmlEvent.dtor";

PotsWmlEvent::~PotsWmlEvent()
{
   Debug::ft(PotsWmlEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlTimeoutEvent_ctor = "PotsWmlTimeoutEvent.ctor";

PotsWmlTimeoutEvent::PotsWmlTimeoutEvent(ServiceSM& owner) :
   PotsWmlEvent(Timeout, owner)
{
   Debug::ft(PotsWmlTimeoutEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlTimeoutEvent_dtor = "PotsWmlTimeoutEvent.dtor";

PotsWmlTimeoutEvent::~PotsWmlTimeoutEvent()
{
   Debug::ft(PotsWmlTimeoutEvent_dtor);
}

//==============================================================================

fn_name PotsWmlState_ctor = "PotsWmlState.ctor";

PotsWmlState::PotsWmlState(Id stid) : State(PotsWmlServiceId, stid)
{
   Debug::ft(PotsWmlState_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlState_dtor = "PotsWmlState.dtor";

PotsWmlState::~PotsWmlState()
{
   Debug::ft(PotsWmlState_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlNull_ctor = "PotsWmlNull.ctor";

PotsWmlNull::PotsWmlNull() : PotsWmlState(PotsWmlState::Null)
{
   Debug::ft(PotsWmlNull_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlActivating_ctor = "PotsWmlActivating.ctor";

PotsWmlActivating::PotsWmlActivating() : PotsWmlState(PotsWmlState::Activating)
{
   Debug::ft(PotsWmlActivating_ctor);

   BindMsgAnalyzer
      (PotsWmlEventHandler::AcAnalyzeMessage, Service::UserPort);
}

//------------------------------------------------------------------------------

fn_name PotsWmlTiming_ctor = "PotsWmlTiming.ctor";

PotsWmlTiming::PotsWmlTiming() : PotsWmlState(PotsWmlState::Timing)
{
   Debug::ft(PotsWmlTiming_ctor);

   BindMsgAnalyzer
      (PotsWmlEventHandler::TiAnalyzeMessage, Service::UserPort);
   BindEventHandler
      (PotsWmlEventHandler::TiTimeout, PotsWmlEvent::Timeout);
}

//==============================================================================

fn_name PotsWmlSsm_ctor = "PotsWmlSsm.ctor";

PotsWmlSsm::PotsWmlSsm() : ServiceSM(PotsWmlServiceId),
   wmlp_(nullptr),
   timer_(false)
{
   Debug::ft(PotsWmlSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_dtor = "PotsWmlSsm.dtor";

PotsWmlSsm::~PotsWmlSsm()
{
   Debug::ft(PotsWmlSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_CalcPort = "PotsWmlSsm.CalcPort";

ServicePortId PotsWmlSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsWmlSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_Cancel = "PotsWmlSsm.Cancel";

void PotsWmlSsm::Cancel()
{
   Debug::ft(PotsWmlSsm_Cancel);

   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto upsm = pssm->UPsm();

   if(timer_) upsm->StopTimer(*this, 0);
   SetNextState(Null);
}

//------------------------------------------------------------------------------

void PotsWmlSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ServiceSM::Display(stream, prefix, options);

   stream << prefix << "wmlp  : " << wmlp_ << CRLF;
   stream << prefix << "timer : " << timer_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_ProcessInitAck = "PotsWmlSsm.ProcessInitAck";

EventHandler::Rc PotsWmlSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsWmlSsm_ProcessInitAck);

   auto& ire = static_cast< InitiationReqEvent& >(currEvent);
   auto sid = ire.GetModifier();
   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto stid = pssm.CurrState();
   auto prof = pssm.Profile();
   auto ppsm = PotsCallPsm::Cast(pssm.UPsm());

   wmlp_ = static_cast< PotsWmlFeatureProfile* >(prof->FindFeature(WML));

   switch(sid)
   {
   case PotsWmlServiceId:
      if(stid == BcState::CollectingInformation)
      {
         timer_ = ppsm->StartTimer(wmlp_->Timeout(), *this, 0);
         SetNextState(PotsWmlState::Timing);
         return EventHandler::Resume;
      }
      break;

   case PotsWmlActivation:
      if(stid == BcState::AnalyzingInformation)
      {
         if(wmlp_ == nullptr)
         {
            return pssm.RaiseReleaseCall(nextEvent, Cause::FacilityRejected);
         }

         pssm.DialedDigits().Clear();
         ppsm->ReportDigits(true);
         ppsm->SetOgTone(Tone::StutteredDial);
         pssm.StartTimer(PotsProtocol::CollectionTimeoutId,
            PotsProtocol::FirstDigitTimeout);
         pssm.SetNextState(BcState::CollectingInformation);
         SetNextState(PotsWmlState::Activating);
         return EventHandler::Suspend;
      }
      break;

   case PotsWmlDeactivation:
      if(stid == BcState::AnalyzingInformation)
      {
         if(wmlp_ == nullptr)
         {
            return pssm.RaiseReleaseCall(nextEvent, Cause::FacilityRejected);
         }

         wmlp_->SetActive(false);
         pssm.RaiseReleaseCall(nextEvent, Cause::Confirmation);
         return EventHandler::Revert;
      }
      break;
   }

   Context::Kill(PotsWmlSsm_ProcessInitAck, stid, sid);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_ProcessInitNack = "PotsWmlSsm.ProcessInitNack";

EventHandler::Rc PotsWmlSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsWmlSsm_ProcessInitNack);

   return EventHandler::Resume;
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_ProcessSap = "PotsWmlSsm.ProcessSap";

EventHandler::Rc PotsWmlSsm::ProcessSap
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsWmlSsm_ProcessSap);

   auto& sap = static_cast< AnalyzeSapEvent& >(currEvent);
   auto tid = sap.GetTrigger();
   auto stid = CurrState();
   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto result = pssm.GetAnalysis();

   switch(stid)
   {
   case PotsWmlState::Timing:
      if(tid == BcTrigger::LocalInformationSap) Cancel();
      break;

   case PotsWmlState::Activating:
      switch(tid)
      {
      case BcTrigger::InvalidInformationSap:
         if(pssm.DialedDigits().Empty())
         {
            if(Address::IsValidDN(wmlp_->GetDN()))
            {
               wmlp_->SetActive(true);
               pssm.RaiseReleaseCall(nextEvent, Cause::Confirmation);
               SetNextState(PotsWmlState::Null);
               return EventHandler::Revert;
            }
         }

         pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
         SetNextState(PotsWmlState::Null);
         return EventHandler::Revert;

      case BcTrigger::SelectRouteSap:
         if(result.selector == Address::DnType)
         {
            wmlp_->SetDN(result.identifier);
            wmlp_->SetActive(true);
            pssm.RaiseReleaseCall(nextEvent, Cause::Confirmation);
         }
         else
         {
            pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
         }

         SetNextState(PotsWmlState::Null);
         return EventHandler::Revert;
      }
   }

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_ProcessSip = "PotsWmlSsm.ProcessSip";

EventHandler::Rc PotsWmlSsm::ProcessSip(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsWmlSsm_ProcessSip);

   auto stid = CurrState();

   if(stid == PotsWmlState::Activating)
   {
      auto& pssm = static_cast< PotsBcSsm& >(*Parent());

      pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
      SetNextState(PotsWmlState::Null);
      return EventHandler::Revert;
   }

   Debug::SwErr(PotsWmlSsm_ProcessSip, stid, 0);
   SetNextState(PotsWmlState::Null);
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsWmlSsm_ProcessSnp = "PotsWmlSsm.ProcessSnp";

EventHandler::Rc PotsWmlSsm::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsWmlSsm_ProcessSnp);

   auto pssm = static_cast< PotsBcSsm* >(Parent());

   if(pssm->HasIdled()) Cancel();
   return EventHandler::Pass;
}

//==============================================================================

PotsWmlEventHandler::PotsWmlEventHandler() { }

//------------------------------------------------------------------------------

PotsWmlEventHandler::~PotsWmlEventHandler() { }

//------------------------------------------------------------------------------

fn_name PotsWmlAcAnalyzeMessage_ProcessEvent =
   "PotsWmlAcAnalyzeMessage.ProcessEvent";

EventHandler::Rc PotsWmlAcAnalyzeMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsWmlAcAnalyzeMessage_ProcessEvent);

   return Pass;
}

//------------------------------------------------------------------------------

fn_name PotsWmlTiAnalyzeMessage_ProcessEvent =
   "PotsWmlTiAnalyzeMessage.ProcessEvent";

EventHandler::Rc PotsWmlTiAnalyzeMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsWmlTiAnalyzeMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();

   if(sid == Signal::Timeout)
   {
      auto tmsg = static_cast< TlvMessage* >(ame.Msg());
      auto toi = tmsg->FindType< TimeoutInfo >(Parameter::Timeout);
      auto& wssm = static_cast< PotsWmlSsm& >(ssm);

      if(toi->owner == &wssm)
      {
         nextEvent = new PotsWmlTimeoutEvent(wssm);
         return Continue;
      }
   }

   return Pass;
}

//------------------------------------------------------------------------------

fn_name PotsWmlTiTimeout_ProcessEvent = "PotsWmlTiTimeout.ProcessEvent";

EventHandler::Rc PotsWmlTiTimeout::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsWmlTiTimeout_ProcessEvent);

   auto& wssm = static_cast< PotsWmlSsm& >(ssm);
   auto& pssm = static_cast< PotsBcSsm& >(*wssm.Parent());
   auto wmlp = wssm.Profile();
   auto dn = wmlp->GetDN();
   auto ds = DigitString(dn);
   auto dsrc = pssm.DialedDigits().AddDigits(ds);

   pssm.StopTimer(PotsProtocol::CollectionTimeoutId);

   if((dsrc == DigitString::IllegalDigit) || (dsrc == DigitString::Overflow))
      pssm.RaiseCollectionTimeout(nextEvent);
   else
      pssm.RaiseLocalInformation(nextEvent);

   wssm.SetNextState(PotsWmlState::Null);
   return EventHandler::Revert;
}
}
