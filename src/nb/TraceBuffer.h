//==============================================================================
//
//  TraceBuffer.h
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
#ifndef TRACEBUFFER_H_INCLUDED
#define TRACEBUFFER_H_INCLUDED

#include "Permanent.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include "NbTypes.h"
#include "SysDecls.h"
#include "SystemTime.h"
#include "SysTypes.h"
#include "ToolTypes.h"

namespace NodeBase
{
   class TraceRecord;
   class FunctionTrace;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Buffer for saving trace records on behalf of trace tools.  Trace tools
//  exist because breakpoint debugging cannot be used in the field.  There
//  are a number of trace tools, each of which captures different events.
//  They are controlled by commands available in CLI increments, and they
//  record debug information by defining TraceRecord subclasses.
//
//  The function tracer, for example, supports detailed debugging:
//
//    >include all on    // capture all activity
//    >set tools f on    // enable tracing of function calls
//    >start             // start tracing
//    run some scenario
//    >stop              // stop tracing
//    >save trace <fn>   // display function calls in "<fn>.trace.txt"
//
class TraceBuffer : public Permanent
{
   friend class Singleton<TraceBuffer>;
public:
   //  Deleted to prohibit copying.
   //
   TraceBuffer(const TraceBuffer& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   TraceBuffer& operator=(const TraceBuffer& that) = delete;

   //> The minimum and maximum sizes of the buffer (in log2).
   //
   static const size_t MinSize;
   static const size_t MaxSize;

   //  A string indicating that no item was selected for tracing.
   //
   static fixed_string NoneSelected;

   //  Enables or disables the tool identified by TID based on VALUE.
   //
   TraceRc SetTool(FlagId tid, bool value);

   //  Returns true if the tool identified by TID is enabled.  This does NOT
   //  imply that tracing is active: that is determined by Debug::TraceOn.
   //
   bool ToolIsOn(FlagId tid) const { return tools_.test(tid); }

   //  Returns a read-only reference to the set of tool flags.
   //
   const Flags& GetTools() const { return tools_; }

   //  Sets all tool flags to TOOLS.
   //
   TraceRc SetTools(const Flags& tools);

   //  Displays, in STREAM, the setting of each tool, and whether tracing
   //  has started.
   //
   void QueryTools(std::ostream& stream) const;

   //  Disables all tools.
   //
   TraceRc ClearTools();

   //  Sets the flag that indicates that something of type FILTER is
   //  included or excluded.
   //
   void SetFilter(FlagId filter) { filters_.set(filter); }

   //  Clears the flag that indicates that something of type FILTER is
   //  included or excluded.
   //
   void ClearFilter(FlagId filter) { filters_.reset(filter); }

   //  Returns true if FILTER is enabled.
   //
   bool FilterIsOn(FlagId filter) const { return filters_.test(filter); }

   //  Traces all activity if ON is true.
   //
   TraceRc SelectAll(bool on);

   //  Sets the size of the buffer so that it can hold 2^N records.
   //
   TraceRc SetSize(size_t n);

   //  Controls whether the buffer will wrap around when full.
   //
   TraceRc SetWrap(bool wrap);

   //  Returns true if the buffer is empty.
   //
   bool Empty() const;

   //  Displays, in STREAM, the buffer's current status.
   //
   void Query(std::ostream& stream) const;

   //  Deletes all trace records in the buffer.
   //
   TraceRc Clear();

   //  Initiates tracing by all enabled tools using OPTS, which is
   //  currently unused.
   //
   TraceRc StartTracing(const std::string& opts);

   //  Invoked when stopping tracing.
   //
   void StopTracing();

   //  Adds RECORD to the buffer.  Deletes RECORD and returns false if
   //  the buffer is locked.
   //
   bool Insert(TraceRecord* record);

   //  Reserves memory for a FunctionTrace instance.
   //
   void* AddFunction();

   //  Moves SECOND to the slot that precedes FIRST.
   //
   void MoveAbove(TraceRecord* second, const TraceRecord* first) const;

   //  Displays all of the records in the trace buffer.  STREAM must be
   //  valid unless an immediate trace is in progress.
   //
   TraceRc DisplayTrace(std::ostream* stream, const std::string& opts);

   //  Displays status information before trace records are displayed.
   //
   void DisplayStart(std::ostream& stream) const;

   //  Returns the time (full) when tracing started.
   //
   const SystemTime::Point& StartTime() const { return startTime_; }

   //  Returns a string specifying when tracing started, followed by this
   //  element's name.
   //
   std::string strTimePlace() const;

   //  A table for counting the number of times each function was invoked.
   //
   typedef std::map<fn_name_arg, size_t> InvocationsTable;

   //  Increments the number of times that FUNC was invoked.
   //
   void RecordInvocation(fn_name_arg func) const;

   //  Returns the invocations database.
   //
   const InvocationsTable& GetInvocations() const { return *invocations_; }

   //  Updates CURR to reference the next record in the buffer.  If CURR
   //  is nullptr, it is set to the buffer's first record.  CURR is set to
   //  nullptr when the end of the buffer is reached.  MASK specifies which
   //  type(s) of record(s) to look for (constants are defined in Tool.h).
   //
   void Next(TraceRecord*& curr, const Flags& mask) const;

   //  Returns the FunctionTrace record for the most recent function recorded
   //  on the thread identified by NID.
   //
   const FunctionTrace* LastFunction(SysThreadId nid) const;

   //  Returns the depth of the most recent destructor recorded on the thread
   //  identified by NID.
   //
   fn_depth LastDtorDepth(SysThreadId nid) const;

   //  Returns the record in SLOT.
   //
   TraceRecord* At(size_t slot) const { return buff_[slot]; }

   //  Invoked before using the Next function for iteration.  This allows new
   //  records to be added to the buffer but prevents an existing record from
   //  being purged to make room for a new one, which could cause a trap while
   //  iterating.
   //
   void Lock();

   //  Unlocks the trace buffer after Lock() has been used for iteration.
   //
   void Unlock();

   //  Returns true if the buffer has been processed, else marks it as having
   //  been processed and returns false.
   //
   bool HasBeenProcessed();

   //  Marks objects held by trace buffer records as being in use.
   //
   void ClaimBlocks() override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;
private:
   //  Creates a buffer of MinSize.  Private because this is a singleton.
   //
   TraceBuffer();

   //  Private because this is a singleton.
   //
   ~TraceBuffer();

   //  A tuple containing a function's name and how many times it was invoked.
   //
   typedef std::pair<fn_name_arg, size_t> FunctionCount;

   //  Allocates space for recording 2^N trace records.  Returns false if
   //  allocation fails.
   //
   bool AllocBuffers(size_t n);

   //  Allocates the next available slot for a TraceRecord subclass.  Returns
   //  UINT32_MAX if no more slots are available or the buffer is locked.
   //
   uint32_t AllocSlot();

   //  Flags that indicate which trace tools are enabled.
   //
   Flags tools_;

   //  Flags that indicate which filters are enabled.
   //
   Flags filters_;

   //  Buffer for a sequence of trace records.
   //
   TraceRecord** buff_;

   //  Buffer for FunctionTrace records, to avoid the overhead of a heap
   //  or object pool.
   //
   FunctionTrace* funcs_;

   //  The current size of buff_ and funcs_.
   //
   uint32_t size_;

   //  The next available slot in buff_.
   //
   std::atomic_uint32_t bnext_;

   //  The next available slot in funcs_.
   //
   std::atomic_uint32_t fnext_;

   //  Set if the buffer should wrap around when full.
   //
   bool wrap_;

   //  Set if the buffer wrapped around (overflow).
   //
   bool ovfl_;

   //  Blocks the creation of a new record if greater than zero.
   //
   std::atomic_uint8_t softLocks_;

   //  The output stream if immediate tracing is being performed.
   //
   ostreamPtr stream_;

   //  The full clock time when tracing started.
   //
   SystemTime::Point startTime_;

   //  The number of times that locking blocked the creation of a buffer entry.
   //
   size_t blocks_;

   //  The table for recording the number of times that a function was invoked.
   //
   std::unique_ptr<InvocationsTable> invocations_;

   //  Set when FunctionTrace.Process is invoked to reorder constructors.
   //
   bool processed_;
};
}
#endif
