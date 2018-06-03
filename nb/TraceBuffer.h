//==============================================================================
//
//  TraceBuffer.h
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
#ifndef TRACEBUFFER_H_INCLUDED
#define TRACEBUFFER_H_INCLUDED

#include "Permanent.h"
#include <atomic>
#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include "NbTypes.h"
#include "SysTime.h"
#include "SysTypes.h"
#include "ToolTypes.h"

namespace NodeBase
{
   class TraceRecord;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Buffer for saving trace records on behalf of trace tools.  Trace tools
//  exist because breakpoint debugging cannot be used in the field.  There
//  are a number of trace tools, each of which captures different events.
//  They are controlled by commands available in CLI increments, and they
//  record debug information by defining TraceRecord subclasses, which add
//  themselves to the trace buffer.
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
   friend class Singleton< TraceBuffer >;
public:
   //> The maximum size of the buffer (in bytes).
   //
   static const size_t MaxSize;

   //> A string indicating that the buffer overflowed.
   //
   static fixed_string OverflowStr;

   //> A string indicating that no item was selected for tracing.
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

   //  Sets the size of the buffer to KBS kilobytes.
   //
   TraceRc SetSize(size_t kbs);

   //  Returns true if the buffer is empty.
   //
   bool Empty() const;

   //  Returns the number of functions that were not traced because the
   //  buffer was locked.
   //
   size_t Blocks() const { return blocks_; }

   //  Returns true if the buffer overflowed.
   //
   bool HasOverflowed() const { return ovfl_; }

   //  Displays, in STREAM, the buffer's size and how full it is.
   //
   void Query(std::ostream& stream) const;

   //  Deletes all trace records in the buffer.
   //
   TraceRc Clear();

   //  Initiates tracing by all enabled tools.  If IMMEDIATE is set, each
   //  trace record is immediately written to the file "immed.trace.txt"
   //  as it is generated.
   //
   TraceRc StartTrace(bool immediate);

   //  Invoked when stopping tracing.
   //
   void StopTrace();

   //  Displays all of the records in the trace buffer.  STREAM must be
   //  valid unless an immediate trace is in progress.
   //
   TraceRc DisplayTrace(std::ostream* stream, bool diff);

   //  Returns true if trace records are being output immediately.
   //
   bool ImmediateTraceOn() const { return immediate_; }

   //  Returns the time (full) when tracing started.
   //
   const SysTime& StartTimeFull() const { return startTime_; }

   //  Returns a string specifying when tracing started, followed by this
   //  element's name.
   //
   std::string strTimePlace() const;

   //  A table for counting the number of times each function was invoked.
   //
   typedef std::map< fn_name_arg, size_t > InvocationsTable;

   //  Increments the number of times that FUNC was invoked.
   //
   void RecordInvocation(fn_name_arg func) const;

   //  Returns the invocations database.
   //
   const InvocationsTable& GetInvocations() const { return *invocations_; }

   //  Reserves space in the buffer for a record of size nBytes and returns
   //  a pointer to where the record can be placed.
   //
   void* AddRecord(size_t nBytes);

   //  Updates RECORD to reference the next record in the buffer.  If RECORD
   //  is nullptr, it is set to the buffer's first record.  RECORD is set to
   //  nullptr when the end of the buffer is reached.  MASK specifies which
   //  type(s) of record(s) to look for (constants are defined in Tool.h).
   //
   void Next(TraceRecord*& record, const Flags& mask) const;

   //  Returns the last record added to the trace buffer.
   //
   const TraceRecord* LastRecord() const { return lastRecord_; }

   //  Returns the depth of the most recent destructor.
   //
   fn_depth LastDtorDepth() const { return dtorDepth_; }

   //  Sets the depth of the most recent destructor.
   //
   void SetLastDtorDepth(fn_depth depth) { dtorDepth_ = depth; }

   //  Invoked before using the Next function for iteration.  This allows new
   //  records to be added to the buffer but prevents an existing record from
   //  being purged to make room for a new one, which could cause a trap while
   //  iterating.
   //
   void Lock();

   //  Unlocks the trace buffer after Lock() has been used for iteration.
   //
   void Unlock();

   //  Returns true if the trace buffer is blocked from adding a new record.
   //
   bool IsLocked();

   //  Marks objects held by trace buffer records as being in use.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates a buffer of InitialSize.  Private because this singleton is
   //  not subclassed.
   //
   TraceBuffer();

   //  Private because this singleton is not subclassed.
   //
   ~TraceBuffer();

   //  Deleted to prohibit copying.
   //
   TraceBuffer(const TraceBuffer& that) = delete;
   TraceBuffer& operator=(const TraceBuffer& that) = delete;

   //  A tuple containing a function's name and how many times it was invoked.
   //
   typedef std::pair< fn_name_arg, size_t > FunctionCount;

   //  Returns the maximum number of records that the trace buffer could
   //  currently contain.
   //
   size_t MaxRecords() const;

   //  Displays records when immediate tracing is active.
   //
   void ImmediateDisplay();

   //  Deletes trace records from start_ to END to make room for the next
   //  record.
   //
   void PurgeRecords(size_t end);

   //> The initial size of the buffer (in bytes).
   //
   static const size_t InitialSize;

   //> The value written after the last record added to the buffer.
   //
   static const uword EndMarker;

   //  Flags that indicate which trace tools are enabled.
   //
   Flags tools_;

   //  Flags that indicate which filters are enabled.
   //
   Flags filters_;

   //  Buffer for trace records (acts as a *circular* buffer).
   //
   uword* buff_;

   //  The current size of buff_, *minus* space for an end-of-buffer marker.
   //
   size_t size_;

   //  The offset of the first record in buff_.
   //
   size_t start_;

   //  The offset after the last record in buff_.
   //
   size_t end_;

   //  Set if the buffer wrapped around (overflow).
   //
   bool ovfl_;

   //  The most recent trace record added to the buffer.
   //
   TraceRecord* lastRecord_;

   //  The depth of the most recent destructor.
   //
   fn_depth dtorDepth_;

   //  Blocks the creation of a new record while one is being constructed.
   //
   std::atomic_bool hardLock_;

   //  Blocks the creation of a new record if it would delete an existing one.
   //
   std::atomic_uint8_t softLocks_;

   //  Set if each trace record should be output immediately.
   //
   bool immediate_;

   //  The output stream if immediate tracing is being performed.
   //
   ostreamPtr stream_;

   //  The full clock time when tracing started.
   //
   SysTime startTime_;

   //  The number of times that locking blocked the creation of a buffer entry.
   //
   size_t blocks_;

   //  The table for recording the number of times that a function was invoked.
   //
   std::unique_ptr< InvocationsTable > invocations_;
};
}
#endif
