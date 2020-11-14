//==============================================================================
//
//  NwLogs.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "NwLogs.h"
#include "Alarm.h"
#include "Debug.h"
#include "Log.h"
#include "LogGroup.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fixed_string NetworkLogGroup = "NET";
fixed_string NetworkAlarmName = "NETWORK";

//------------------------------------------------------------------------------

void CreateNwLogs(RestartLevel level)
{
   Debug::ft("NetworkBase.CreateNwLogs");

   if(level < RestartReboot) return;

   new Alarm(NetworkAlarmName, "Network access lost", 5);

   auto group = new LogGroup(NetworkLogGroup, "Network Layer");
   new Log(group, NetworkStartupFailure, "Network startup failure");
   new Log(group, NetworkShutdownFailure, "Network shutdown failure");
   new Log(group, NetworkUnavailable, "Network unavailable");
   new Log(group, NetworkPortOccupied, "IP port already occupied");
   new Log(group, NetworkServiceFailure, "Network service has failed");
   new Log(group, NetworkAllocFailure, "Network allocation failure");
   new Log(group, NetworkFunctionError, "Network function error");
   new Log(group, NetworkAvailable, "Network available");
   new Log(group, NetworkServiceAvailable, "Network service is available");
   new Log(group, NetworkSocketError, "Socket function error");
   new Log(group, NetworkNoDestination, "No destination from input handler");
}
}
