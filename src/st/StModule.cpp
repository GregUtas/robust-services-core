//==============================================================================
//
//  StModule.cpp
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
#include "StModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "NtModule.h"
#include "SbAppIds.h"
#include "SbModule.h"
#include "Singleton.h"
#include "StIncrement.h"
#include "SymbolRegistry.h"
#include "TestSessions.h"

using namespace NodeTools;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
StModule::StModule() : Module()
{
   Debug::ft("StModule.ctor");

   //  Create the modules required by SessionTools.
   //
   Singleton<SbModule>::Instance();
   Singleton<NtModule>::Instance();
   Singleton<ModuleRegistry>::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

StModule::~StModule()
{
   Debug::ftnt("StModule.dtor");
}

//------------------------------------------------------------------------------

void StModule::Enable()
{
   Debug::ft("StModule.Enable");

   Singleton<SbModule>::Instance()->Enable();
   Singleton<NtModule>::Instance()->Enable();
   Module::Enable();
}

//------------------------------------------------------------------------------

void StModule::Shutdown(RestartLevel level)
{
   Debug::ft("StModule.Shutdown");
}

//------------------------------------------------------------------------------

void StModule::Startup(RestartLevel level)
{
   Debug::ft("StModule.Startup");

   Singleton<TestProtocol>::Instance()->Startup(level);
   Singleton<TestService>::Instance()->Startup(level);
   Singleton<TestFactory>::Instance()->Startup(level);
   Singleton<StIncrement>::Instance()->Startup(level);

   //  Define symbols.
   //
   auto reg = Singleton<SymbolRegistry>::Instance();
   reg->BindSymbol("factory.test", TestFactoryId);
}
}
