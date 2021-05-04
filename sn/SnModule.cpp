//==============================================================================
//
//  SnModule.cpp
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
#include "SnModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "PbModule.h"
#include "PotsBicService.h"
#include "PotsBocService.h"
#include "PotsCcwService.h"
#include "PotsCfbService.h"
#include "PotsCfnService.h"
#include "PotsCfuService.h"
#include "PotsCfxService.h"
#include "PotsCwtService.h"
#include "PotsHtlService.h"
#include "PotsMultiplexer.h"
#include "PotsProxySessions.h"
#include "PotsSessions.h"
#include "PotsStatistics.h"
#include "PotsSusService.h"
#include "PotsTreatmentRegistry.h"
#include "PotsWmlService.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SnIncrement.h"
#include "SymbolRegistry.h"
#include "SysTypes.h"

using namespace PotsBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace ServiceNode
{
SnModule::SnModule() : Module()
{
   Debug::ft("SnModule.ctor");

   //  Create the modules required by ServiceNode.
   //
   Singleton< PbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

SnModule::~SnModule()
{
   Debug::ftnt("SnModule.dtor");
}

//------------------------------------------------------------------------------

void SnModule::Shutdown(RestartLevel level)
{
   Debug::ft("SnModule.Shutdown");

   Singleton< PotsTreatmentRegistry >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

void SnModule::Startup(RestartLevel level)
{
   Debug::ft("SnModule.Startup");

   Singleton< SnIncrement >::Instance()->Startup(level);

   Singleton< PotsBcService >::Instance()->Startup(level);
   Singleton< PotsProxyService >::Instance()->Startup(level);
   Singleton< PotsCallFactory >::Instance()->Startup(level);
   Singleton< PotsMuxFactory >::Instance()->Startup(level);
   Singleton< PotsCallIpService >::Instance()->Startup(level);
   Singleton< PotsTreatmentRegistry >::Instance()->Startup(level);

   Singleton< PotsBicService >::Instance()->Startup(level);
   Singleton< PotsBocService >::Instance()->Startup(level);
   Singleton< PotsCcwService >::Instance()->Startup(level);
   Singleton< PotsCfbService >::Instance()->Startup(level);
   Singleton< PotsCfnService >::Instance()->Startup(level);
   Singleton< PotsCfxService >::Instance()->Startup(level);
   Singleton< PotsCfuActivate >::Instance()->Startup(level);
   Singleton< PotsCfuDeactivate >::Instance()->Startup(level);
   Singleton< PotsCfuService >::Instance()->Startup(level);
   Singleton< PotsCwaService >::Instance()->Startup(level);
   Singleton< PotsCwbService >::Instance()->Startup(level);
   Singleton< PotsCwmService >::Instance()->Startup(level);
   Singleton< PotsDiscService >::Instance()->Startup(level);
   Singleton< PotsHtlService >::Instance()->Startup(level);
   Singleton< PotsMuxService >::Instance()->Startup(level);
   Singleton< PotsSusService >::Instance()->Startup(level);
   Singleton< PotsWmlActivate >::Instance()->Startup(level);
   Singleton< PotsWmlDeactivate >::Instance()->Startup(level);
   Singleton< PotsWmlService >::Instance()->Startup(level);

   Singleton< PotsStatistics >::Instance()->Startup(level);

   //  Create initiators.
   //
   if(level >= RestartReboot)
   {
      new PotsOSusInitiator;
      new PotsTSusInitiator;
      new PotsBocInitiator;
      new PotsBicInitiator;
      new PotsHtlInitiator;
      new PotsWmlInitiator;
      new PotsCwtInitiator;
      new PotsCfuInitiator;
      new PotsCfbInitiator;
      new PotsCfnInitiator;
   }

   //  Define symbols.
   //
   auto reg = Singleton< SymbolRegistry >::Instance();

   reg->BindSymbol("factory.pots.shelf", PotsShelfFactoryId);
   reg->BindSymbol("factory.pots.call", PotsCallFactoryId);
   reg->BindSymbol("factory.pots.mux", PotsMuxFactoryId);

   reg->BindSymbol("service.cwb", PotsCwbServiceId);
   reg->BindSymbol("service.cwm", PotsCwmServiceId);
   reg->BindSymbol("service.cwa", PotsCwaServiceId);
   reg->BindSymbol("service.disc", PotsDiscServiceId);

   reg->BindSymbol("facility.req", Facility::InitiationReq);
   reg->BindSymbol("facility.ack", Facility::InitiationAck);
   reg->BindSymbol("facility.nack", Facility::InitiationNack);

   reg->BindSymbol("facility.cwt.timeout", PotsCwtFacility::InitiationTimeout);
   reg->BindSymbol("facility.cwt.unanswered", PotsCwtFacility::Unanswered);
   reg->BindSymbol("facility.cwt.answered", PotsCwtFacility::Answered);
   reg->BindSymbol("facility.cwt.retrieved", PotsCwtFacility::Retrieved);
   reg->BindSymbol("facility.cwt.reconnected", PotsCwtFacility::Reconnected);
   reg->BindSymbol("facility.cwt.reanswered", PotsCwtFacility::Reanswered);
   reg->BindSymbol("facility.cwt.released", PotsCwtFacility::InactiveReleased);
   reg->BindSymbol("facility.cwt.alerted", PotsCwtFacility::Alerted);
}
}
