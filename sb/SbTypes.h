//==============================================================================
//
//  SbTypes.h
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
#ifndef SBTYPES_H_INCLUDED
#define SBTYPES_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <memory>
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  A limited set of SessionBase types are defined here to insulate classes
//  that only need to use these definitions.  "Global" means that a type is
//  unique across all SessionBase applications, whereas "local" means that
//  it is unique only in a restricted context.
//
namespace SessionBase
{
//  Protocol identifier (global).
//
typedef uint16_t ProtocolId;

//  Signal identifier (local to a protocol).
//
typedef uint16_t SignalId;

//  Parameter identifier (local to a protocol).
//
typedef uint16_t ParameterId;

//  Factory identifier (global).
//
typedef uint16_t FactoryId;

//  Service identifier (global).
//
typedef uint16_t ServiceId;

//  Service port identifier (local to a service).  A service uses
//  these to distinguish roles of ProtocolSMs.
//
typedef NodeBase::id_t ServicePortId;

//> Highest valid service port identifier.
//
constexpr ServicePortId MaxServicePortId = 11;

//  Event identifier (local to a service).
//
typedef uint16_t EventId;

//  State identifier (local to a service).
//
typedef uint16_t StateId;

//  Event handler identifier (local to a service).
//
typedef uint16_t EventHandlerId;

//  Trigger identifier (local to a service).
//
typedef uint16_t TriggerId;

//  Timer identifier.  Each application defines its own identifiers to
//  distinguish the timers that it uses.  NIL_ID can actually be used
//  as a TimerId, but applications generally use it to indicate that
//  no timer is running.
//
typedef uint16_t TimerId;

//  Types of processing contexts that factories can use.
//
enum ContextType
{
   SingleMsg,     // MsgContext: stream of independent messages (stateless)
   SinglePort,    // PsmContext: single PSM or stack (stateful)
   MultiPort,     // SsmContext: root SSM + multiple PSMs or stacks (stateful)
   ContextType_N  // number of context types
};

//  Returns a string for TYPE.
//
NodeBase::c_string strContextType(ContextType type);

//  Inserts a string for TYPE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, ContextType type);

//  Message priorities.
//
typedef uint8_t MsgPriority;

constexpr MsgPriority INGRESS = 0;      // from user starting a new session
constexpr MsgPriority EGRESS = 1;       // to user receiving a new session
constexpr MsgPriority PROGRESS = 2;     // to an existing session
constexpr MsgPriority IMMEDIATE = 3;    // between SSMs serving same user
constexpr MsgPriority MAX_PRIORITY = 3;

//  Returns a string for displaying PRIO.
//
NodeBase::c_string strMsgPriority(MsgPriority prio);

//  Object pool users.  This is used as a parameter to overrides of
//  operator new in order to support segregated pools that allocate
//  identical objects.
//
enum SbPoolUser
{
   PayloadUser,  // payload applications
   ToolUser      // trace tool applications
};

//  Forward declarations.
//
class Context;
class Event;
class Factory;
class Initiator;
class InvokerThread;
class Message;
class MsgPort;
class ProtocolLayer;
class ProtocolSM;
class RootServiceSM;
class SbIpBuffer;
class Service;
class SsmContext;
class State;
class Timer;
class Trigger;
struct MsgHeader;

typedef std::unique_ptr< SbIpBuffer > SbIpBufferPtr;

//  Test session identifier, used by InjectCommand and VerifyCommand.
//
typedef NodeBase::id_t TestSessionId;

//  Used by functions that support VerifyCommand.
//
struct SkipInfo
{
   SignalId first;  // first signal skipped
   int count;       // total signals skipped
};
}
#endif
