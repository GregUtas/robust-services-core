//==============================================================================
//
//  PotsCfxService.cpp
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
#include "PotsCfxService.h"
#include <ostream>
#include <string>
#include "BcAddress.h"
#include "BcProtocol.h"
#include "BcRouting.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "Event.h"
#include "PotsCfnFeature.h"
#include "PotsFeatures.h"
#include "PotsProfile.h"
#include "PotsProtocol.h"
#include "PotsSessions.h"
#include "ProxyBcSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "State.h"
#include "SysTypes.h"
#include "TimerProtocol.h"
#include "Tones.h"

using namespace MediaBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfxState : public State
{
public:
   static const Id FCFXS = ServiceSM::Null;

   static const Id Null       = FCFXS + 0;  // just created (CFU/CFB/CFN)
   static const Id Activating = FCFXS + 1;  // CFU programming forward-to DN
   static const Id Timing     = FCFXS + 2;  // CFN waiting to forward call
protected:
   explicit PotsCfxState(Id stid);
   virtual ~PotsCfxState();
};

class PotsCfxNull : public PotsCfxState
{
   friend class Singleton< PotsCfxNull >;
private:
   PotsCfxNull();
};

class PotsCfuActivating : public PotsCfxState
{
   friend class Singleton< PotsCfuActivating >;
private:
   PotsCfuActivating();
};

class PotsCfbTiming : public PotsCfxState
{
   friend class Singleton< PotsCfbTiming >;
private:
   PotsCfbTiming();
};

class PotsCfxEvent : public Event
{
public:
   static const Id Timeout = NextId + 0;
   virtual ~PotsCfxEvent();
protected:
   PotsCfxEvent(Id eid, ServiceSM& owner);
};

class PotsCfbTimeoutEvent : public PotsCfxEvent
{
public:
   explicit PotsCfbTimeoutEvent(ServiceSM& owner);
   ~PotsCfbTimeoutEvent();
};

class PotsCfxEventHandler : public EventHandler
{
public:
   static const Id UnAnalyzeMessage = NextId + 0;
   static const Id TiAnalyzeMessage = NextId + 1;
   static const Id TiTimeout        = NextId + 2;
protected:
   PotsCfxEventHandler();
   virtual ~PotsCfxEventHandler();
};

