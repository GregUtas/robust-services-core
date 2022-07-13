//==============================================================================
//
//  PotsSusService.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsSusNull : public State
{
   friend class Singleton<PotsSusNull>;

   PotsSusNull();
   ~PotsSusNull() = default;
};

class PotsSusSsm : public ServiceSM
{
public:
   PotsSusSsm();
   ~PotsSusSsm();
private:
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

PotsSusInitiator::PotsSusInitiator(TriggerId tid, Initiator::Priority prio) :
   Initiator(PotsSusServiceId, PotsCallServiceId, tid, prio)
{
   Debug::ft("PotsSusInitiator.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsSusInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsSusInitiator.ProcessEvent");

   const auto& pssm = static_cast<const PotsBcSsm&>(parentSsm);
   auto prof = pssm.Profile();

   if(prof->HasFeature(SUS))
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsSusServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

PotsOSusInitiator::PotsOSusInitiator() :
   PotsSusInitiator(BcTrigger::AuthorizeOriginationSap,
      PotsAuthorizeOriginationSap::PotsSusPriority)
{
   Debug::ft("PotsOSusInitiator.ctor");
}

//------------------------------------------------------------------------------

PotsTSusInitiator::PotsTSusInitiator() :
   PotsSusInitiator(BcTrigger::AuthorizeTerminationSap,
      PotsAuthorizeTerminationSap::PotsSusPriority)
{
   Debug::ft("PotsTSusInitiator.ctor");
}

//==============================================================================

PotsSusService::PotsSusService() : Service(PotsSusServiceId, false, true)
{
   Debug::ft("PotsSusService.ctor");

   Singleton<PotsSusNull>::Instance();
}

//------------------------------------------------------------------------------

PotsSusService::~PotsSusService()
{
   Debug::ftnt("PotsSusService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsSusService::AllocModifier() const
{
   Debug::ft("PotsSusService.AllocModifier");

   return new PotsSusSsm;
}

//==============================================================================

PotsSusNull::PotsSusNull() : State(PotsSusServiceId, ServiceSM::Null)
{
   Debug::ft("PotsSusNull.ctor");
}

//==============================================================================

PotsSusSsm::PotsSusSsm() : ServiceSM(PotsSusServiceId)
{
   Debug::ft("PotsSusSsm.ctor");
}

//------------------------------------------------------------------------------

PotsSusSsm::~PotsSusSsm()
{
   Debug::ftnt("PotsSusSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId PotsSusSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsSusSsm.CalcPort");

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsSusSsm::ProcessInitAck(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsSusSsm.ProcessInitAck");

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
      Context::Kill("invalid state", stid);
   }

   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsSusSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsSusSsm.ProcessInitNack");

   return EventHandler::Resume;
}
}
