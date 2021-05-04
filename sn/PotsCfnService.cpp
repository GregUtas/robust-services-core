//==============================================================================
//
//  PotsCfnService.cpp
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
#include "PotsCfnService.h"
#include "Debug.h"
#include "EventHandler.h"
#include "PotsCfnFeature.h"
#include "PotsCfxService.h"
#include "PotsFeatures.h"
#include "PotsProfile.h"
#include "PotsSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsCfnInitiator::PotsCfnInitiator() : Initiator(PotsCfnServiceId,
   PotsCallServiceId, BcTrigger::LocalAlertingSnp,
      PotsLocalAlertingSnp::PotsCfnPriority)
{
   Debug::ft("PotsCfnInitiator.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCfnInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCfnInitiator.ProcessEvent");

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();
   auto cfnp = static_cast< PotsCfnFeatureProfile* >(prof->FindFeature(CFN));

   if((cfnp != nullptr) && cfnp->IsActive())
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsCfnServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

PotsCfnService::PotsCfnService() : Service(PotsCfnServiceId, false, true)
{
   Debug::ft("PotsCfnService.ctor");
}

//------------------------------------------------------------------------------

PotsCfnService::~PotsCfnService()
{
   Debug::ftnt("PotsCfnService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCfnService::AllocModifier() const
{
   Debug::ft("PotsCfnService.AllocModifier");

   return new PotsCfxSsm(PotsCfnServiceId);
}
}
