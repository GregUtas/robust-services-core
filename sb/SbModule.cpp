//==============================================================================
//
//  SbModule.cpp
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
#include "SbModule.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "InvokerPoolRegistry.h"
#include "NbAppIds.h"
#include "NwModule.h"
#include "ProtocolRegistry.h"
#include "SbIncrement.h"
#include "SbInvokerPools.h"
#include "SbLogs.h"
#include "SbPools.h"
#include "SbTracer.h"
#include "ServiceRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TimerProtocol.h"
#include "TimerRegistry.h"
#include "TimerThread.h"

using namespace NetworkBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
bool SbModule::Registered = Register();

fn_name SbModule_Register = "SbModule.Register";

//------------------------------------------------------------------------------

fn_name SbModule_ctor = "SbModule.ctor";

SbModule::SbModule() : Module(SbModuleId)
{
   Debug::ft(SbModule_ctor);
}

//------------------------------------------------------------------------------

fn_name SbModule_dtor = "SbModule.dtor";

SbModule::~SbModule()
{
   Debug::ft(SbModule_dtor);
}

//------------------------------------------------------------------------------

void SbModule::Patch(sel_t selector, void* arguments)
{
   Module::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool SbModule::Register()
{
   Debug::ft(SbModule_Register);

   //  Create the modules required by SessionBase.
   //
   Singleton< NwModule >::Instance();
   Singleton< SbModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name SbModule_Shutdown = "SbModule.Shutdown";

void SbModule::Shutdown(RestartLevel level)
{
   Debug::ft(SbModule_Shutdown);

   Singleton< SbIncrement >::Instance()->Shutdown(level);
   Singleton< SbTracer >::Instance()->Shutdown(level);
   Singleton< TimerProtocol >::Instance()->Shutdown(level);
   Singleton< PayloadInvokerPool >::Instance()->Shutdown(level);

   Singleton< SbIpBufferPool >::Instance()->Shutdown(level);
   Singleton< ContextPool >::Instance()->Shutdown(level);
   Singleton< MsgPortPool >::Instance()->Shutdown(level);
   Singleton< MessagePool >::Instance()->Shutdown(level);
   Singleton< ProtocolSMPool >::Instance()->Shutdown(level);
   Singleton< TimerPool >::Instance()->Shutdown(level);
   Singleton< ServiceSMPool >::Instance()->Shutdown(level);
   Singleton< EventPool >::Instance()->Shutdown(level);
   Singleton< BtIpBufferPool >::Instance()->Shutdown(level);

   Singleton< TimerRegistry >::Instance()->Shutdown(level);
   Singleton< FactoryRegistry >::Instance()->Shutdown(level);
   Singleton< ServiceRegistry >::Instance()->Shutdown(level);
   Singleton< ProtocolRegistry >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name SbModule_Startup = "SbModule.Startup";

void SbModule::Startup(RestartLevel level)
{
   Debug::ft(SbModule_Startup);

   CreateSbLogs(level);

   Singleton< ProtocolRegistry >::Instance()->Startup(level);
   Singleton< ServiceRegistry >::Instance()->Startup(level);
   Singleton< FactoryRegistry >::Instance()->Startup(level);
   Singleton< TimerRegistry >::Instance()->Startup(level);

   Singleton< SbIpBufferPool >::Instance()->Startup(level);
   Singleton< ContextPool >::Instance()->Startup(level);
   Singleton< MsgPortPool >::Instance()->Startup(level);
   Singleton< MessagePool >::Instance()->Startup(level);
   Singleton< ProtocolSMPool >::Instance()->Startup(level);
   Singleton< TimerPool >::Instance()->Startup(level);
   Singleton< ServiceSMPool >::Instance()->Startup(level);
   Singleton< EventPool >::Instance()->Startup(level);
   Singleton< BtIpBufferPool >::Instance()->Startup(level);

   Singleton< PayloadInvokerPool >::Instance()->Startup(level);
   Singleton< TimerProtocol >::Instance()->Startup(level);
   Singleton< SbTracer >::Instance()->Startup(level);
   Singleton< SbIncrement >::Instance()->Startup(level);

   //  Start threads.
   //
   Singleton< TimerThread >::Instance()->Startup(level);
   Singleton< InvokerPoolRegistry >::Instance()->Startup(level);
}
}
