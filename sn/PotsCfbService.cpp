//==============================================================================
//
//  PotsCfbService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   (const ServiceSM& parentSsm, Event& icEvent, Event*& ogEvent) const
{
   Debug::ft(PotsCfbInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();
   auto cfbp = static_cast< PotsCfbFeatureProfile* >(prof->FindFeature(CFB));

   if((cfbp != nullptr) && cfbp->IsActive())
   {
      ogEvent = new InitiationReqEvent(*icEvent.Owner(), PotsCfbServiceId);
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