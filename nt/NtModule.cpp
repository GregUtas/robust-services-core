//==============================================================================
//
//  NtModule.cpp
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
#include "NtModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "NbAppIds.h"
#include "NbModule.h"
#include "NtIncrement.h"
#include "Singleton.h"
#include "SymbolRegistry.h"
#include "TestDatabase.h"

//------------------------------------------------------------------------------

namespace NodeTools
{
NtModule::NtModule() : Module()
{
   Debug::ft("NtModule.ctor");

   //  Create the modules required by NodeTools.
   //
   Singleton< NbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

NtModule::~NtModule()
{
   Debug::ftnt("NtModule.dtor");
}

//------------------------------------------------------------------------------

void NtModule::Shutdown(RestartLevel level)
{
   Debug::ft("NtModule.Shutdown");

   auto testdb = Singleton< TestDatabase >::Extant();
   if(testdb != nullptr) testdb->Shutdown(level);
}

//------------------------------------------------------------------------------

void NtModule::Startup(RestartLevel level)
{
   Debug::ft("NtModule.Startup");

   Singleton< NtIncrement >::Instance()->Startup(level);

   //  Define symbols.
   //
   auto reg = Singleton< SymbolRegistry >::Instance();
   reg->BindSymbol("flag.disablerootthread", DisableRootThread);
   reg->BindSymbol("flag.reenterthread", ThreadReenterFlag);
   reg->BindSymbol("flag.recovertrap", ThreadRecoverTrapFlag);
   reg->BindSymbol("flag.threadctortrap", ThreadCtorTrapFlag);
   reg->BindSymbol("flag.threadctorretrap", ThreadCtorRetrapFlag);
   reg->BindSymbol("flag.threadretrap", ThreadRetrapFlag);
   reg->BindSymbol("flag.threaddtortrap", ThreadDtorTrapFlag);
}
}
