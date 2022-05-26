//==============================================================================
//
//  SbModule.cpp
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
#include "SbModule.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "InvokerPoolRegistry.h"
#include "ModuleRegistry.h"
#include "NwModule.h"
#include "ProtocolRegistry.h"
#include "SbIncrement.h"
#include "SbInvokerPools.h"
#include "SbLogs.h"
#include "SbPools.h"
#include "SbTracer.h"
#include "ServiceRegistry.h"
#include "Singleton.h"
#include "TimerProtocol.h"
#include "TimerRegistry.h"
#include "TimerThread.h"

using namespace NetworkBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
SbModule::SbModule() : Module()
{
   Debug::ft("SbModule.ctor");

   //  Create the modules required by SessionBase.
   //
   Singleton<NwModule>::Instance();
   Singleton<ModuleRegistry>::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

SbModule::~SbModule()
{
   Debug::ftnt("SbModule.dtor");
}

//------------------------------------------------------------------------------

void SbModule::Patch(sel_t selector, void* arguments)
{
   Module::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void SbModule::Shutdown(RestartLevel level)
{
   Debug::ft("SbModule.Shutdown");

   Singleton<TimerRegistry>::Instance()->Shutdown(level);
   Singleton<FactoryRegistry>::Instance()->Shutdown(level);
   Singleton<ServiceRegistry>::Instance()->Shutdown(level);
   Singleton<ProtocolRegistry>::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

void SbModule::Startup(RestartLevel level)
{
   Debug::ft("SbModule.Startup");

   CreateSbLogs(level);

   Singleton<ProtocolRegistry>::Instance()->Startup(level);
   Singleton<ServiceRegistry>::Instance()->Startup(level);
   Singleton<FactoryRegistry>::Instance()->Startup(level);
   Singleton<TimerRegistry>::Instance()->Startup(level);

   Singleton<SbIpBufferPool>::Instance()->Startup(level);
   Singleton<ContextPool>::Instance()->Startup(level);
   Singleton<MsgPortPool>::Instance()->Startup(level);
   Singleton<MessagePool>::Instance()->Startup(level);
   Singleton<ProtocolSMPool>::Instance()->Startup(level);
   Singleton<TimerPool>::Instance()->Startup(level);
   Singleton<ServiceSMPool>::Instance()->Startup(level);
   Singleton<EventPool>::Instance()->Startup(level);
   Singleton<BtIpBufferPool>::Instance()->Startup(level);

   Singleton<TimerProtocol>::Instance()->Startup(level);
   Singleton<SbTracer>::Instance()->Startup(level);
   Singleton<SbIncrement>::Instance()->Startup(level);

   //  Create thread pools.
   //
   Singleton<PayloadInvokerPool>::Instance();

   //  Start threads.
   //
   Singleton<TimerThread>::Instance()->Startup(level);
   Singleton<InvokerPoolRegistry>::Instance()->Startup(level);
}
}
