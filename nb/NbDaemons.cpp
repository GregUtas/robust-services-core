//==============================================================================
//
//  NbDaemons.cpp
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
#include "NbDaemons.h"
#include "CliThread.h"
#include "Debug.h"
#include "DeferredThread.h"
#include "LogThread.h"
#include "ObjectPoolAudit.h"
#include "Singleton.h"
#include "StatisticsThread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string CliDaemonName = "cli";

//------------------------------------------------------------------------------

CliDaemon::CliDaemon() : Daemon(CliDaemonName, 1)
{
   Debug::ft("CliDaemon.ctor");
}

//------------------------------------------------------------------------------

CliDaemon::~CliDaemon()
{
   Debug::ftnt("CliDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* CliDaemon::CreateThread()
{
   Debug::ft("CliDaemon.CreateThread");
   return Singleton< CliThread >::Instance();
}

//------------------------------------------------------------------------------

AlarmStatus CliDaemon::GetAlarmLevel() const
{
   Debug::ft("CliDaemon.GetAlarmLevel");
   return CriticalAlarm;
}

//------------------------------------------------------------------------------

void CliDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string DeferredDaemonName = "defer";

//------------------------------------------------------------------------------

DeferredDaemon::DeferredDaemon() : Daemon(DeferredDaemonName, 1)
{
   Debug::ft("DeferredDaemon.ctor");
}

//------------------------------------------------------------------------------

DeferredDaemon::~DeferredDaemon()
{
   Debug::ftnt("DeferredDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* DeferredDaemon::CreateThread()
{
   Debug::ft("DeferredDaemon.CreateThread");
   return Singleton< DeferredThread >::Instance();
}

//------------------------------------------------------------------------------

AlarmStatus DeferredDaemon::GetAlarmLevel() const
{
   Debug::ft("DeferredDaemon.GetAlarmLevel");
   return CriticalAlarm;
}

//------------------------------------------------------------------------------

void DeferredDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string LogDaemonName = "log";

//------------------------------------------------------------------------------

LogDaemon::LogDaemon() : Daemon(LogDaemonName, 1)
{
   Debug::ft("LogDaemon.ctor");
}

//------------------------------------------------------------------------------

LogDaemon::~LogDaemon()
{
   Debug::ftnt("LogDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* LogDaemon::CreateThread()
{
   Debug::ft("LogDaemon.CreateThread");
   return Singleton< LogThread >::Instance();
}

//------------------------------------------------------------------------------

AlarmStatus LogDaemon::GetAlarmLevel() const
{
   Debug::ft("LogDaemon.GetAlarmLevel");
   return CriticalAlarm;
}

//------------------------------------------------------------------------------

void LogDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string ObjectDaemonName = "objaud";

//------------------------------------------------------------------------------

ObjectDaemon::ObjectDaemon() : Daemon(ObjectDaemonName, 1)
{
   Debug::ft("ObjectDaemon.ctor");
}

//------------------------------------------------------------------------------

ObjectDaemon::~ObjectDaemon()
{
   Debug::ftnt("ObjectDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* ObjectDaemon::CreateThread()
{
   Debug::ft("ObjectDaemon.CreateThread");
   return Singleton< ObjectPoolAudit >::Instance();
}

//------------------------------------------------------------------------------

void ObjectDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string StatisticsDaemonName = "stats";

//------------------------------------------------------------------------------

StatisticsDaemon::StatisticsDaemon() : Daemon(StatisticsDaemonName, 1)
{
   Debug::ft("StatisticsDaemon.ctor");
}

//------------------------------------------------------------------------------

StatisticsDaemon::~StatisticsDaemon()
{
   Debug::ftnt("StatisticsDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* StatisticsDaemon::CreateThread()
{
   Debug::ft("StatisticsDaemon.CreateThread");
   return Singleton< StatisticsThread >::Instance();
}

//------------------------------------------------------------------------------

void StatisticsDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}
}
