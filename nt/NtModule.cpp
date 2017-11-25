//==============================================================================
//
//  NtModule.cpp
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
