//==============================================================================
//
//  NbModule.cpp
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
#include "NbModule.h"
#include "AlarmRegistry.h"
#include "CfgParmRegistry.h"
#include "CinThread.h"
#include "ClassRegistry.h"
#include "CliRegistry.h"
#include "CliThread.h"
#include "CoutThread.h"
#include "DaemonRegistry.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "FileThread.h"
#include "InitFlags.h"
#include "LogBufferRegistry.h"
#include "LogGroupRegistry.h"
#include "LogThread.h"
#include "Memory.h"
#include "ModuleRegistry.h"
#include "NbIncrement.h"
#include "NbLogs.h"
#include "NbPools.h"
#include "ObjectPoolAudit.h"
#include "ObjectPoolRegistry.h"
#include "PosixSignalRegistry.h"
#include "Singleton.h"
#include "Singletons.h"
#include "StatisticsRegistry.h"
#include "StatisticsThread.h"
#include "SymbolRegistry.h"
#include "SysThreadStack.h"
#include "ThisThread.h"
#include "ThreadAdmin.h"
#include "ThreadRegistry.h"
#include "TraceBuffer.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
NbModule::NbModule() : Module()
{
   Debug::ft("NbModule.ctor");

   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

NbModule::~NbModule()
{
   Debug::ftnt("NbModule.dtor");
}

//------------------------------------------------------------------------------

void NbModule::Patch(sel_t selector, void* arguments)
{
   Module::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void NbModule::Shutdown(RestartLevel level)
{
   Debug::ft("NbModule.Shutdown");

   Singleton< NbIncrement >::Instance()->Shutdown(level);
   Singleton< SymbolRegistry >::Instance()->Shutdown(level);
   Singleton< CliRegistry >::Instance()->Shutdown(level);
   Singleton< Element >::Instance()->Shutdown(level);
   Singleton< ClassRegistry >::Instance()->Shutdown(level);
   Singleton< ThreadAdmin >::Instance()->Shutdown(level);
   Singleton< ThreadRegistry >::Instance()->Shutdown(level);
   Singleton< ObjectPoolRegistry >::Instance()->Shutdown(level);
   Singleton< DaemonRegistry >::Instance()->Shutdown(level);
   Singleton< CfgParmRegistry >::Instance()->Shutdown(level);
   Singleton< LogGroupRegistry >::Instance()->Shutdown(level);
   Singleton< AlarmRegistry >::Instance()->Shutdown(level);
   Singleton< StatisticsRegistry >::Instance()->Shutdown(level);
   Singleton< LogBufferRegistry >::Instance()->Shutdown(level);
   Singleton< PosixSignalRegistry >::Instance()->Shutdown(level);

   Singleton< TraceBuffer >::Instance()->Shutdown(level);
   SysThreadStack::Shutdown(level);
   Memory::Shutdown();
   Singletons::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

void NbModule::Startup(RestartLevel level)
{
   Debug::ft("NbModule.Startup");

   //  Create/start singletons.  Some of these already exist as a
   //  result of creating RootThread, but their Startup functions
   //  must be invoked.
   //
   Singleton< PosixSignalRegistry >::Instance()->Startup(level);
   Singleton< LogBufferRegistry >::Instance()->Startup(level);
   Singleton< StatisticsRegistry >::Instance()->Startup(level);
   Singleton< AlarmRegistry >::Instance()->Startup(level);
   Singleton< LogGroupRegistry >::Instance()->Startup(level);
   CreateNbLogs(level);
   Singleton< CfgParmRegistry >::Instance()->Startup(level);
   Singleton< DaemonRegistry >::Instance()->Startup(level);
   Singleton< ObjectPoolRegistry >::Instance()->Startup(level);
   Singleton< ThreadRegistry >::Instance()->Startup(level);
   Singleton< ThreadAdmin >::Instance()->Startup(level);
   Singleton< MsgBufferPool >::Instance()->Startup(level);
   Singleton< ClassRegistry >::Instance()->Startup(level);
   Singleton< Element >::Instance()->Startup(level);
   Singleton< CliRegistry >::Instance()->Startup(level);
   Singleton< SymbolRegistry >::Instance()->Startup(level);
   Singleton< NbIncrement >::Instance()->Startup(level);

   //  See if we're supposed to cause an initialization timeout.
   //
   if(InitFlags::CauseTimeout())
   {
      ThisThread::Pause(TIMEOUT_NEVER);
   }

   //  Create/start threads.
   //
   Singleton< FileThread >::Instance()->Startup(level);
   Singleton< CoutThread >::Instance()->Startup(level);
   Singleton< CinThread >::Instance()->Startup(level);
   Singleton< ObjectPoolAudit >::Instance()->Startup(level);
   Singleton< StatisticsThread >::Instance()->Startup(level);
   Singleton< LogThread >::Instance()->Startup(level);
   Singleton< CliThread >::Instance()->Startup(level);
}
}
