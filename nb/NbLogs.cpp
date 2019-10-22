//==============================================================================
//
//  NbLogs.cpp
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
#include "NbLogs.h"
#include "Debug.h"
#include "Log.h"
#include "LogGroup.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string NodeLogGroup = "NODE";
fixed_string SoftwareLogGroup = "SW";
fixed_string ConfigLogGroup = "CFG";
fixed_string StatsLogGroup = "STATS";
fixed_string ThreadLogGroup = "THR";
fixed_string ObjPoolLogGroup = "OBJ";

//------------------------------------------------------------------------------

fn_name NodeBase_CreateNbLogs = "NodeBase.CreateNbLogs";

void CreateNbLogs(RestartLevel level)
{
   Debug::ft(NodeBase_CreateNbLogs);

   if(level <= RestartWarm) return;

   auto group = new LogGroup(NodeLogGroup, "Node");
   new Log(group, NodeInitTimeout, "Initialization timeout");
   new Log(group, NodeSchedTimeout, "Scheduling timeout");
   new Log(group, NodeNoSymbolInfo, "Symbol information not loaded");
   new Log(group, NodeRestart, "Node restart");
   new Log(group, NodeRunning, "Node running");

   group = new LogGroup(SoftwareLogGroup, "Debugging");
   new Log(group, SoftwareError, "Software error");

   group = new LogGroup(ConfigLogGroup, "Configuration Parameters");
   new Log(group, ConfigFileNotFound, "Configuration file not found");
   new Log(group, ConfigKeyInvalid, "Configuration key invalid");
   new Log(group, ConfigKeyInUse, "Configuration key already in use");
   new Log(group, ConfigValueInvalid, "Configuration value invalid");
   new Log(group, ConfigValueMissing, "Configuration value not found");
   new Log(group, ConfigExtraIgnored, "Configuration extra input ignored");

   group = new LogGroup(StatsLogGroup, "Statistics");
   new Log(group, StatsReport, "Statistics report");

   group = new LogGroup(ThreadLogGroup, "Threads");
   new Log(group, ThreadCriticalDeath, "Death of critical thread");
   new Log(group, ThreadDeleted, "Thread deleted");
   new Log(group, ThreadExited, "Thread exited");
   new Log(group, ThreadException, "Exception");
   new Log(group, ThreadSignalRaised, "SIgnal raised");
   new Log(group, ThreadSignalReraised, "Signal reraised");
   new Log(group, ThreadYielded, "Thread yielded");
   new Log(group, ThreadForcedToExit, "Thread forced to exit");

   group = new LogGroup(ObjPoolLogGroup, "Object Pools");
   new Log(group, ObjPoolExpansionFailed, "Object pool expansion failed");
   new Log(group, ObjPoolBlocksInUse, "Object pool blocks in use");
   new Log(group, ObjPoolExpanded, "Object pool size expanded");
   new Log(group, ObjPoolQueueCorrupt, "Object pool queue corrupt");
   new Log(group, ObjPoolQueueCount, "Object pool queue count incorrect");
   new Log(group, ObjPoolBlockRecovered, "Object pool block recovered");
   new Log(group, ObjPoolBlocksRecovered, "Object pool blocks recovered");
}
}