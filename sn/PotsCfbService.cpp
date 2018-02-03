//==============================================================================
//
//  PotsCfbService.cpp
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
#include "SysTypes.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsCfbInitiator_ctor = "PotsCfbInitiator.ctor";

PotsCfbInitiator::PotsCfbInitiator() :
   Initiator(PotsCfbServiceId, PotsCallServiceId,
   BcTrigger::LocalBusySap, PotsLocalBusySap::PotsCfbPriority)
{
   Debug::ft(PotsCfbInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfbInitiator_ProcessEvent = "PotsCfbInitiator.ProcessEvent";

EventHandler::Rc PotsCfbInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCfbInitiator_ProcessEvent);

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

fn_name PotsCfbService_ctor = "PotsCfbService.ctor";

PotsCfbService::PotsCfbService() : Service(PotsCfbServiceId, false, true)
{
   Debug::ft(PotsCfbService_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfbService_dtor = "PotsCfbService.dtor";

PotsCfbService::~PotsCfbService()
{
   Debug::ft(PotsCfbService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfbService_AllocModifier = "PotsCfbService.AllocModifier";

ServiceSM* PotsCfbService::AllocModifier() const
{
   Debug::ft(PotsCfbService_AllocModifier);

   return new PotsCfxSsm(PotsCfbServiceId);
}
}
