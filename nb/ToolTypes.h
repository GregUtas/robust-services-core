//==============================================================================
//
//  ToolTypes.h
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
#ifndef TOOLTYPES_H_INCLUDED
#define TOOLTYPES_H_INCLUDED

#include <iosfwd>

//------------------------------------------------------------------------------
//
//  Definitions for debug tools.
//
namespace NodeBase
{
//  How an item is currently selected by a trace tool.
//
enum TraceStatus
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
const char* strTraceRc(TraceRc rc);

//  Trace tool identifiers.  A new trace tool must define an entry here.
//
enum ToolIds
{
   ToolBuffer = 1,      // internal use
   FunctionTracer = 2,  // function calls
   MemoryTracer = 3,    // memory allocations/deallocations
   ObjPoolTracer = 4,   // pooled objects
   NetworkTracer = 5,   // socket events
   TransTracer = 6,     // SessionBase transactions
   BufferTracer = 7,    // SessionBase IpBuffers
   ContextTracer = 8,   // SessionBase contexts
   ParserTracer = 9     // parser "object code generation"
};
}
#endif