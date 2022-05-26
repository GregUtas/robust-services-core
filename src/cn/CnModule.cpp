//==============================================================================
//
//  CnModule.cpp
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
#include "CnModule.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "SbModule.h"
#include "Singleton.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace ControlNode
{
CnModule::CnModule() : Module()
{
   Debug::ft("CnModule.ctor");

   //  Create the modules required by ControlNode.
   //
   Singleton<SbModule>::Instance();
   Singleton<ModuleRegistry>::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

CnModule::~CnModule()
{
   Debug::ftnt("CnModule.dtor");
}

//------------------------------------------------------------------------------

void CnModule::Shutdown(RestartLevel level)
{
   Debug::ft("CnModule.Shutdown");
}

//------------------------------------------------------------------------------

void CnModule::Startup(RestartLevel level)
{
   Debug::ft("CnModule.Startup");
}
}
