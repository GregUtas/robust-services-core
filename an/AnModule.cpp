//==============================================================================
//
//  AnModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "AnModule.h"
#include "AnIncrement.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "PbModule.h"
#include "PotsShelf.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace PotsBase;

//------------------------------------------------------------------------------

namespace AccessNode
{
bool AnModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name AnModule_ctor = "AnModule.ctor";

AnModule::AnModule() : Module(AnModuleId)
{
   Debug::ft(AnModule_ctor);
}

//------------------------------------------------------------------------------

fn_name AnModule_dtor = "AnModule.dtor";

AnModule::~AnModule()
{
   Debug::ft(AnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name AnModule_Register = "AnModule.Register";

bool AnModule::Register()
{
   Debug::ft(AnModule_Register);

   //  Create the modules required by AccessNode.
   //
   Singleton< PbModule >::Instance();
   Singleton< AnModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name AnModule_Shutdown = "AnModule.Shutdown";

void AnModule::Shutdown(RestartLevel level)
{
   Debug::ft(AnModule_Shutdown);

   Singleton< AnIncrement >::Instance()->Shutdown(level);
   Singleton< PotsShelfIpService >::Instance()->Shutdown(level);
   Singleton< PotsShelfFactory >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name AnModule_Startup = "AnModule.Startup";

void AnModule::Startup(RestartLevel level)
{
   Debug::ft(AnModule_Startup);

   Singleton< PotsShelfFactory >::Instance()->Startup(level);
   Singleton< PotsShelfIpService >::Instance()->Startup(level);
   Singleton< AnIncrement >::Instance()->Startup(level);
}
}