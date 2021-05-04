//==============================================================================
//
//  OnModule.cpp
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
#include "OnModule.h"
#include "CnModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "PbModule.h"
#include "Singleton.h"

using namespace ControlNode;
using namespace PotsBase;

//------------------------------------------------------------------------------

namespace OperationsNode
{
OnModule::OnModule() : Module()
{
   Debug::ft("OnModule.ctor");

   //  Create the modules required by OperationsNode.
   //
   Singleton< CnModule >::Instance();
   Singleton< PbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

OnModule::~OnModule()
{
   Debug::ftnt("OnModule.dtor");
}

//------------------------------------------------------------------------------

void OnModule::Shutdown(RestartLevel level)
{
   Debug::ft("OnModule.Shutdown");
}

//------------------------------------------------------------------------------

void OnModule::Startup(RestartLevel level)
{
   Debug::ft("OnModule.Startup");
}
}
