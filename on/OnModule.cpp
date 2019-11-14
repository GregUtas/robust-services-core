//==============================================================================
//
//  OnModule.cpp
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
#include "OnModule.h"
#include "CnModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "PbModule.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace ControlNode;
using namespace PotsBase;

//------------------------------------------------------------------------------

namespace OperationsNode
{
fn_name OnModule_ctor = "OnModule.ctor";

OnModule::OnModule() : Module()
{
   Debug::ft(OnModule_ctor);

   //  Create the modules required by OperationsNode.
   //
   Singleton< CnModule >::Instance();
   Singleton< PbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

fn_name OnModule_dtor = "OnModule.dtor";

OnModule::~OnModule()
{
   Debug::ft(OnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name OnModule_Shutdown = "OnModule.Shutdown";

void OnModule::Shutdown(RestartLevel level)
{
   Debug::ft(OnModule_Shutdown);
}

//------------------------------------------------------------------------------

fn_name OnModule_Startup = "OnModule.Startup";

void OnModule::Startup(RestartLevel level)
{
   Debug::ft(OnModule_Startup);
}
}
