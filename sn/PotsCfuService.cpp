//==============================================================================
//
//  PotsCfuService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsCfuInitiator_ctor = "PotsCfuInitiator.ctor";

PotsCfuInitiator::PotsCfuInitiator() : Initiator(PotsCfuServiceId,
   PotsCallServiceId, BcTrigger::AuthorizeTerminationSap,
   PotsAuthorizeTerminationSap::PotsCfuPriority)
{
   Debug::ft(PotsCfuInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuInitiator_ProcessEvent = "PotsCfuInitiator.ProcessEvent";

EventHandler::Rc PotsCfuInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& icEvent, Event*& ogEvent) const
{
   Debug::ft(PotsCfuInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();
   auto cfup = static_cast< PotsCfuFeatureProfile* >(prof->FindFeature(CFU));

   if((cfup != nullptr) && cfup->IsActive())
   {
      ogEvent = new InitiationReqEvent(*icEvent.Owner(), PotsCfuServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

fn_name PotsCfuActivate_ctor = "PotsCfuActivate.ctor";

PotsCfuActivate::PotsCfuActivate() : Service(PotsCfuActivation, false, true)
{
   Debug::ft(PotsCfuActivate_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuActivate_dtor = "PotsCfuActivate.dtor";

PotsCfuActivate::~PotsCfuActivate()
{
   Debug::ft(PotsCfuActivate_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuActivate_AllocModifier = "PotsCfuActivate.AllocModifier";

ServiceSM* PotsCfuActivate::AllocModifier() const
{
   Debug::ft(PotsCfuActivate_AllocModifier);

   return new PotsCfxSsm(PotsCfuActivation);
}

//==============================================================================

fn_name PotsCfuDeactivate_ctor = "PotsCfuDeactivate.ctor";

PotsCfuDeactivate::PotsCfuDeactivate() :
   Service(PotsCfuDeactivation, false, true)
{
   Debug::ft(PotsCfuDeactivate_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuDeactivate_dtor = "PotsCfuDeactivate.dtor";

PotsCfuDeactivate::~PotsCfuDeactivate()
{
   Debug::ft(PotsCfuDeactivate_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuDeactivate_AllocModifier = "PotsCfuDeactivate.AllocModifier";

ServiceSM* PotsCfuDeactivate::AllocModifier() const
{
   Debug::ft(PotsCfuDeactivate_AllocModifier);

   return new PotsCfxSsm(PotsCfuDeactivation);
}

//==============================================================================

fn_name PotsCfuService_ctor = "PotsCfuService.ctor";

PotsCfuService::PotsCfuService() : Service(PotsCfuServiceId, false, true)
{
   Debug::ft(PotsCfuService_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuService_dtor = "PotsCfuService.dtor";

PotsCfuService::~PotsCfuService()
{
   Debug::ft(PotsCfuService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfuService_AllocModifier = "PotsCfuService.AllocModifier";

ServiceSM* PotsCfuService::AllocModifier() const
{
   Debug::ft(PotsCfuService_AllocModifier);

   return new PotsCfxSsm(PotsCfuServiceId);
}
}