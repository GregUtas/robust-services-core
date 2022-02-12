//==============================================================================
//
//  NbTypes.cpp
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
#include "NbTypes.h"
#include <sstream>
#include "Debug.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
const Flags VerboseOpt = Flags(1 << DispVerbose);

//------------------------------------------------------------------------------

fixed_string AlarmSymbols[AlarmStatus_N + 1] =
{
   "    ",  // NoAlarm
   "  * ",  // MinorAlarm
   " ** ",  // MajorAlarm
   "*** ",  // CriticalAlarm
   "  ? "   // AlarmStatus_N
};

c_string AlarmStatusSymbol(AlarmStatus status)
{
   if((status >= 0) && (status < AlarmStatus_N)) return AlarmSymbols[status];
   return AlarmSymbols[AlarmStatus_N];
}

fixed_string AlarmStatusStrings[AlarmStatus_N + 1] =
{
   "NoAlarm",
   "Minor",
   "Major",
   "Critical",
   ERROR_STR
};

//------------------------------------------------------------------------------
//
//  The initial character in the following strings must be
//  unique to support BlockingReasonChar.
//
fixed_string BlockingReasonStrings[BlockingReason_N + 1] =
{
   "ready",     // NotBlocked
   "clock",     // BlockedOnClock
   "network",   // BlockedOnNetwork
   "stream",    // BlockedOnStream
   "database",  // BlockedOnDatabase
   "???"        // BlockingReason_N
};

char BlockingReasonChar(BlockingReason reason)
{
   std::ostringstream stream;
   stream << reason;
   return stream.str().front();
}

//------------------------------------------------------------------------------

fixed_string FactionStrings[Faction_N + 1] =
{
   "Idle",
   "Audit",
   "Background",
   "Operations",
   "Maintenance",
   "Payload",
   "LoadTest",
   "System",
   "Watchdog",
   ERROR_STR
};

char FactionChar(Faction faction)
{
   if((faction >= 0) && (faction < Faction_N))
      return FactionStrings[faction][0];
   return '?';
}

//------------------------------------------------------------------------------

LogType GetLogType(LogId id)
{
   Debug::ftnt("NodeBase.GetLogType");

   if((id >= TroubleLog) && (id <= TroubleLog + 99)) return TroubleLog;
   if((id >= ThresholdLog) && (id <= ThresholdLog + 99)) return ThresholdLog;
   if((id >= StateLog) && (id <= StateLog + 99)) return StateLog;
   if((id >= PeriodicLog) && (id <= PeriodicLog + 99)) return PeriodicLog;
   if((id >= InfoLog) && (id <= InfoLog + 199)) return InfoLog;
   if((id >= MiscLog) && (id <= MiscLog + 199)) return MiscLog;
   if((id >= DebugLog) && (id <= DebugLog + 99)) return DebugLog;
   return LogType_N;
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, AlarmStatus status)
{
   if((status >= 0) && (status < AlarmStatus_N))
      stream << AlarmStatusStrings[status];
   else
      stream << AlarmStatusStrings[AlarmStatus_N];
   return stream;
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, BlockingReason reason)
{
   if((reason >= 0) && (reason < BlockingReason_N))
      stream << BlockingReasonStrings[reason];
   else
      stream << BlockingReasonStrings[BlockingReason_N];
   return stream;
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, Faction faction)
{
   if((faction >= 0) && (faction < Faction_N))
      stream << FactionStrings[faction];
   else
      stream << FactionStrings[Faction_N];
   return stream;
}
}
