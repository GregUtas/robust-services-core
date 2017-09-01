//==============================================================================
//
//  PbModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PbModule.h"
#include "CbModule.h"
#include "Debug.h"
#include "NbAppIds.h"
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
bool PbModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name PbModule_ctor = "PbModule.ctor";

PbModule::PbModule() : Module(PbModuleId)
{
   Debug::ft(PbModule_ctor);
}

//------------------------------------------------------------------------------

fn_name PbModule_dtor = "PbModule.dtor";

PbModule::~PbModule()
{
   Debug::ft(PbModule_dtor);
}

//------------------------------------------------------------------------------

fn_name PbModule_Register = "PbModule.Register";

bool PbModule::Register()
{
   Debug::ft(PbModule_Register);

   //  Create the modules required by PotsBase.
   //
   Singleton< CbModule >::Instance();
   Singleton< PbModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name PbModule_Shutdown = "PbModule.Shutdown";

void PbModule::Shutdown(RestartLevel level)
{
   Debug::ft(PbModule_Shutdown);

   Singleton< PotsIncrement >::Instance()->Shutdown(level);
   Singleton< PotsProtocol >::Instance()->Shutdown(level);
   Singleton< PotsBicFeature >::Instance()->Shutdown(level);
   Singleton< PotsBocFeature >::Instance()->Shutdown(level);
   Singleton< PotsCfbFeature >::Instance()->Shutdown(level);
   Singleton< PotsCfnFeature >::Instance()->Shutdown(level);
   Singleton< PotsCfuFeature >::Instance()->Shutdown(level);
   Singleton< PotsCwtFeature >::Instance()->Shutdown(level);
   Singleton< PotsCxfFeature >::Instance()->Shutdown(level);
   Singleton< PotsHtlFeature >::Instance()->Shutdown(level);
   Singleton< PotsSusFeature >::Instance()->Shutdown(level);
   Singleton< PotsTwcFeature >::Instance()->Shutdown(level);
   Singleton< PotsWmlFeature >::Instance()->Shutdown(level);
   Singleton< PotsFeatureRegistry >::Instance()->Shutdown(level);
   Singleton< PotsProfileRegistry >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name PbModule_Startup = "PbModule.Startup";

void PbModule::Startup(RestartLevel level)
{
   Debug::ft(PbModule_Startup);

   Singleton< PotsProfileRegistry >::Instance()->Startup(level);
   Singleton< PotsBicFeature >::Instance()->Startup(level);
   Singleton< PotsBocFeature >::Instance()->Startup(level);
   Singleton< PotsCfbFeature >::Instance()->Startup(level);
   Singleton< PotsCfnFeature >::Instance()->Startup(level);
   Singleton< PotsCfuFeature >::Instance()->Startup(level);
   Singleton< PotsCwtFeature >::Instance()->Startup(level);
   Singleton< PotsCxfFeature >::Instance()->Startup(level);
   Singleton< PotsHtlFeature >::Instance()->Startup(level);
   Singleton< PotsSusFeature >::Instance()->Startup(level);
   Singleton< PotsTwcFeature >::Instance()->Startup(level);
   Singleton< PotsWmlFeature >::Instance()->Startup(level);
   Singleton< PotsProtocol >::Instance()->Startup(level);

   //  Audit the features and create their CLI parameters.
   //  This is done before creating the POTS CLI increment.
   //
   if(level >= RestartReload)
   {
      Singleton< PotsFeatureRegistry >::Instance()->Audit();
   }

   Singleton< PotsIncrement >::Instance()->Startup(level);
}
}