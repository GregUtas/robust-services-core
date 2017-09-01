//==============================================================================
//
//  NtModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "NtModule.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "NbModule.h"
#include "NtIncrement.h"
#include "Singleton.h"
#include "SymbolRegistry.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeTools
{
bool NtModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name NtModule_ctor = "NtModule.ctor";

NtModule::NtModule() : Module(NtModuleId)
{
   Debug::ft(NtModule_ctor);
}

//------------------------------------------------------------------------------

fn_name NtModule_dtor = "NtModule.dtor";

NtModule::~NtModule()
{
   Debug::ft(NtModule_dtor);
}

//------------------------------------------------------------------------------

fn_name NtModule_Register = "NtModule.Register";

bool NtModule::Register()
{
   Debug::ft(NtModule_Register);

   //  Create the modules required by NodeTools.
   //
   Singleton< NbModule >::Instance();
   Singleton< NtModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name NtModule_Shutdown = "NtModule.Shutdown";

void NtModule::Shutdown(RestartLevel level)
{
   Debug::ft(NtModule_Shutdown);

   Singleton< NtIncrement >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name NtModule_Startup = "NtModule.Startup";

void NtModule::Startup(RestartLevel level)
{
   Debug::ft(NtModule_Startup);

   Singleton< NtIncrement >::Instance()->Startup(level);

   //  Define symbols.
   //
   if(level < RestartCold) return;

   auto reg = Singleton< SymbolRegistry >::Instance();
   reg->BindSymbol("flag.criticalthread", ThreadCriticalFlag);
   reg->BindSymbol("flag.threadctortrap", ThreadCtorTrapFlag);
   reg->BindSymbol("flag.recoverytrap", ThreadRecoveryTrapFlag);
   reg->BindSymbol("flag.showtoolprogress", ShowToolProgress);
}
}
