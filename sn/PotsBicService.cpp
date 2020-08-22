//==============================================================================
//
//  PotsBicService.cpp
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
#include "PotsBicService.h"
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
class PotsBicNull : public State
{
   friend class Singleton< PotsBicNull >;
private:
   PotsBicNull();
};

class PotsBicSsm : public ServiceSM
{
public:
   PotsBicSsm();
   ~PotsBicSsm();
private:
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

fn_name PotsBicInitiator_ctor = "PotsBicInitiator.ctor";

PotsBicInitiator::PotsBicInitiator() : Initiator(PotsBicServiceId,
   PotsCallServiceId, BcTrigger::AuthorizeTerminationSap,
   PotsAuthorizeTerminationSap::PotsBicPriority)
{
   Debug::ft(PotsBicInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsBicInitiator_ProcessEvent = "PotsBicInitiator.ProcessEvent";

EventHandler::Rc PotsBicInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBicInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();

   if(prof->HasFeature(BIC))
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsBicServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

fn_name PotsBicService_ctor = "PotsBicService.ctor";

PotsBicService::PotsBicService() : Service(PotsBicServiceId, false, true)
{
   Debug::ft(PotsBicService_ctor);

   Singleton< PotsBicNull >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsBicService_dtor = "PotsBicService.dtor";

PotsBicService::~PotsBicService()
{
   Debug::ftnt(PotsBicService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsBicService_AllocModifier = "PotsBicService.AllocModifier";

ServiceSM* PotsBicService::AllocModifier() const
{
   Debug::ft(PotsBicService_AllocModifier);

   return new PotsBicSsm;
}

//==============================================================================

fn_name PotsBicNull_ctor = "PotsBicNull.ctor";

PotsBicNull::PotsBicNull() : State(PotsBicServiceId, ServiceSM::Null)
{
   Debug::ft(PotsBicNull_ctor);
}

//==============================================================================

fn_name PotsBicSsm_ctor = "PotsBicSsm.ctor";

PotsBicSsm::PotsBicSsm() : ServiceSM(PotsBicServiceId)
{
   Debug::ft(PotsBicSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsBicSsm_dtor = "PotsBicSsm.dtor";

PotsBicSsm::~PotsBicSsm()
{
   Debug::ftnt(PotsBicSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsBicSsm_CalcPort = "PotsBicSsm.CalcPort";

ServicePortId PotsBicSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsBicSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsBicSsm_ProcessInitAck = "PotsBicSsm.ProcessInitAck";

EventHandler::Rc PotsBicSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsBicSsm_ProcessInitAck);

   auto& pssm = *Parent();
   auto stid = pssm.CurrState();

   if(stid == BcState::AuthorizingTermination)
   {
      pssm.SetNextSap(BcTrigger::TerminationDeniedSap);
      nextEvent = new BcTerminationDeniedEvent
         (pssm, Cause::IncomingCallsBarred);
      return EventHandler::Revert;
   }

   Context::Kill("invalid state", stid);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsBicSsm_ProcessInitNack = "PotsBicSsm.ProcessInitNack";

EventHandler::Rc PotsBicSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsBicSsm_ProcessInitNack);

   return EventHandler::Resume;
}
}
