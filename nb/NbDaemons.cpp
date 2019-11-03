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
fixed_string CliDaemon_ctor = "CliDaemon.ctor";
fixed_string CliDaemon_dtor = "CliDaemon.dtor";
fixed_string CliDaemon_CreateThread = "CliDaemon.CreateThread";

CliDaemon::CliDaemon() : Daemon(CliDaemonName, 1)
{
   Debug::ft(CliDaemon_ctor);
}

CliDaemon::~CliDaemon()
{
   Debug::ft(CliDaemon_dtor);
}

Thread* CliDaemon::CreateThread()
{
   Debug::ft(CliDaemon_CreateThread);
   return Singleton< CliThread >::Instance();
}

//------------------------------------------------------------------------------

fixed_string ObjectDaemonName = "objaud";
fixed_string ObjectDaemon_ctor = "ObjectDaemon.ctor";
fixed_string ObjectDaemon_dtor = "ObjectDaemon.dtor";
fixed_string ObjectDaemon_CreateThread = "ObjectDaemon.CreateThread";

ObjectDaemon::ObjectDaemon() : Daemon(ObjectDaemonName, 1)
{
   Debug::ft(ObjectDaemon_ctor);
}

ObjectDaemon::~ObjectDaemon()
{
   Debug::ft(ObjectDaemon_dtor);
}

Thread* ObjectDaemon::CreateThread()
{
   Debug::ft(ObjectDaemon_CreateThread);
   return Singleton< ObjectPoolAudit >::Instance();
}

//------------------------------------------------------------------------------

fixed_string StatisticsDaemonName = "stats";
fixed_string StatisticsDaemon_ctor = "StatisticsDaemon.ctor";
fixed_string StatisticsDaemon_dtor = "StatisticsDaemon.dtor";
fixed_string StatisticsDaemon_CreateThread = "StatisticsDaemon.CreateThread";

StatisticsDaemon::StatisticsDaemon() : Daemon(StatisticsDaemonName, 1)
{
   Debug::ft(StatisticsDaemon_ctor);
}

StatisticsDaemon::~StatisticsDaemon()
{
   Debug::ft(StatisticsDaemon_dtor);
}

Thread* StatisticsDaemon::CreateThread()
{
   Debug::ft(StatisticsDaemon_CreateThread);
   return Singleton< StatisticsThread >::Instance();
}
}
