//==============================================================================
//
//  NbTypes.h
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
#ifndef NBTYPES_H_INCLUDED
#define NBTYPES_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include "Allocators.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  A limited set of NodeBase types are defined here to insulate classes
//  that only need to use these definitions.
//
namespace NodeBase
{
//  Options for the Display function.
//
enum DisplayOptions
{
   DispVerbose,  // full object display
   DispOption_N  // number of reasons; can be used to extend this enum
};

extern const Flags VerboseOpt;  // flag with DispVerbose set

//  Reasons for thread blocking.
//
enum BlockingReason : uint8_t
{
   NotBlocked,         // running or ready to run
   BlockedOnClock,     // SysThread::Delay
   BlockedOnNetwork,   // SysUdpSocket::Recvfrom or SysTcpSocket::Poll
   BlockedOnConsole,   // CinThread::GetLine (console)
   BlockedOnDatabase,  // in-memory database
   BlockingReason_N    // number of reasons
};

//  Inserts a string for REASON into STREAM.
//
std::ostream& operator<<(std::ostream& stream, BlockingReason reason);

//  Returns a character that identifies REASON.
//
char BlockingReasonChar(BlockingReason reason);

//  Scheduler factions.  Under proportional scheduling, threads in the
//  same faction share the same "pie slices".
//
enum Faction
{
   IdleFaction,         // idle thread (not used)
   AuditFaction,        // corrective audits
   BackgroundFaction,   // generating reports
   OperationsFaction,   // CLI, provisioning
   MaintenanceFaction,  // shelf management
   PayloadFaction,      // applications for end users
   LoadTestFaction,     // load generator for stress testing
   SystemFaction,       // InitThread
   WatchdogFaction,     // RootThread
   Faction_N            // number of factions
};

//  A set of flags that indicates which factions can be scheduled.
//
typedef std::bitset< Faction_N > FactionFlags;

//  Inserts a string for FACTION into STREAM.
//
std::ostream& operator<<(std::ostream& stream, Faction faction);

//  Returns a character that identifies FACTION.
//
char FactionChar(Faction faction);

//  Types of logs.  Each LogId (see below) should be defined using one
//  of these enumerators plus an offset.
//
enum LogType
{
   TroubleLog = 100,    // 100-199: fault; intervention may be possible
   ThresholdLog = 200,  // 200-299: level reached or exceeded
   StateLog = 300,      // 300-399: state change or progress update
   PeriodicLog = 400,   // 400-499: automatic report
   InfoLog = 500,       // 500-699: no intervention required
   MiscLog = 700,       // 700-899: other types of logs
   DebugLog = 900,      // 900-999: to help debug software
   LogType_N = 0        // illegal value
};

//  Alarm levels.
//
enum AlarmStatus
{
   NoAlarm,        // alarm off
   MinorAlarm,     // narrow degradation/outage
   MajorAlarm,     // broader degradation/outage
   CriticalAlarm,  // widespread degradation/outage
   AlarmStatus_N   // number of alarm statuses
};

//  Inserts a string for STATUS into STREAM.  The string is 4 characters wide,
//  contains only asterisks and spaces, and ends with a space.
//
std::ostream& operator<<(std::ostream& stream, AlarmStatus status);

//  Returns a 4-character string that corresponds to STATUS.
//
fixed_string AlarmStatusSymbol(AlarmStatus status);

//  The direction of a message.
//
enum MsgDirection
{
   MsgIncoming,
   MsgOutgoing
};

//  Various functions return a std::streamsize to indicate how many characters
//  were transferred from (to) an input (output) stream.  Returning a positive
//  value indicates success; the following values are used to report an error.
//
enum StreamRc
{
   StreamRestart = -6,    // use of stream not allowed during a restart
   StreamInterrupt = -5,  // client interrupted before input was received
   StreamInUse = -4,      // stream is already in use
   StreamFailure = -3,    // stream's failbit was set
   StreamBadChar = -2,    // stream contains an invalid character
   StreamEof = -1,        // reached end of input stream
   StreamEmpty = 0,       // buffer is empty (e.g. a bare \0, \n, or Enter key)
   StreamOk = 1           // reports success when a size is not required
};

//  An identifier for a module.
//
typedef uint16_t ModuleId;

//  An identifier for a thread.
//
typedef uint16_t ThreadId;

//  An identifier for a log.
//
typedef uint16_t LogId;

//  Returns the type of log associated with ID.
//
LogType GetLogType(LogId id);

//  An identifier for a trace record.
//
typedef uint8_t TraceRecordId;

//  An identifier for an object pool.
//
typedef uint8_t ObjectPoolId;

//  An identifier for an object block.
//
typedef uint32_t PooledObjectId;

//  A sequence number for an object block.
//
typedef uint8_t PooledObjectSeqNo;

//  Forward declarations of classes whose instances are typically
//  owned by a unique_ptr.
//
class CfgBoolParm;
class CfgFlagParm;
class CfgIntParm;
class CfgStrParm;
class Counter;
class Accumulator;
class HighWatermark;
class LowWatermark;
class StatisticsGroup;

typedef std::unique_ptr< CfgBoolParm > CfgBoolParmPtr;
typedef std::unique_ptr< CfgFlagParm > CfgFlagParmPtr;
typedef std::unique_ptr< CfgIntParm > CfgIntParmPtr;
typedef std::unique_ptr< CfgStrParm > CfgStrParmPtr;
typedef std::unique_ptr< Counter > CounterPtr;
typedef std::unique_ptr< Accumulator > AccumulatorPtr;
typedef std::unique_ptr< HighWatermark > HighWatermarkPtr;
typedef std::unique_ptr< LowWatermark > LowWatermarkPtr;
typedef std::unique_ptr< StatisticsGroup > StatisticsGroupPtr;

//  Forward declarations of templates.
//
template< class T > class Singleton;

//  Versions of std::string that support the various MemTypes.  See the
//  comments in Allocators.h.  There is no PermanentString, as it would
//  be equivalent to std::string.
//
using CharTraits = std::char_traits<char>;

using DynamicStr =
   std::basic_string<char, CharTraits, DynamicAllocator<char>>;
using ImmutableStr =
   std::basic_string<char, CharTraits, ImmutableAllocator<char>>;
using PersistentStr =
   std::basic_string<char, CharTraits, PersistentAllocator<char>>;
using ProtectedStr =
   std::basic_string<char, CharTraits, ProtectedAllocator<char>>;
using TemporaryStr =
   std::basic_string<char, CharTraits, TemporaryAllocator<char>>;

using ProtectedStrPtr = std::unique_ptr< ProtectedStr >;
}
#endif