class PotsCfxUnAnalyzeMessage : public PotsCfxEventHandler
{
   friend class Singleton< PotsCfxUnAnalyzeMessage >;
private:
   PotsCfxUnAnalyzeMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCfbTiAnalyzeMessage : public PotsCfxEventHandler
{
   friend class Singleton< PotsCfbTiAnalyzeMessage >;
private:
   PotsCfbTiAnalyzeMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCfbTiTimeout : public PotsCfxEventHandler
{
   friend class Singleton< PotsCfbTiTimeout >;
private:
   PotsCfbTiTimeout() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//==============================================================================

fixed_string PotsCfbTimeoutEventStr = "PotsCfbTimeoutEvent";

fn_name PotsCfxService_ctor = "PotsCfxService.ctor";

PotsCfxService::PotsCfxService() : Service(PotsCfxServiceId, false, true)
{
   Debug::ft(PotsCfxService_ctor);

   Singleton< PotsCfxNull >::Instance();
   Singleton< PotsCfuActivating >::Instance();
   Singleton< PotsCfbTiming >::Instance();

   BindHandler(*Singleton< PotsCfxUnAnalyzeMessage >::Instance(),
      PotsCfxEventHandler::UnAnalyzeMessage);
   BindHandler(*Singleton< PotsCfbTiAnalyzeMessage >::Instance(),
      PotsCfxEventHandler::TiAnalyzeMessage);
   BindHandler(*Singleton< PotsCfbTiTimeout >::Instance(),
      PotsCfxEventHandler::TiTimeout);

   BindEventName(PotsCfbTimeoutEventStr, PotsCfxEvent::Timeout);
}

//------------------------------------------------------------------------------

fn_name PotsCfxService_dtor = "PotsCfxService.dtor";

PotsCfxService::~PotsCfxService()
{
   Debug::ft(PotsCfxService_dtor);
}

//==============================================================================

fn_name PotsCfxState_ctor = "PotsCfxState.ctor";

PotsCfxState::PotsCfxState(Id stid) : State(PotsCfxServiceId, stid)
{
   Debug::ft(PotsCfxState_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfxState_dtor = "PotsCfxState.dtor";

PotsCfxState::~PotsCfxState()
{
   Debug::ft(PotsCfxState_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfxNull_ctor = "PotsCfxNull.ctor";

PotsCfxNull::PotsCfxNull() : PotsCfxState(PotsCfxState::Null)
{
   Debug::ft(PotsCfxNull_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuActivating_ctor = "PotsCfuActivating.ctor";

PotsCfuActivating::PotsCfuActivating() : PotsCfxState(PotsCfxState::Activating)
{
   Debug::ft(PotsCfuActivating_ctor);

   BindMsgAnalyzer(PotsCfxEventHandler::UnAnalyzeMessage, Service::UserPort);
}

//------------------------------------------------------------------------------

fn_name PotsCfbTiming_ctor = "PotsCfbTiming.ctor";

PotsCfbTiming::PotsCfbTiming() : PotsCfxState(PotsCfxState::Timing)
{
   Debug::ft(PotsCfbTiming_ctor);

   BindMsgAnalyzer(PotsCfxEventHandler::TiAnalyzeMessage, Service::UserPort);
   BindEventHandler(PotsCfxEventHandler::TiTimeout, PotsCfxEvent::Timeout);
}

//==============================================================================

fn_name PotsCfxEvent_ctor = "PotsCfxEvent.ctor";

PotsCfxEvent::PotsCfxEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft(PotsCfxEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfxEvent_dtor = "PotsCfxEvent.dtor";

PotsCfxEvent::~PotsCfxEvent()
{
   Debug::ft(PotsCfxEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfbTimeoutEvent_ctor = "PotsCfbTimeoutEvent.ctor";

PotsCfbTimeoutEvent::PotsCfbTimeoutEvent(ServiceSM& owner) :
   PotsCfxEvent(Timeout, owner)
{
   Debug::ft(PotsCfbTimeoutEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfbTimeoutEvent_dtor = "PotsCfbTimeoutEvent.dtor";

PotsCfbTimeoutEvent::~PotsCfbTimeoutEvent()
{
   Debug::ft(PotsCfbTimeoutEvent_dtor);
}

//==============================================================================

PotsCfxEventHandler::PotsCfxEventHandler() { }

//------------------------------------------------------------------------------

PotsCfxEventHandler::~PotsCfxEventHandler() { }

//------------------------------------------------------------------------------

fn_name PotsCfxUnAnalyzeMessage_ProcessEvent =
   "PotsCfxUnAnalyzeMessage.ProcessEvent";

EventHandler::Rc PotsCfxUnAnalyzeMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCfxUnAnalyzeMessage_ProcessEvent);

   return Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCfbTiAnalyzeMessage_ProcessEvent =
   "PotsCfbTiAnalyzeMessage.ProcessEvent";

EventHandler::Rc PotsCfbTiAnalyzeMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCfbTiAnalyzeMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();

   if(sid == Signal::Timeout)
   {
      auto tmsg = static_cast< TlvMessage* >(ame.Msg());
      auto toi = tmsg->FindType< TimeoutInfo >(Parameter::Timeout);
      auto& cssm = static_cast< PotsCfxSsm& >(ssm);

      if(toi->owner == &cssm)
      {
         nextEvent = new PotsCfbTimeoutEvent(cssm);
         return Continue;
      }
   }

   return Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCfbTiTimeout_ProcessEvent = "PotsCfbTiTimeout.ProcessEvent";

EventHandler::Rc PotsCfbTiTimeout::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCfbTiTimeout_ProcessEvent);

   auto& cssm = static_cast< PotsCfxSsm& >(ssm);
   auto& pssm = static_cast< PotsBcSsm& >(*cssm.Parent());

   if(pssm.CurrState() == BcState::TermAlerting)
   {
      return cssm.ForwardCall(nextEvent);
   }

   Context::Kill(PotsCfbTiTimeout_ProcessEvent, 0, 0);
   return Suspend;
}

//==============================================================================

fn_name PotsCfxSsm_ctor = "PotsCfxSsm.ctor";

PotsCfxSsm::PotsCfxSsm(ServiceId sid) : ServiceSM(PotsCfxServiceId),
   cfxp_(nullptr),
   timer_(false)
{
   Debug::ft(PotsCfxSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_dtor = "PotsCfxSsm.dtor";

PotsCfxSsm::~PotsCfxSsm()
{
   Debug::ft(PotsCfxSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_CalcPort = "PotsCfxSsm.CalcPort";

ServicePortId PotsCfxSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsCfxSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_Cancel = "PotsCfxSsm.Cancel";

void PotsCfxSsm::Cancel()
{
   Debug::ft(PotsCfxSsm_Cancel);

   if(timer_)
   {
      auto pssm = static_cast< PotsBcSsm* >(Parent());
      auto upsm = pssm->UPsm();
      upsm->StopTimer(*this, 0);
   }

   SetNextState(Null);
}

//------------------------------------------------------------------------------

void PotsCfxSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ServiceSM::Display(stream, prefix, options);

   stream << prefix << "cfxp  : " << cfxp_ << CRLF;
   stream << prefix << "timer : " << timer_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_ForwardCall = "PotsCfxSsm.ForwardCall";

EventHandler::Rc PotsCfxSsm::ForwardCall(Event*& nextEvent)
{
   Debug::ft(PotsCfxSsm_ForwardCall);

   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto upsm = pssm.UPsm();
   auto npsm = pssm.NPsm();

   auto nmsg = static_cast< CipMessage* >(npsm->FindRcvdMsg(CipSignal::IAM));
   if(nmsg == nullptr)
      return ReleaseCall(nextEvent, Cause::TemporaryFailure, nullptr);

   auto ppsm = pssm.AllocOgProxy();
   if(ppsm == nullptr)
      return ReleaseCall(nextEvent, Cause::TemporaryFailure, nullptr);

   auto umsg = new CipMessage(ppsm, 44);
   if(umsg == nullptr)
      return ReleaseCall(nextEvent, Cause::TemporaryFailure, nullptr);

   auto clg = nmsg->FindType< DigitString >(CipParameter::Calling);
   if(clg == nullptr)
      return ReleaseCall(nextEvent, Cause::ParameterAbsent, umsg);

   auto cld = nmsg->FindType< DigitString >(CipParameter::Called);
   if(cld == nullptr)
      return ReleaseCall(nextEvent, Cause::ParameterAbsent, umsg);

   if(nmsg->FindType< DigitString >(CipParameter::OriginalCalled) != nullptr)
      return ReleaseCall(nextEvent, Cause::ExcessiveRedirection, umsg);

   umsg->SetSignal(CipSignal::IAM);

   RouteResult route;
   route.selector = PotsCallFactoryId;
   route.identifier = pssm.Profile()->GetDN();
   umsg->AddRoute(route);

   umsg->AddAddress(*cld, CipParameter::Calling);
   umsg->AddAddress(cfxp_->GetDN(), CipParameter::Called);

   auto oclg = nmsg->FindType< DigitString >(CipParameter::OriginalCalling);

   if(oclg != nullptr)
   {
      umsg->AddAddress(*oclg, CipParameter::OriginalCalling);
      umsg->AddAddress(*clg, CipParameter::OriginalCalled);
   }
   else
   {
      umsg->AddAddress(*clg, CipParameter::OriginalCalling);
   }

   npsm->EnableMedia(*ppsm);
   ppsm->SetOgTone(Tone::Media);

   auto stid = pssm.CurrState();

   switch(stid)
   {
   case BcState::AuthorizingTermination:
   case BcState::SelectingFacility:
      pssm.SetNextState(BcState::PresentingCall);
   }

   if(upsm == nullptr)
   {
      ppsm->SetIcTone(Tone::Media);
      pssm.MorphToService(PotsProxyServiceId);
      return EventHandler::Suspend;
   }

   pssm.RaiseReleaseUser(nextEvent, Cause::CallRedirected);
   SetNextState(Null);
   return EventHandler::Revert;
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_ProcessInitAck = "PotsCfxSsm.ProcessInitAck";

EventHandler::Rc PotsCfxSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCfxSsm_ProcessInitAck);

   auto& ire = static_cast< InitiationReqEvent& >(currEvent);
   auto sid = ire.GetModifier();
   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto ppsm = PotsCallPsm::Cast(pssm.UPsm());
   auto stid = pssm.CurrState();
   auto prof = pssm.Profile();

   DnRouteFeatureProfile* cfxp;
   PotsCfnFeatureProfile* cfnp;

   switch(sid)
   {
   case PotsCfuActivation:
      if(stid == BcState::AnalyzingInformation)
      {
         cfxp = static_cast< DnRouteFeatureProfile* >(prof->FindFeature(CFU));

         if(cfxp == nullptr)
         {
            return pssm.RaiseReleaseCall(nextEvent, Cause::FacilityRejected);
         }

         SetProfile(cfxp);
         pssm.DialedDigits().Clear();
         ppsm->ReportDigits(true);
         ppsm->SetOgTone(Tone::StutteredDial);
         pssm.StartTimer(PotsProtocol::CollectionTimeoutId,
            PotsProtocol::FirstDigitTimeout);
         pssm.SetNextState(BcState::CollectingInformation);
         SetNextState(PotsCfxState::Activating);
         return EventHandler::Suspend;
      }
      break;

   case PotsCfuDeactivation:
      if(stid == BcState::AnalyzingInformation)
      {
         cfxp = static_cast< DnRouteFeatureProfile* >(prof->FindFeature(CFU));

         if(cfxp == nullptr)
         {
            return pssm.RaiseReleaseCall(nextEvent, Cause::FacilityRejected);
         }

         cfxp->SetActive(false);
         pssm.RaiseReleaseCall(nextEvent, Cause::Confirmation);
         return EventHandler::Revert;
      }
      break;

   case PotsCfuServiceId:
      cfxp = static_cast< DnRouteFeatureProfile* >(prof->FindFeature(CFU));
      if(cfxp == nullptr) Context::Kill(PotsCfxSsm_ProcessInitAck, stid, sid);
      SetProfile(cfxp);
      return ForwardCall(nextEvent);

   case PotsCfbServiceId:
      cfxp = static_cast< DnRouteFeatureProfile* >(prof->FindFeature(CFB));
      if(cfxp == nullptr) Context::Kill(PotsCfxSsm_ProcessInitAck, stid, sid);
      SetProfile(cfxp);
      return ForwardCall(nextEvent);

   case PotsCfnServiceId:
      cfnp = static_cast< PotsCfnFeatureProfile* >(prof->FindFeature(CFN));
      if(cfnp == nullptr) Context::Kill(PotsCfxSsm_ProcessInitAck, stid, sid);
      SetProfile(cfnp);
      timer_ = ppsm->StartTimer(cfnp->Timeout(), *this, 0);
      SetNextState(PotsCfxState::Timing);
      return EventHandler::Resume;
   }

   Context::Kill(PotsCfxSsm_ProcessInitAck, stid, sid);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_ProcessInitNack = "PotsCfxSsm.ProcessInitNack";

EventHandler::Rc PotsCfxSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCfxSsm_ProcessInitNack);

   return EventHandler::Resume;
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_ProcessSap = "PotsCfxSsm.ProcessSap";

EventHandler::Rc PotsCfxSsm::ProcessSap
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCfxSsm_ProcessSap);

   auto stid = CurrState();
   auto& sap = static_cast< AnalyzeSapEvent& >(currEvent);
   auto tid = sap.GetTrigger();
   auto cfup = Profile();
   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto result = pssm.GetAnalysis();

   if(stid == PotsCfxState::Activating)
   {
      switch(tid)
      {
      case BcTrigger::InvalidInformationSap:
         if(pssm.DialedDigits().Empty())
         {
            if(Address::IsValidDN(cfup->GetDN()))
            {
               cfup->SetActive(true);
               pssm.RaiseReleaseCall(nextEvent, Cause::Confirmation);
               SetNextState(PotsCfxState::Null);
               return EventHandler::Revert;
            }
         }

         pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
         SetNextState(PotsCfxState::Null);
         return EventHandler::Revert;

      case BcTrigger::SelectRouteSap:
         if(result.selector == Address::DnType)
         {
            cfup->SetDN(result.identifier);
            cfup->SetActive(true);
            pssm.RaiseReleaseCall(nextEvent, Cause::Confirmation);
         }
         else
         {
            pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
         }

         SetNextState(PotsCfxState::Null);
         return EventHandler::Revert;
      }
   }

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_ProcessSip = "PotsCfxSsm.ProcessSip";

EventHandler::Rc PotsCfxSsm::ProcessSip(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCfxSsm_ProcessSip);

   auto stid = CurrState();

   if(stid == PotsCfxState::Activating)
   {
      auto& pssm = static_cast< PotsBcSsm& >(*Parent());

      pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
      SetNextState(PotsCfxState::Null);
      return EventHandler::Revert;
   }

   auto& ire = static_cast< InitiationReqEvent& >(currEvent);

   ire.DenyRequest();
   Debug::SwErr(PotsCfxSsm_ProcessSip, stid, ire.GetModifier());
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_ProcessSnp = "PotsCfxSsm.ProcessSnp";

EventHandler::Rc PotsCfxSsm::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCfxSsm_ProcessSnp);

   auto pssm = static_cast< PotsBcSsm* >(Parent());

   if(pssm->HasIdled())
   {
      Cancel();
   }
   else if(CurrState() == PotsCfxState::Timing)
   {
      auto& snp = static_cast< AnalyzeSnpEvent& >(currEvent);
      auto tid = snp.GetTrigger();

      if(tid == BcTrigger::LocalAnswerSnp) Cancel();
   }

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCfxSsm_ReleaseCall = "PotsCfxSsm.ReleaseCall";

EventHandler::Rc PotsCfxSsm::ReleaseCall
   (Event*& nextEvent, Cause::Ind cause, CipMessage* msg)
{
   Debug::ft(PotsCfxSsm_ReleaseCall);

   auto& pssm = static_cast< PotsBcSsm& >(*Parent());

   if(msg != nullptr) delete msg;
   pssm.RaiseReleaseCall(nextEvent, cause);
   SetNextState(PotsCfxState::Null);
   return EventHandler::Revert;
}
}
