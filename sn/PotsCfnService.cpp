//==============================================================================
//
//  PotsCfnService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsCfnInitiator_ctor = "PotsCfnInitiator.ctor";

PotsCfnInitiator::PotsCfnInitiator() : Initiator(PotsCfnServiceId,
   PotsCallServiceId, BcTrigger::LocalAlertingSnp,
   PotsLocalAlertingSnp::PotsCfnPriority)
{
   Debug::ft(PotsCfnInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfnInitiator_ProcessEvent = "PotsCfnInitiator.ProcessEvent";

EventHandler::Rc PotsCfnInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& icEvent, Event*& ogEvent) const
{
   Debug::ft(PotsCfnInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();
   auto cfnp = static_cast< PotsCfnFeatureProfile* >(prof->FindFeature(CFN));

   if((cfnp != nullptr) && cfnp->IsActive())
   {
      ogEvent = new InitiationReqEvent(*icEvent.Owner(), PotsCfnServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

fn_name PotsCfnService_ctor = "PotsCfnService.ctor";

PotsCfnService::PotsCfnService() : Service(PotsCfnServiceId, false, true)
{
   Debug::ft(PotsCfnService_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfnService_dtor = "PotsCfnService.dtor";

PotsCfnService::~PotsCfnService()
{
   Debug::ft(PotsCfnService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfnService_AllocModifier = "PotsCfnService.AllocModifier";

ServiceSM* PotsCfnService::AllocModifier() const
{
   Debug::ft(PotsCfnService_AllocModifier);

   return new PotsCfxSsm(PotsCfnServiceId);
}
}