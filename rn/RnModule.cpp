//==============================================================================
//
//  RnModule.cpp
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
#include "RnModule.h"
#include "CbModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace RoutingNode
{
fn_name RnModule_ctor = "RnModule.ctor";

RnModule::RnModule() : Module()
{
   Debug::ft(RnModule_ctor);

   //  Create the modules required by RoutingNode.
   //
   Singleton< CbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

fn_name RnModule_dtor = "RnModule.dtor";

RnModule::~RnModule()
{
   Debug::ftnt(RnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name RnModule_Shutdown = "RnModule.Shutdown";

void RnModule::Shutdown(RestartLevel level)
{
   Debug::ft(RnModule_Shutdown);
}

//------------------------------------------------------------------------------

fn_name RnModule_Startup = "RnModule.Startup";

void RnModule::Startup(RestartLevel level)
{
   Debug::ft(RnModule_Startup);
}
}
