//==============================================================================
//
//  StModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "StModule.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "NtModule.h"
#include "SbAppIds.h"
#include "SbModule.h"
#include "Singleton.h"
#include "StIncrement.h"
#include "SymbolRegistry.h"
#include "SysTypes.h"
#include "TestSessions.h"

using namespace NodeTools;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
bool StModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name StModule_ctor = "StModule.ctor";

StModule::StModule() : Module(StModuleId)
{
   Debug::ft(StModule_ctor);
}

//------------------------------------------------------------------------------

fn_name StModule_dtor = "StModule.dtor";

StModule::~StModule()
{
   Debug::ft(StModule_dtor);
}

//------------------------------------------------------------------------------

fn_name StModule_Register = "StModule.Register";

bool StModule::Register()
{
   Debug::ft(StModule_Register);

   //  Create the modules required by SessionTools.
   //
   Singleton< SbModule >::Instance();
   Singleton< NtModule >::Instance();
   Singleton< StModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name StModule_Shutdown = "StModule.Shutdown";

void StModule::Shutdown(RestartLevel level)
{
   Debug::ft(StModule_Shutdown);

   Singleton< StIncrement >::Instance()->Shutdown(level);
   Singleton< TestFactory >::Instance()->Shutdown(level);
   Singleton< TestService >::Instance()->Shutdown(level);
   Singleton< TestProtocol >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name StModule_Startup = "StModule.Startup";

void StModule::Startup(RestartLevel level)
{
   Debug::ft(StModule_Startup);

   Singleton< TestProtocol >::Instance()->Startup(level);
   Singleton< TestService >::Instance()->Startup(level);
   Singleton< TestFactory >::Instance()->Startup(level);
   Singleton< StIncrement >::Instance()->Startup(level);

   //  Define symbols.
   //
   if(level < RestartCold) return;

   auto reg = Singleton< SymbolRegistry >::Instance();
   reg->BindSymbol("factory.test", TestFactoryId);
}
}