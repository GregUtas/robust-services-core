//==============================================================================
//
//  PotsCfbService.cpp
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
#include "PotsCfbService.h"
#include "Debug.h"
#include "EventHandler.h"
#include "PotsCfbFeature.h"
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
PotsCfbInitiator::PotsCfbInitiator() :
   Initiator(PotsCfbServiceId, PotsCallServiceId,
   BcTrigger::LocalBusySap, PotsLocalBusySap::PotsCfbPriority)
{
   Debug::ft("PotsCfbInitiator.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCfbInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCfbInitiator.ProcessEvent");

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();
   auto cfbp = static_cast< PotsCfbFeatureProfile* >(prof->FindFeature(CFB));

   if((cfbp != nullptr) && cfbp->IsActive())
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsCfbServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

PotsCfbService::PotsCfbService() : Service(PotsCfbServiceId, false, true)
{
   Debug::ft("PotsCfbService.ctor");
}

//------------------------------------------------------------------------------

PotsCfbService::~PotsCfbService()
{
   Debug::ftnt("PotsCfbService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCfbService::AllocModifier() const
{
   Debug::ft("PotsCfbService.AllocModifier");

   return new PotsCfxSsm(PotsCfbServiceId);
}
}
