//==============================================================================
//
//  PotsBocService.cpp
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

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBocNull : public State
{
   friend class Singleton< PotsBocNull >;

   PotsBocNull();
   ~PotsBocNull() = default;
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

PotsBocInitiator::PotsBocInitiator() : Initiator(PotsBocServiceId,
   PotsCallServiceId, BcTrigger::AuthorizeOriginationSap,
      PotsAuthorizeOriginationSap::PotsBocPriority)
{
   Debug::ft("PotsBocInitiator.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBocInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBocInitiator.ProcessEvent");

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

PotsBocService::PotsBocService() : Service(PotsBocServiceId, false, true)
{
   Debug::ft("PotsBocService.ctor");

   Singleton< PotsBocNull >::Instance();
}

//------------------------------------------------------------------------------

PotsBocService::~PotsBocService()
{
   Debug::ftnt("PotsBocService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsBocService::AllocModifier() const
{
   Debug::ft("PotsBocService.AllocModifier");

   return new PotsBocSsm;
}

//==============================================================================

PotsBocNull::PotsBocNull() : State(PotsBocServiceId, ServiceSM::Null)
{
   Debug::ft("PotsBocNull.ctor");
}

//==============================================================================

PotsBocSsm::PotsBocSsm() : ServiceSM(PotsBocServiceId)
{
   Debug::ft("PotsBocSsm.ctor");
}

//------------------------------------------------------------------------------

PotsBocSsm::~PotsBocSsm()
{
   Debug::ftnt("PotsBocSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId PotsBocSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsBocSsm.CalcPort");

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBocSsm::ProcessInitAck(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsBocSsm.ProcessInitAck");

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

EventHandler::Rc PotsBocSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsBocSsm.ProcessInitNack");

   return EventHandler::Resume;
}
}
