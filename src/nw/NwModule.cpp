//==============================================================================
//
//  NwModule.cpp
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
#include "NwModule.h"
#include "Debug.h"
#include "IpPortRegistry.h"
#include "IpServiceRegistry.h"
#include "LocalAddrTest.h"
#include "ModuleRegistry.h"
#include "NbModule.h"
#include "NwIncrement.h"
#include "NwLogs.h"
#include "NwPools.h"
#include "NwTracer.h"
#include "Singleton.h"
#include "SysSocket.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
NwModule::NwModule() : Module()
{
   Debug::ft("NwModule.ctor");

   //  Create the modules required by NetworkBase.
   //
   Singleton<NbModule>::Instance();
   Singleton<ModuleRegistry>::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

NwModule::~NwModule()
{
   Debug::ftnt("NwModule.dtor");
}

//------------------------------------------------------------------------------

void NwModule::Patch(sel_t selector, void* arguments)
{
   Module::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void NwModule::Shutdown(RestartLevel level)
{
   Debug::ft("NwModule.Shutdown");

   Singleton<IpPortRegistry>::Instance()->Shutdown(level);
   Singleton<IpServiceRegistry>::Instance()->Shutdown(level);

   if(level >= RestartCold) SysSocket::StopLayer();
}

//------------------------------------------------------------------------------

void NwModule::Startup(RestartLevel level)
{
   Debug::ft("NwModule.Startup");

   CreateNwLogs(level);

   if(level >= RestartCold)
   {
      SysSocket::StartLayer();
   }

   Singleton<NwTracer>::Instance()->Startup(level);
   Singleton<IpServiceRegistry>::Instance()->Startup(level);
   Singleton<IpPortRegistry>::Instance()->Startup(level);
   Singleton<IpBufferPool>::Instance()->Startup(level);
   Singleton<TinyBufferPool>::Instance()->Startup(level);
   Singleton<SmallBufferPool>::Instance()->Startup(level);
   Singleton<MediumBufferPool>::Instance()->Startup(level);
   Singleton<LargeBufferPool>::Instance()->Startup(level);
   Singleton<HugeBufferPool>::Instance()->Startup(level);
   Singleton<SendLocalIpService>::Instance()->Startup(level);
   Singleton<SendLocalThread>::Instance()->Startup(level);
   Singleton<NwIncrement>::Instance()->Startup(level);
}
}
