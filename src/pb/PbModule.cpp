//==============================================================================
//
//  PbModule.cpp
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
#include "PbModule.h"
#include "CbModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "PotsBicFeature.h"
#include "PotsBocFeature.h"
#include "PotsCfbFeature.h"
#include "PotsCfnFeature.h"
#include "PotsCfuFeature.h"
#include "PotsCwtFeature.h"
#include "PotsCxfFeature.h"
#include "PotsFeatureRegistry.h"
#include "PotsHtlFeature.h"
#include "PotsIncrement.h"
#include "PotsLogs.h"
#include "PotsProfileRegistry.h"
#include "PotsProtocol.h"
#include "PotsSusFeature.h"
#include "PotsTwcFeature.h"
#include "PotsWmlFeature.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
PbModule::PbModule() : Module()
{
   Debug::ft("PbModule.ctor");

   //  Create the modules required by PotsBase.
   //
   Singleton<CbModule>::Instance();
   Singleton<ModuleRegistry>::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

PbModule::~PbModule()
{
   Debug::ftnt("PbModule.dtor");
}

//------------------------------------------------------------------------------

void PbModule::Shutdown(RestartLevel level)
{
   Debug::ft("PbModule.Shutdown");

   Singleton<PotsFeatureRegistry>::Instance()->Shutdown(level);
   Singleton<PotsProfileRegistry>::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

void PbModule::Startup(RestartLevel level)
{
   Debug::ft("PbModule.Startup");

   CreatePotsLogs(level);
   Singleton<PotsProfileRegistry>::Instance()->Startup(level);
   Singleton<PotsBicFeature>::Instance()->Startup(level);
   Singleton<PotsBocFeature>::Instance()->Startup(level);
   Singleton<PotsCfbFeature>::Instance()->Startup(level);
   Singleton<PotsCfnFeature>::Instance()->Startup(level);
   Singleton<PotsCfuFeature>::Instance()->Startup(level);
   Singleton<PotsCwtFeature>::Instance()->Startup(level);
   Singleton<PotsCxfFeature>::Instance()->Startup(level);
   Singleton<PotsHtlFeature>::Instance()->Startup(level);
   Singleton<PotsSusFeature>::Instance()->Startup(level);
   Singleton<PotsTwcFeature>::Instance()->Startup(level);
   Singleton<PotsWmlFeature>::Instance()->Startup(level);
   Singleton<PotsProtocol>::Instance()->Startup(level);

   //  Audit the features and create their CLI parameters.
   //  This is done before creating the POTS CLI increment.
   //
   if(level >= RestartReboot)
   {
      Singleton<PotsFeatureRegistry>::Instance()->Audit();
   }

   Singleton<PotsIncrement>::Instance()->Startup(level);
}
}
