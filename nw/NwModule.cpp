//==============================================================================
//
//  NwModule.cpp
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
#include "NwModule.h"
#include "Debug.h"
#include "IpBuffer.h"
#include "IpPortRegistry.h"
#include "IpServiceRegistry.h"
#include "ModuleRegistry.h"
#include "NbModule.h"
#include "NwIncrement.h"
#include "NwLogs.h"
#include "NwTracer.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysSocket.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name NwModule_ctor = "NwModule.ctor";

NwModule::NwModule() : Module()
{
   Debug::ft(NwModule_ctor);

   //  Create the modules required by NetworkBase.
   //
   Singleton< NbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

fn_name NwModule_dtor = "NwModule.dtor";

NwModule::~NwModule()
{
   Debug::ft(NwModule_dtor);
}

//------------------------------------------------------------------------------

void NwModule::Patch(sel_t selector, void* arguments)
{
   Module::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name NwModule_Shutdown = "NwModule.Shutdown";

void NwModule::Shutdown(RestartLevel level)
{
   Debug::ft(NwModule_Shutdown);

   Singleton< NwIncrement >::Instance()->Shutdown(level);
   Singleton< IpBufferPool >::Instance()->Shutdown(level);
   Singleton< IpPortRegistry >::Instance()->Shutdown(level);
   Singleton< IpServiceRegistry >::Instance()->Shutdown(level);
   Singleton< NwTracer >::Instance()->Shutdown(level);

   if(level >= RestartCold) SysSocket::StopLayer();
}

//------------------------------------------------------------------------------

fn_name NwModule_Startup = "NwModule.Startup";

void NwModule::Startup(RestartLevel level)
{
   Debug::ft(NwModule_Startup);

   CreateNwLogs();

   if((level >= RestartCold) && !SysSocket::StartLayer())
   {
      Restart::Initiate(SocketLayerUnavailable, 0);
   }

   Singleton< NwTracer >::Instance()->Startup(level);
   Singleton< IpServiceRegistry >::Instance()->Startup(level);
   Singleton< IpPortRegistry >::Instance()->Startup(level);
   Singleton< IpBufferPool >::Instance()->Startup(level);
   Singleton< NwIncrement >::Instance()->Startup(level);
}
}
