//==============================================================================
//
//  PotsCcwService.cpp
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
private:
   PotsCcwNull();
};

class PotsCcwActive : public PotsCcwState
{
   friend class Singleton< PotsCcwActive >;
private:
   PotsCcwActive();
};

class PotsCcwAcCollectInformation : public EventHandler
{
   friend class Singleton< PotsCcwAcCollectInformation >;
private:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
   PotsCcwAcCollectInformation() { }
};

class PotsCcwSsm : public ServiceSM
{
public:
   PotsCcwSsm();
   ~PotsCcwSsm();
private:
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   virtual EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessSip
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessSnp
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

fn_name PotsCcwService_ctor = "PotsCcwService.ctor";

PotsCcwService::PotsCcwService() : Service(PotsCcwServiceId, false, true)
{
   Debug::ft(PotsCcwService_ctor);

   Singleton< PotsCcwNull >::Instance();
   Singleton< PotsCcwActive >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsCcwService_dtor = "PotsCcwService.dtor";

PotsCcwService::~PotsCcwService()
{
   Debug::ft(PotsCcwService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCcwService_AllocModifier = "PotsCcwService.AllocModifier";

ServiceSM* PotsCcwService::AllocModifier() const
{
   Debug::ft(PotsCcwService_AllocModifier);

   return new PotsCcwSsm;
}

//==============================================================================

fn_name PotsCcwState_ctor = "PotsCcwState.ctor";

PotsCcwState::PotsCcwState(Id stid) : State(PotsCcwServiceId, stid)
{
   Debug::ft(PotsCcwState_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCcwState_dtor = "PotsCcwState.dtor";

PotsCcwState::~PotsCcwState()
{
   Debug::ft(PotsCcwState_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCcwNull_ctor = "PotsCcwNull.ctor";

PotsCcwNull::PotsCcwNull() : PotsCcwState(PotsCcwNull::Null)
{
   Debug::ft(PotsCcwNull_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCcwActive_ctor = "PotsCcwActive.ctor";

PotsCcwActive::PotsCcwActive() : PotsCcwState(PotsCcwState::Active)
{
   Debug::ft(PotsCcwActive_ctor);
}

//==============================================================================

fn_name PotsCcwSsm_ctor = "PotsCcwSsm.ctor";

PotsCcwSsm::PotsCcwSsm() : ServiceSM(PotsCcwServiceId)
{
   Debug::ft(PotsCcwSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCcwSsm_dtor = "PotsCcwSsm.dtor";

PotsCcwSsm::~PotsCcwSsm()
{
   Debug::ft(PotsCcwSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCcwSsm_CalcPort = "PotsCcwSsm.CalcPort";

ServicePortId PotsCcwSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsCcwSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsCcwSsm_ProcessInitAck = "PotsCcwSsm.ProcessInitAck";

EventHandler::Rc PotsCcwSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCcwSsm_ProcessInitAck);

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

   Context::Kill(PotsCcwSsm_ProcessInitAck, stid, 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsCcwSsm_ProcessInitNack = "PotsCcwSsm.ProcessInitNack";

EventHandler::Rc PotsCcwSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCcwSsm_ProcessInitNack);

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

   Debug::SwErr(PotsCcwSsm_ProcessSip, stid, 0);
   SetNextState(PotsCcwState::Null);
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCcwSsm_ProcessSnp = "PotsCcwSsm.ProcessSnp";

EventHandler::Rc PotsCcwSsm::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCcwSsm_ProcessSnp);

   auto pssm = static_cast< PotsBcSsm* >(Parent());

   if(pssm->HasIdled()) SetNextState(Null);
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCcwAcCollectInformation_ProcessEvent =
   "PotsCcwAcCollectInformation.ProcessEvent";

EventHandler::Rc PotsCcwAcCollectInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCcwAcCollectInformation_ProcessEvent);

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
