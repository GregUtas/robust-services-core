//==============================================================================
//
//  PotsCfxService.cpp
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
#include "PotsCfxService.h"
#include "Event.h"
#include "State.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "BcAddress.h"
#include "BcProtocol.h"
#include "BcRouting.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "PotsCfnFeature.h"
#include "PotsFeatures.h"
#include "PotsProfile.h"
#include "PotsProtocol.h"
#include "PotsSessions.h"
#include "ProxyBcSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
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

   PotsCfxNull();
   ~PotsCfxNull() = default;
};

class PotsCfuActivating : public PotsCfxState
{
   friend class Singleton< PotsCfuActivating >;

   PotsCfuActivating();
   ~PotsCfuActivating() = default;
};

class PotsCfbTiming : public PotsCfxState
{
   friend class Singleton< PotsCfbTiming >;

   PotsCfbTiming();
   ~PotsCfbTiming() = default;
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
   PotsCfxEventHandler() = default;
   virtual ~PotsCfxEventHandler() = default;
};

class PotsCfxUnAnalyzeMessage : public PotsCfxEventHandler
{
   friend class Singleton< PotsCfxUnAnalyzeMessage >;

   PotsCfxUnAnalyzeMessage() = default;
   ~PotsCfxUnAnalyzeMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCfbTiAnalyzeMessage : public PotsCfxEventHandler
{
   friend class Singleton< PotsCfbTiAnalyzeMessage >;

   PotsCfbTiAnalyzeMessage() = default;
   ~PotsCfbTiAnalyzeMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCfbTiTimeout : public PotsCfxEventHandler
{
   friend class Singleton< PotsCfbTiTimeout >;

   PotsCfbTiTimeout() = default;
   ~PotsCfbTiTimeout() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//==============================================================================

fixed_string PotsCfbTimeoutEventStr = "PotsCfbTimeoutEvent";

PotsCfxService::PotsCfxService() : Service(PotsCfxServiceId, false, true)
{
   Debug::ft("PotsCfxService.ctor");

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

PotsCfxService::~PotsCfxService()
{
   Debug::ftnt("PotsCfxService.dtor");
}

//==============================================================================

PotsCfxState::PotsCfxState(Id stid) : State(PotsCfxServiceId, stid)
{
   Debug::ft("PotsCfxState.ctor");
}

//------------------------------------------------------------------------------

PotsCfxState::~PotsCfxState()
{
   Debug::ftnt("PotsCfxState.dtor");
}

//------------------------------------------------------------------------------

PotsCfxNull::PotsCfxNull() : PotsCfxState(PotsCfxState::Null)
{
   Debug::ft("PotsCfxNull.ctor");
}

//------------------------------------------------------------------------------

PotsCfuActivating::PotsCfuActivating() : PotsCfxState(PotsCfxState::Activating)
{
   Debug::ft("PotsCfuActivating.ctor");

   BindMsgAnalyzer(PotsCfxEventHandler::UnAnalyzeMessage, Service::UserPort);
}

//------------------------------------------------------------------------------

PotsCfbTiming::PotsCfbTiming() : PotsCfxState(PotsCfxState::Timing)
{
   Debug::ft("PotsCfbTiming.ctor");

   BindMsgAnalyzer(PotsCfxEventHandler::TiAnalyzeMessage, Service::UserPort);
   BindEventHandler(PotsCfxEventHandler::TiTimeout, PotsCfxEvent::Timeout);
}

//==============================================================================

PotsCfxEvent::PotsCfxEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft("PotsCfxEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCfxEvent::~PotsCfxEvent()
{
   Debug::ftnt("PotsCfxEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCfbTimeoutEvent::PotsCfbTimeoutEvent(ServiceSM& owner) :
   PotsCfxEvent(Timeout, owner)
{
   Debug::ft("PotsCfbTimeoutEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCfbTimeoutEvent::~PotsCfbTimeoutEvent()
{
   Debug::ftnt("PotsCfbTimeoutEvent.dtor");
}

//==============================================================================

EventHandler::Rc PotsCfxUnAnalyzeMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCfxUnAnalyzeMessage.ProcessEvent");

   return Pass;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCfbTiAnalyzeMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCfbTiAnalyzeMessage.ProcessEvent");

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

EventHandler::Rc PotsCfbTiTimeout::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCfbTiTimeout.ProcessEvent");

   auto& cssm = static_cast< PotsCfxSsm& >(ssm);
   auto& pssm = static_cast< PotsBcSsm& >(*cssm.Parent());

   if(pssm.CurrState() == BcState::TermAlerting)
   {
      return cssm.ForwardCall(nextEvent);
   }

   Context::Kill("invalid state", pssm.CurrState());
   return Suspend;
}

//==============================================================================

PotsCfxSsm::PotsCfxSsm(ServiceId sid) : ServiceSM(PotsCfxServiceId),
   cfxp_(nullptr),
   timer_(false)
{
   Debug::ft("PotsCfxSsm.ctor");
}

//------------------------------------------------------------------------------

PotsCfxSsm::~PotsCfxSsm()
{
   Debug::ftnt("PotsCfxSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId PotsCfxSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsCfxSsm.CalcPort");

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

void PotsCfxSsm::Cancel()
{
   Debug::ft("PotsCfxSsm.Cancel");

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

EventHandler::Rc PotsCfxSsm::ForwardCall(Event*& nextEvent)
{
   Debug::ft("PotsCfxSsm.ForwardCall");

   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto upsm = pssm.UPsm();
   auto npsm = pssm.NPsm();

   auto nmsg = npsm->FindRcvdMsg(CipSignal::IAM);
   if(nmsg == nullptr)
      return ReleaseCall(nextEvent, Cause::TemporaryFailure, nullptr);

   auto ppsm = pssm.AllocOgProxy();
   if(ppsm == nullptr)
      return ReleaseCall(nextEvent, Cause::TemporaryFailure, nullptr);

   auto umsg = new CipMessage(ppsm, 44);

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

EventHandler::Rc PotsCfxSsm::ProcessInitAck(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCfxSsm.ProcessInitAck");

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
      if(cfxp == nullptr) Context::Kill("CFU not assigned", pack2(stid, sid));
      SetProfile(cfxp);
      return ForwardCall(nextEvent);

   case PotsCfbServiceId:
      cfxp = static_cast< DnRouteFeatureProfile* >(prof->FindFeature(CFB));
      if(cfxp == nullptr) Context::Kill("CFB not assigned", pack2(stid, sid));
      SetProfile(cfxp);
      return ForwardCall(nextEvent);

   case PotsCfnServiceId:
      cfnp = static_cast< PotsCfnFeatureProfile* >(prof->FindFeature(CFN));
      if(cfnp == nullptr) Context::Kill("CFN not assigned", pack2(stid, sid));
      SetProfile(cfnp);
      timer_ = ppsm->StartTimer(cfnp->Timeout(), *this, 0);
      SetNextState(PotsCfxState::Timing);
      return EventHandler::Resume;
   }

   Context::Kill("invalid service", pack2(stid, sid));
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCfxSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCfxSsm.ProcessInitNack");

   return EventHandler::Resume;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCfxSsm::ProcessSap(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCfxSsm.ProcessSap");

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
   Debug::SwLog(PotsCfxSsm_ProcessSip,
      "unexpected state", pack2(ire.GetModifier(), stid));
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCfxSsm::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCfxSsm.ProcessSnp");

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

EventHandler::Rc PotsCfxSsm::ReleaseCall
   (Event*& nextEvent, Cause::Ind cause, CipMessage* msg)
{
   Debug::ft("PotsCfxSsm.ReleaseCall");

   auto& pssm = static_cast< PotsBcSsm& >(*Parent());

   delete msg;
   pssm.RaiseReleaseCall(nextEvent, cause);
   SetNextState(PotsCfxState::Null);
   return EventHandler::Revert;
}
}
