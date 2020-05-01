//==============================================================================
//
//  AnModule.cpp
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
#include "AnModule.h"
#include "AnIncrement.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "PbModule.h"
#include "PotsShelf.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace PotsBase;

//------------------------------------------------------------------------------

namespace AccessNode
{
fn_name AnModule_ctor = "AnModule.ctor";

AnModule::AnModule() : Module()
{
   Debug::ft(AnModule_ctor);

   //  Create the modules required by AccessNode.
   //
   Singleton< PbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

fn_name AnModule_dtor = "AnModule.dtor";

AnModule::~AnModule()
{
   Debug::ft(AnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name AnModule_Shutdown = "AnModule.Shutdown";

void AnModule::Shutdown(RestartLevel level)
{
   Debug::ft(AnModule_Shutdown);
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
