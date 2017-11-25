//==============================================================================
//
//  CnModule.cpp
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
#include "CnModule.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "SbModule.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace ControlNode
{
bool CnModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name CnModule_ctor = "CnModule.ctor";

CnModule::CnModule() : Module(CnModuleId)
{
   Debug::ft(CnModule_ctor);
}

//------------------------------------------------------------------------------

fn_name CnModule_dtor = "CnModule.dtor";

CnModule::~CnModule()
{
   Debug::ft(CnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name CnModule_Register = "CnModule.Register";

bool CnModule::Register()
{
   Debug::ft(CnModule_Register);

   //  Include the modules required by ControlNode.
   //
   Singleton< SbModule >::Instance();
   Singleton< CnModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name CnModule_Shutdown = "CnModule.Shutdown";

void CnModule::Shutdown(RestartLevel level)
{
   Debug::ft(CnModule_Shutdown);
}

//------------------------------------------------------------------------------

fn_name CnModule_Startup = "CnModule.Startup";

void CnModule::Startup(RestartLevel level)
{
   Debug::ft(CnModule_Startup);
}
}