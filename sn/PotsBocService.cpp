//==============================================================================
//
//  PotsBocService.cpp
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
#include "PotsBocService.h"
#include "ServiceSM.h"
#include "State.h"
#include "BcCause.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "EventHandler.h"
#include "PotsFeatures.h"
#include "PotsProfile.h"
#include "PotsSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBocNull : public State
{
   friend class Singleton< PotsBocNull >;
private:
   PotsBocNull();
};

class PotsBocSsm : public ServiceSM
{
public:
   PotsBocSsm();
   ~PotsBocSsm();
private:
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

fn_name PotsBocInitiator_ctor = "PotsBocInitiator.ctor";

PotsBocInitiator::PotsBocInitiator() : Initiator(PotsBocServiceId,
   PotsCallServiceId, BcTrigger::AuthorizeOriginationSap,
   PotsAuthorizeOriginationSap::PotsBocPriority)
{
   Debug::ft(PotsBocInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsBocInitiator_ProcessEvent = "PotsBocInitiator.ProcessEvent";

EventHandler::Rc PotsBocInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBocInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();

   if(prof->HasFeature(BOC))
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsBocServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

fn_name PotsBocService_ctor = "PotsBocService.ctor";

PotsBocService::PotsBocService() : Service(PotsBocServiceId, false, true)
{
   Debug::ft(PotsBocService_ctor);

   Singleton< PotsBocNull >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsBocService_dtor = "PotsBocService.dtor";

PotsBocService::~PotsBocService()
{
   Debug::ft(PotsBocService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsBocService_AllocModifier = "PotsBocService.AllocModifier";

ServiceSM* PotsBocService::AllocModifier() const
{
   Debug::ft(PotsBocService_AllocModifier);

   return new PotsBocSsm;
}

//==============================================================================

fn_name PotsBocNull_ctor = "PotsBocNull.ctor";

PotsBocNull::PotsBocNull() : State(PotsBocServiceId, ServiceSM::Null)
{
   Debug::ft(PotsBocNull_ctor);
}

//==============================================================================

fn_name PotsBocSsm_ctor = "PotsBocSsm.ctor";

PotsBocSsm::PotsBocSsm() : ServiceSM(PotsBocServiceId)
{
   Debug::ft(PotsBocSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsBocSsm_dtor = "PotsBocSsm.dtor";

PotsBocSsm::~PotsBocSsm()
{
   Debug::ft(PotsBocSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsBocSsm_CalcPort = "PotsBocSsm.CalcPort";

ServicePortId PotsBocSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsBocSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsBocSsm_ProcessInitAck = "PotsBocSsm.ProcessInitAck";

EventHandler::Rc PotsBocSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsBocSsm_ProcessInitAck);

   auto& pssm = *Parent();
   auto stid = pssm.CurrState();

   if(stid == BcState::AuthorizingOrigination)
   {
      pssm.SetNextSap(BcTrigger::OriginationDeniedSap);
      nextEvent = new BcOriginationDeniedEvent
         (pssm, Cause::OutgoingCallsBarred);
      return EventHandler::Revert;
   }

   Context::Kill("invalid state", stid);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsBocSsm_ProcessInitNack = "PotsBocSsm.ProcessInitNack";

EventHandler::Rc PotsBocSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsBocSsm_ProcessInitNack);

   return EventHandler::Resume;
}
}
