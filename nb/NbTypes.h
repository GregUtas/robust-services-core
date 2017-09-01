//==============================================================================
//
//  NbTypes.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

extern const Flags Vb_Mask;  // used in Flags(Vb_Mask) to set DispVerbose

//  Reasons for thread blocking.
//
enum BlockingReason
{
   NotBlocked,         // running or ready to run
   BlockedOnClock,     // SysThread::Delay (non-zero time)
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
   PayloadFaction,      // session processing
   SystemFaction,       // InitThread
   WatchdogFaction,     // RootThread
   Faction_N            // number of factions
};

//  Inserts a string for FACTION into STREAM.
//
std::ostream& operator<<(std::ostream& stream, Faction faction);

//  Returns a character that identifies FACTION.
//
char FactionChar(Faction faction);

//  The length of a message's payload in bytes.
//
typedef uint16_t MsgSize;

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

//  An identifier for an object pool.
//
typedef uint8_t ObjectPoolId;

//  An identifier for an object block.
//
typedef id_t PooledObjectId;

//  A sequence number for an object block.
//
typedef uint8_t PooledObjectSeqNo;

//  Forward declarations of classes whose instances are typically
//  owned by a unique_ptr.
//
class CfgBoolParm;
class CfgFileTimeParm;
class CfgFlagParm;
class CfgIntParm;
class CfgStrParm;
class Counter;
class Accumulator;
class HighWatermark;
class LowWatermark;
class StatisticsGroup;

typedef std::unique_ptr< CfgBoolParm > CfgBoolParmPtr;
typedef std::unique_ptr< CfgFileTimeParm > CfgFileTimeParmPtr;
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
template< typename T > class Singleton;

//  Versions of std::string that support the various MemTypes.  See the
//  comments in Allocators.h.
//
typedef std::char_traits<char> CharTraits;
typedef std::basic_string<char, CharTraits, DynAllocator<char>>  DynString;
typedef std::basic_string<char, CharTraits, ImmAllocator<char>>  ImmString;
typedef std::basic_string<char, CharTraits, PermAllocator<char>> PermString;
typedef std::basic_string<char, CharTraits, ProtAllocator<char>> ProtString;
typedef std::basic_string<char, CharTraits, TempAllocator<char>> TempString;
}
#endif
