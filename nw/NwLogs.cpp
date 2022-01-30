//==============================================================================
//
//  NwLogs.cpp
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
#include "NwLogs.h"
#include <sstream>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "Debug.h"
#include "Log.h"
#include "LogGroup.h"
#include "Singleton.h"
#include "SysSocket.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Tracks whether the network layer is up.
//
static bool NetworkIsUp_ = true;

//  Raises or clears, based on STATUS, the alarm identified by NAME, and
//  whose status change is communicated by generating a log with ID that
//  also displays ERR.
//
static void UpdateAlarm(c_string name,
   LogId id, AlarmStatus status, const string& err);

//------------------------------------------------------------------------------

fixed_string NetworkLogGroup = "NET";
fixed_string NetInitAlarmName = "NETINIT";
fixed_string LocAddrAlarmName = "LOCADDR";
fixed_string NetworkAlarmName = "NETWORK";

void CreateNwLogs(RestartLevel level)
{
   Debug::ft("NetworkBase.CreateNwLogs");

   if(level >= RestartCold)
   {
      //  During a cold restart or higher, all I/O threads exit, and we
      //  deregister and reregister as a user of the network layer.  Mark
      //  the network as available for now--if it isn't, we will find out
      //  when registration fails or an I/O thread cannot allocate a socket.
      //
      NetworkIsUp_ = true;
   }

   if(level < RestartReboot) return;

   new Alarm(NetInitAlarmName, "Network layer not initialized", 5);
   new Alarm(NetworkAlarmName, "Network access lost", 5);
   new Alarm(LocAddrAlarmName, "Local address unreachable", 5);

   auto group = new LogGroup(NetworkLogGroup, "Network Layer");
   new Log(group, NetworkStartupFailure, "Network startup failure");
   new Log(group, NetworkUnavailable, "Network is unavailable");
   new Log(group, NetworkPortOccupied, "IP port already occupied");
   new Log(group, NetworkServiceFailure, "Network service has failed");
   new Log(group, NetworkAllocFailure, "Network allocation failure");
   new Log(group, NetworkFunctionError, "Network function error");
   new Log(group, NetworkLocalAddrFailure, "Local address failure");
   new Log(group, NetworkAvailable, "Network is available");
   new Log(group, NetworkServiceAvailable, "Network service is available");
   new Log(group, NetworkStartupSuccess, "Network startup successful");
   new Log(group, NetworkLocalAddrSuccess, "Local address verified");
   new Log(group, NetworkSocketError, "Socket function error");
   new Log(group, NetworkNoDestination, "No destination from input handler");
}

//------------------------------------------------------------------------------

void NetworkIsUp()
{
   if(NetworkIsUp_) return;

   Debug::ft("NetworkBase.NetworkIsUp");

   UpdateAlarm(NetworkAlarmName, NetworkAvailable, NoAlarm, EMPTY_STR);
}

//------------------------------------------------------------------------------

void OutputNwLog(LogId id, c_string func, nwerr_t errval, c_string extra)
{
   Debug::ft("NetworkBase.OutputNwLog");

   //  Suppress all network logs when the network layer is down.
   //
   if(!NetworkIsUp_) return;

   auto log = Log::Create(NetworkLogGroup, id);

   if(log != nullptr)
   {
      *log << Log::Tab << func << ": errval=" << errval << extra;
      Log::Submit(log);
   }

   string name(SysSocket::AlarmName(errval));
   if(name.empty()) return;
   id = (name == NetInitAlarmName ? NetworkStartupFailure : NetworkUnavailable);
   UpdateAlarm(name.c_str(), id, CriticalAlarm, std::to_string(errval));
}

//------------------------------------------------------------------------------

bool ReportLayerStart(const string& err)
{
   Debug::ft("NetworkBase.ReportLayerStart");

   auto ok = err.empty();
   auto status = (ok ? NoAlarm : CriticalAlarm);
   auto id = (ok ? NetworkStartupSuccess : NetworkStartupFailure);
   UpdateAlarm(NetInitAlarmName, id, status, err);
   return ok;
}

//------------------------------------------------------------------------------

static void UpdateAlarm
   (c_string name, LogId id, AlarmStatus status, const string& err)
{
   if(status != NoAlarm)
   {
      if(!NetworkIsUp_) return;
   }
   else
   {
      if(NetworkIsUp_) return;
   }

   auto reg = Singleton< AlarmRegistry >::Instance();
   auto alarm = reg->Find(name);

   if(alarm != nullptr)
   {
      auto log = alarm->Create(NetworkLogGroup, id, status);

      if(log != nullptr)
      {
         if(!err.empty()) *log << Log::Tab << "errval=" << err;
         Log::Submit(log);
      }
   }

   NetworkIsUp_ = (status == NoAlarm ? true : false);
}
}
