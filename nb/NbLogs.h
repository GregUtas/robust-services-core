//==============================================================================
//
//  NbLogs.h
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
#ifndef NBLOGS_H_INCLUDED
#define NBLOGS_H_INCLUDED

#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
   //  Logs for NodeBase.
   //
   extern fixed_string NodeLogGroup;
   constexpr LogId NodeInitTimeout = TroubleLog;
   constexpr LogId NodeSchedTimeout = TroubleLog + 1;
   constexpr LogId NodeNoSymbolInfo = TroubleLog + 2;
   constexpr LogId NodeRestart = TroubleLog + 3;
   constexpr LogId NodeRunning = InfoLog;

   extern fixed_string SoftwareLogGroup;
   constexpr LogId SoftwareError = DebugLog;

   extern fixed_string ConfigLogGroup;
   constexpr LogId ConfigFileNotFound = TroubleLog;
   constexpr LogId ConfigKeyInvalid = TroubleLog + 1;
   constexpr LogId ConfigKeyInUse = TroubleLog + 2;
   constexpr LogId ConfigValueInvalid = TroubleLog + 3;
   constexpr LogId ConfigValueMissing = TroubleLog + 4;
   constexpr LogId ConfigExtraIgnored = InfoLog;

   extern fixed_string StatsLogGroup;
   constexpr LogId StatsReport = PeriodicLog;

   extern fixed_string ThreadLogGroup;
   constexpr LogId ThreadCriticalDeath = TroubleLog;
   constexpr LogId ThreadUnavailable = StateLog;
   constexpr LogId ThreadDeleted = DebugLog;
   constexpr LogId ThreadExited = DebugLog + 1;
   constexpr LogId ThreadException = DebugLog + 2;
   constexpr LogId ThreadSignalRaised = DebugLog + 3;
   constexpr LogId ThreadSignalReraised = DebugLog + 4;
   constexpr LogId ThreadYielded = DebugLog + 5;
   constexpr LogId ThreadForcedToExit = DebugLog + 6;
   constexpr LogId ThreadMutexesReleased = DebugLog + 7;

   extern fixed_string ObjPoolLogGroup;
   constexpr LogId ObjPoolExpansionFailed = TroubleLog;
   constexpr LogId ObjPoolBlocksInUse = ThresholdLog;
   constexpr LogId ObjPoolExpanded = StateLog;
   constexpr LogId ObjPoolQueueCorrupt = DebugLog;
   constexpr LogId ObjPoolQueueCount = DebugLog + 1;
   constexpr LogId ObjPoolBlockRecovered = DebugLog + 2;
   constexpr LogId ObjPoolBlocksRecovered = DebugLog + 3;

   void CreateNbLogs();
}
#endif
