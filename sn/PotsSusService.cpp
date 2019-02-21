//==============================================================================
//
//  PotsSusService.cpp
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
#include "PotsSusService.h"
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
class PotsSusNull : public State
{
   friend class Singleton< PotsSusNull >;
private:
   PotsSusNull();
};

class PotsSusSsm : public ServiceSM
{
public:
   PotsSusSsm();
   ~PotsSusSsm();
private:
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   virtual EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

fn_name PotsSusInitiator_ctor = "PotsSusInitiator.ctor";

PotsSusInitiator::PotsSusInitiator(TriggerId tid, Initiator::Priority prio) :
   Initiator(PotsSusServiceId, PotsCallServiceId, tid, prio)
{
   Debug::ft(PotsSusInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsSusInitiator_ProcessEvent = "PotsSusInitiator.ProcessEvent";

EventHandler::Rc PotsSusInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsSusInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();

   if(prof->HasFeature(SUS))
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsSusServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name PotsOSusInitiator_ctor = "PotsOSusInitiator.ctor";

PotsOSusInitiator::PotsOSusInitiator() :
   PotsSusInitiator(BcTrigger::AuthorizeOriginationSap,
   PotsAuthorizeOriginationSap::PotsSusPriority)
{
   Debug::ft(PotsOSusInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsTSusInitiator_ctor = "PotsTSusInitiator.ctor";

PotsTSusInitiator::PotsTSusInitiator() :
   PotsSusInitiator(BcTrigger::AuthorizeTerminationSap,
   PotsAuthorizeTerminationSap::PotsSusPriority)
{
   Debug::ft(PotsTSusInitiator_ctor);
}

//==============================================================================

fn_name PotsSusService_ctor = "PotsSusService.ctor";

PotsSusService::PotsSusService() : Service(PotsSusServiceId, false, true)
{
   Debug::ft(PotsSusService_ctor);

   Singleton< PotsSusNull >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsSusService_dtor = "PotsSusService.dtor";

PotsSusService::~PotsSusService()
{
   Debug::ft(PotsSusService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsSusService_AllocModifier = "PotsSusService.AllocModifier";

ServiceSM* PotsSusService::AllocModifier() const
{
   Debug::ft(PotsSusService_AllocModifier);

   return new PotsSusSsm;
}

//==============================================================================

fn_name PotsSusNull_ctor = "PotsSusNull.ctor";

PotsSusNull::PotsSusNull() : State(PotsSusServiceId, ServiceSM::Null)
{
   Debug::ft(PotsSusNull_ctor);
}

//==============================================================================

fn_name PotsSusSsm_ctor = "PotsSusSsm.ctor";

PotsSusSsm::PotsSusSsm() : ServiceSM(PotsSusServiceId)
{
   Debug::ft(PotsSusSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsSusSsm_dtor = "PotsSusSsm.dtor";

PotsSusSsm::~PotsSusSsm()
{
   Debug::ft(PotsSusSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsSusSsm_CalcPort = "PotsSusSsm.CalcPort";

ServicePortId PotsSusSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsSusSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsSusSsm_ProcessInitAck = "PotsSusSsm.ProcessInitAck";

EventHandler::Rc PotsSusSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsSusSsm_ProcessInitAck);

   auto& pssm = *Parent();
   auto stid = pssm.CurrState();

   switch(stid)
   {
   case BcState::AuthorizingOrigination:
      pssm.SetNextSap(BcTrigger::OriginationDeniedSap);
      nextEvent = new BcOriginationDeniedEvent(pssm, Cause::FacilityRejected);
      return EventHandler::Revert;

   case BcState::AuthorizingTermination:
      pssm.SetNextSap(BcTrigger::TerminationDeniedSap);
      nextEvent = new BcTerminationDeniedEvent
         (pssm, Cause::DestinationOutOfOrder);
      return EventHandler::Revert;

   default:
      Context::Kill(PotsSusSsm_ProcessInitAck, stid, 0);
   }

   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsSusSsm_ProcessInitNack = "PotsSusSsm.ProcessInitNack";

EventHandler::Rc PotsSusSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsSusSsm_ProcessInitNack);

   return EventHandler::Resume;
}
}
