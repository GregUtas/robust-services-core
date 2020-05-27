//==============================================================================
//
//  NbDaemons.cpp
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
#include "NbDaemons.h"
#include "CliThread.h"
#include "Debug.h"
#include "ObjectPoolAudit.h"
#include "Singleton.h"
#include "StatisticsThread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string CliDaemonName = "cli";

//------------------------------------------------------------------------------

fn_name CliDaemon_ctor = "CliDaemon.ctor";

CliDaemon::CliDaemon() : Daemon(CliDaemonName, 1)
{
   Debug::ft(CliDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name CliDaemon_dtor = "CliDaemon.dtor";

CliDaemon::~CliDaemon()
{
   Debug::ftnt(CliDaemon_dtor);
}

//------------------------------------------------------------------------------

fn_name CliDaemon_CreateThread = "CliDaemon.CreateThread";

Thread* CliDaemon::CreateThread()
{
   Debug::ft(CliDaemon_CreateThread);
   return Singleton< CliThread >::Instance();
}

//------------------------------------------------------------------------------

fn_name CliDaemon_GetAlarmLevel = "CliDaemon.GetAlarmLevel";

AlarmStatus CliDaemon::GetAlarmLevel() const
{
   Debug::ft(CliDaemon_GetAlarmLevel);
   return CriticalAlarm;
}

//------------------------------------------------------------------------------

void CliDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string LogDaemonName = "log";

//------------------------------------------------------------------------------

fn_name LogDaemon_ctor = "LogDaemon.ctor";

LogDaemon::LogDaemon() : Daemon(LogDaemonName, 1)
{
   Debug::ft(LogDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name LogDaemon_dtor = "LogDaemon.dtor";

LogDaemon::~LogDaemon()
{
   Debug::ftnt(LogDaemon_dtor);
}

//------------------------------------------------------------------------------

fn_name LogDaemon_CreateThread = "LogDaemon.CreateThread";

Thread* LogDaemon::CreateThread()
{
   Debug::ft(LogDaemon_CreateThread);
   return Singleton< CliThread >::Instance();
}

//------------------------------------------------------------------------------

fn_name LogDaemon_GetAlarmLevel = "LogDaemon.GetAlarmLevel";

AlarmStatus LogDaemon::GetAlarmLevel() const
{
   Debug::ft(LogDaemon_GetAlarmLevel);
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

fn_name ObjectDaemon_ctor = "ObjectDaemon.ctor";

ObjectDaemon::ObjectDaemon() : Daemon(ObjectDaemonName, 1)
{
   Debug::ft(ObjectDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name ObjectDaemon_dtor = "ObjectDaemon.dtor";

ObjectDaemon::~ObjectDaemon()
{
   Debug::ftnt(ObjectDaemon_dtor);
}

//------------------------------------------------------------------------------

fn_name ObjectDaemon_CreateThread = "ObjectDaemon.CreateThread";

Thread* ObjectDaemon::CreateThread()
{
   Debug::ft(ObjectDaemon_CreateThread);
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

fn_name StatisticsDaemon_ctor = "StatisticsDaemon.ctor";

StatisticsDaemon::StatisticsDaemon() : Daemon(StatisticsDaemonName, 1)
{
   Debug::ft(StatisticsDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name StatisticsDaemon_dtor = "StatisticsDaemon.dtor";

StatisticsDaemon::~StatisticsDaemon()
{
   Debug::ftnt(StatisticsDaemon_dtor);
}

//------------------------------------------------------------------------------

fn_name StatisticsDaemon_CreateThread = "StatisticsDaemon.CreateThread";

Thread* StatisticsDaemon::CreateThread()
{
   Debug::ft(StatisticsDaemon_CreateThread);
   return Singleton< StatisticsThread >::Instance();
}

//------------------------------------------------------------------------------

void StatisticsDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}
}
