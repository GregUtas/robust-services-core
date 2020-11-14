//==============================================================================
//
//  PotsCfuService.cpp
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
#include "PotsCfuService.h"
#include "Debug.h"
#include "EventHandler.h"
#include "PotsCfuFeature.h"
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
PotsCfuInitiator::PotsCfuInitiator() : Initiator(PotsCfuServiceId,
   PotsCallServiceId, BcTrigger::AuthorizeTerminationSap,
   PotsAuthorizeTerminationSap::PotsCfuPriority)
{
   Debug::ft("PotsCfuInitiator.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCfuInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCfuInitiator.ProcessEvent");

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();
   auto cfup = static_cast< PotsCfuFeatureProfile* >(prof->FindFeature(CFU));

   if((cfup != nullptr) && cfup->IsActive())
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsCfuServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

PotsCfuActivate::PotsCfuActivate() : Service(PotsCfuActivation, false, true)
{
   Debug::ft("PotsCfuActivate.ctor");
}

//------------------------------------------------------------------------------

PotsCfuActivate::~PotsCfuActivate()
{
   Debug::ftnt("PotsCfuActivate.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCfuActivate::AllocModifier() const
{
   Debug::ft("PotsCfuActivate.AllocModifier");

   return new PotsCfxSsm(PotsCfuActivation);
}

//==============================================================================

PotsCfuDeactivate::PotsCfuDeactivate() :
   Service(PotsCfuDeactivation, false, true)
{
   Debug::ft("PotsCfuDeactivate.ctor");
}

//------------------------------------------------------------------------------

PotsCfuDeactivate::~PotsCfuDeactivate()
{
   Debug::ftnt("PotsCfuDeactivate.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCfuDeactivate::AllocModifier() const
{
   Debug::ft("PotsCfuDeactivate.AllocModifier");

   return new PotsCfxSsm(PotsCfuDeactivation);
}

//==============================================================================

PotsCfuService::PotsCfuService() : Service(PotsCfuServiceId, false, true)
{
   Debug::ft("PotsCfuService.ctor");
}

//------------------------------------------------------------------------------

PotsCfuService::~PotsCfuService()
{
   Debug::ftnt("PotsCfuService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCfuService::AllocModifier() const
{
   Debug::ft("PotsCfuService.AllocModifier");

   return new PotsCfxSsm(PotsCfuServiceId);
}
}
