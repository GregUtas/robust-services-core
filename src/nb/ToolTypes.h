//==============================================================================
//
//  ToolTypes.h
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
#ifndef TOOLTYPES_H_INCLUDED
#define TOOLTYPES_H_INCLUDED

#include <iosfwd>
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  Definitions for debug tools.
//
namespace NodeBase
{
//  How an item is currently selected by a trace tool.
//
enum TraceStatus : int16_t
{
   TraceDefault,   // item has neither been included nor excluded
   TraceExcluded,  // item has been excluded
   TraceIncluded,  // item has been included
   TraceStatus_N   // number of trace statuses
};

//  Inserts a string for STATUS into STREAM.
//
std::ostream& operator<<(std::ostream& stream, TraceStatus status);

//  Outcomes of trace tool functions.
//
enum TraceRc
{
   TraceOk,            // success
   AlreadyStarted,     // tracing has already started
   BufferAllocFailed,  // could not allocate a trace buffer
   RegistryIsFull,     // registry for selected item is full
   NoSuchItem,         // selected item does not exist
   NoBufferAllocated,  // tracing cannot start without a buffer
   NoItemSelected,     // tracing cannot start without choosing what to trace
   NoToolSelected,     // tracing cannot start without choosing a trace tool
   NotWhileTracing,    // command not allowed while tracing is in progress
   BufferNotEmpty,     // operation not allowed if the buffer has entries
   BufferEmpty,        // nothing captured during tracing
   CouldNotOpenFile,   // could not create file to generate report
   NothingToDisplay,   // could not find any trace records relevant to report
   NotInField,         // operation is not allowed in the field
   TraceFailed,        // operation failed for some other reason
   TraceRc_N           // number of trace return codes
};

//  Returns a string that explains RC.
//
c_string strTraceRc(TraceRc rc);

//  Trace tool identifiers.
//
constexpr FlagId ToolBuffer = 1;       // internal use
constexpr FlagId FunctionTracer = 2;   // function calls
constexpr FlagId MemoryTracer = 3;     // memory allocations/deallocations
constexpr FlagId ObjPoolTracer = 4;    // pooled objects
constexpr FlagId NetworkTracer = 5;    // socket events
constexpr FlagId TransTracer = 6;      // SessionBase transactions
constexpr FlagId BufferTracer = 7;     // SessionBase IpBuffers
constexpr FlagId ContextTracer = 8;    // SessionBase contexts
constexpr FlagId ParserTracer = 9;     // Parser "object code generation"
constexpr FlagId FirstAppTracer = 10;  // start of application-specific tracers
}
#endif
