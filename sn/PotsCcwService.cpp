//==============================================================================
//
//  PotsCcwService.cpp
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
#include "PotsCcwService.h"
#include "EventHandler.h"
#include "ServiceSM.h"
#include "State.h"
#include "BcAddress.h"
#include "BcCause.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "PotsFeatures.h"
#include "PotsProfile.h"
#include "PotsProtocol.h"
#include "PotsSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "Tones.h"

using namespace CallBase;
using namespace MediaBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCcwState : public State
{
public:
   static const Id FCCWS = ServiceSM::Null;

   static const Id Null   = FCCWS + 0;
   static const Id Active = FCCWS + 1;
protected:
   explicit PotsCcwState(Id stid);
   virtual ~PotsCcwState();
};

class PotsCcwNull : public PotsCcwState
{
   friend class Singleton< PotsCcwNull >;

   PotsCcwNull();
   ~PotsCcwNull() = default;
};

class PotsCcwActive : public PotsCcwState
{
   friend class Singleton< PotsCcwActive >;

   PotsCcwActive();
   ~PotsCcwActive() = default;
};

class PotsCcwAcCollectInformation : public EventHandler
{
   friend class Singleton< PotsCcwAcCollectInformation >;

   PotsCcwAcCollectInformation() = default;
   ~PotsCcwAcCollectInformation() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCcwSsm : public ServiceSM
{
public:
   PotsCcwSsm();
   ~PotsCcwSsm();
private:
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessSip(Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessSnp(Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

PotsCcwService::PotsCcwService() : Service(PotsCcwServiceId, false, true)
{
   Debug::ft("PotsCcwService.ctor");

   Singleton< PotsCcwNull >::Instance();
   Singleton< PotsCcwActive >::Instance();

   Singleton< PotsCcwAcCollectInformation >::Instance();
}

//------------------------------------------------------------------------------

PotsCcwService::~PotsCcwService()
{
   Debug::ftnt("PotsCcwService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCcwService::AllocModifier() const
{
   Debug::ft("PotsCcwService.AllocModifier");

   return new PotsCcwSsm;
}

//==============================================================================

PotsCcwState::PotsCcwState(Id stid) : State(PotsCcwServiceId, stid)
{
   Debug::ft("PotsCcwState.ctor");
}

//------------------------------------------------------------------------------

PotsCcwState::~PotsCcwState()
{
   Debug::ftnt("PotsCcwState.dtor");
}

//------------------------------------------------------------------------------

PotsCcwNull::PotsCcwNull() : PotsCcwState(Null)
{
   Debug::ft("PotsCcwNull.ctor");
}

//------------------------------------------------------------------------------

PotsCcwActive::PotsCcwActive() : PotsCcwState(PotsCcwState::Active)
{
   Debug::ft("PotsCcwActive.ctor");
}

//==============================================================================

PotsCcwSsm::PotsCcwSsm() : ServiceSM(PotsCcwServiceId)
{
   Debug::ft("PotsCcwSsm.ctor");
}

//------------------------------------------------------------------------------

PotsCcwSsm::~PotsCcwSsm()
{
   Debug::ftnt("PotsCcwSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId PotsCcwSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsCcwSsm.CalcPort");

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCcwSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCcwSsm.ProcessInitAck");

   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto stid = pssm.CurrState();

   if(stid == BcState::AnalyzingInformation)
   {
      auto prof = pssm.Profile();
      auto cwtp = prof->FindFeature(CWT);

      //  The first method of CCW activation is to go offhook, dial the CCW
      //  service code, get stuttered dial tone, and dial the destination.
      //  Using the Force Transition event to apply stuttered dial tone
      //  allows the Warm Line service to re-trigger and dial the destination.
      //
      if(cwtp == nullptr)
      {
         return pssm.RaiseReleaseCall(nextEvent, Cause::FacilityRejected);
      }

      auto handler = Singleton< PotsCcwAcCollectInformation >::Instance();
      pssm.SetNextState(BcState::CollectingInformation);
      pssm.SetNextSap(BcTrigger::CollectInformationSap);
      nextEvent = new ForceTransitionEvent(pssm, *handler);
      SetNextState(PotsCcwState::Active);
      return EventHandler::Revert;

      //p The second method of CCW activation is to flash, get stuttered dial
      //  tone, dial the CCW service code, receive confirmation tone, and be
      //  reconnected to the held call.  In that case, CCW will have to move
      //  from the active call to the held call before applying confirmation
      //  tone and reconnecting the held call.
   }

   Context::Kill("invalid state", stid);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCcwSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCcwSsm.ProcessInitNack");

   return EventHandler::Resume;
}

//------------------------------------------------------------------------------

fn_name PotsCcwSsm_ProcessSip = "PotsCcwSsm.ProcessSip";

EventHandler::Rc PotsCcwSsm::ProcessSip(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCcwSsm_ProcessSip);

   auto stid = CurrState();

   if(stid == PotsCcwState::Active)
   {
      auto& ire = static_cast< InitiationReqEvent& >(currEvent);

      if(ire.GetModifier() == PotsCwaServiceId)
      {
         ire.DenyRequest();
      }

      return EventHandler::Pass;
   }

   Debug::SwLog(PotsCcwSsm_ProcessSip, "unexpected state", stid);
   SetNextState(PotsCcwState::Null);
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCcwSsm::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCcwSsm.ProcessSnp");

   auto pssm = static_cast< PotsBcSsm* >(Parent());

   if(pssm->HasIdled()) SetNextState(Null);
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCcwAcCollectInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCcwAcCollectInformation.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto ppsm = PotsCallPsm::Cast(pssm.UPsm());

   pssm.DialedDigits().Clear();
   ppsm->ReportDigits(true);
   ppsm->SetOgTone(Tone::StutteredDial);
   pssm.StartTimer
      (PotsProtocol::CollectionTimeoutId, PotsProtocol::FirstDigitTimeout);
   return EventHandler::Suspend;
}
}
