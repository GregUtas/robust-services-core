//==============================================================================
//
//  TraceBuffer.cpp
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
#include "TraceBuffer.h"
#include "TraceRecord.h"
#include <memory>
#include <sstream>
#include "Debug.h"
#include "Element.h"
#include "InitFlags.h"
#include "Memory.h"
#include "Registry.h"
#include "Singleton.h"
#include "SysFile.h"
#include "ThisThread.h"
#include "Tool.h"
#include "ToolRegistry.h"
#include "TraceDump.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A trace record inserted by the trace buffer itself.
//
class BufferTrace : public TraceRecord
{
public:
   //  Types of internal trace records.
   //
   static const Id Resumed = 1;  // trace resumed after being stopped

   //  Constructs an invalid trace record during immediate tracing,
   //  pending the construction of a new trace record.
   //
   explicit BufferTrace(size_t size);

   //  Constructs a trace record to indicate when tracing resumed after
   //  being stopped.
   //
   BufferTrace();

   //  Overridden to display the trace record.
   //
   virtual bool Display(ostream& stream, bool diff) override;
};

//------------------------------------------------------------------------------

BufferTrace::BufferTrace(size_t size) : TraceRecord(size, ToolBuffer)
{
   rid_ = InvalidId;
}

//------------------------------------------------------------------------------

BufferTrace::BufferTrace() : TraceRecord(sizeof(BufferTrace), ToolBuffer)
{
   rid_ = Resumed;
}

//------------------------------------------------------------------------------

const string NilTraceStr("ERROR: invalid trace record");
const string ResumeTraceStr("BREAK OF TRACE " + string(65,'='));

bool BufferTrace::Display(ostream& stream, bool diff)
{
   switch(rid_)
   {
   case Resumed:
      stream << ResumeTraceStr;
      break;
   default:
      stream << NilTraceStr;
   }
   return true;
}

//==============================================================================

const size_t TraceBuffer::MaxSize = 0x04000000;      // 64MW
const size_t TraceBuffer::InitialSize = 0x00200000;  // 2MW
const uword TraceBuffer::EndMarker = 0;
fixed_string TraceBuffer::NoneSelected = "none";
fixed_string TraceBuffer::OverflowStr =
   "The buffer wrapped around. Older entries were lost.";

//------------------------------------------------------------------------------

TraceBuffer::TraceBuffer() :
   buff_(nullptr),
   size_(0),
   start_(0),
   end_(0),
   ovfl_(false),
   lastRecord_(nullptr),
   dtorDepth_(-1),
   hardLock_(false),
   softLocks_(0),
   immediate_(false),
   stream_(nullptr),
   blocks_(0)
{
   buff_ = (uword*) Memory::Alloc(InitialSize << BYTES_PER_WORD_LOG2, MemPerm);
   size_ = InitialSize;
   invocations_.reset(new InvocationsTable);

   if(InitFlags::TraceInit())
   {
      SetTool(FunctionTracer, true);
      SetFilter(TraceAll);
      StartTrace(InitFlags::ImmediateTrace());
   }
}

//------------------------------------------------------------------------------

TraceBuffer::~TraceBuffer()
{
   //  Delete all trace records before freeing the buffer.
   //
   Clear();

   Memory::Free(buff_);
   buff_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_AddRecord = "TraceBuffer.AddRecord";

void* TraceBuffer::AddRecord(size_t nBytes)
{
   //  While a record is being added, the trace buffer is in a transient
   //  state.  Existing records may be deleted to make room for the new
   //  record, start_ and size_ are being updated, and the new record is
   //  only partially constructed.  The buffer is therefore locked, and
   //  the constructor of a TraceRecord subclass must call Unlock.
   //
   if(hardLock_.exchange(true))
   {
      ++blocks_;
      return nullptr;
   }

   //  If immediate output is desired, display records as they are created.
   //
   if(immediate_) ImmediateDisplay();

   auto size = Memory::Words(nBytes);  // size of the record in the buffer
   auto newEnd = end_ + size;          // next value of end_

   //  "Top" means the physical start of the buffer: [0].
   //  "Bottom" means the physical end of the buffer: [size_ - 1].
   //  "Start" means the logical start of the buffer: [start_].
   //  "End" means logical end of the buffer: [end_].
   //
   if(newEnd >= size_)
   {
      //  The buffer has overflowed, so the new record will be added at
      //  the top.  If the start is below the end, this is the *second*
      //  (or later) occurrence of wraparound, in which case the records
      //  between the start and the bottom must be purged.  After that,
      //  one or more records at the top must be purged to make room for
      //  the new record.
      //
      if(softLocks_.load() > 0)
      {
         ++blocks_;
         hardLock_.store(false);
         return nullptr;
      }

      ovfl_ = true;
      if(start_ > end_) PurgeRecords(size_ - 1);
      PurgeRecords(size);

      //  There should already be an end-of-buffer marker at the bottom of
      //  the buffer as a result of having added the previous record.
      //
      if(buff_[end_] != EndMarker)
      {
         Debug::SwErr(TraceBuffer_AddRecord, start_, end_);
         buff_[end_] = EndMarker;
      }

      end_ = 0;
      newEnd = size;
   }
   else
   {
      //  If the start is below the end, the buffer has wrapped around.  If
      //  the new record will also reach or surpass the start, records must
      //  be purged to make room for the new record.
      //
      if(start_ > end_)
      {
         if(softLocks_.load() > 0)
         {
            ++blocks_;
            hardLock_.store(false);
            return nullptr;
         }

         if(newEnd >= start_) PurgeRecords(newEnd);
      }
   }

   //  Return a pointer to the space allocated for the record after updating
   //  the end-of-buffer location.  If records are being display immediately,
   //  construct a temporary record in case the current thread is scheduled
   //  out before the new record has been fully constructed.
   //
   auto addr = (TraceRecord*) &buff_[end_];
   end_ = newEnd;
   buff_[end_] = EndMarker;
   if(immediate_) new (addr) BufferTrace(size);
   lastRecord_ = addr;
   hardLock_.store(false);
   return addr;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_ClaimBlocks = "TraceBuffer.ClaimBlocks";

void TraceBuffer::ClaimBlocks()
{
   Debug::ft(TraceBuffer_ClaimBlocks);

   auto count = MaxRecords();

   //  Only BuffTracer entries contains objects that must be claimed,
   //  so skip all others.
   //
   TraceRecord* record = nullptr;
   auto mask = Flags(1 << BufferTracer);

   Lock();
      for(Next(record, mask); record != nullptr; Next(record, mask))
      {
         record->ClaimBlocks();

         if(--count <= 0)
         {
            Debug::SwErr(TraceBuffer_ClaimBlocks, MaxRecords(), 0);
            break;
         }
      }
   Unlock();
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_Clear = "TraceBuffer.Clear";

TraceRc TraceBuffer::Clear()
{
   Debug::ft(TraceBuffer_Clear);

   //  If tracing has been stopped, delete all records in the buffer
   //  and reset member variables.
   //
   if(Debug::TraceOn()) return NotWhileTracing;

   TraceRecord* record = nullptr;
   auto count = 0;
   auto mask = Flags().set();

   for(Next(record, mask); record != nullptr; Next(record, mask))
   {
      delete record;

      if(++count >= 100)
      {
         ThisThread::PauseOver(90);
         count = 0;
      }
   }

   start_ = 0;
   end_ = 0;
   ovfl_ = false;
   lastRecord_ = nullptr;
   dtorDepth_ = -1;
   hardLock_.store(false);
   softLocks_.exchange(0);
   blocks_ = 0;

   invocations_->clear();

   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_ClearTools = "TraceBuffer.ClearTools";

TraceRc TraceBuffer::ClearTools()
{
   Debug::ft(TraceBuffer_ClearTools);

   tools_.reset();
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_DisplayTrace = "TraceBuffer.DisplayTrace";

TraceRc TraceBuffer::DisplayTrace(ostream* stream, bool diff)
{
   Debug::ft(TraceBuffer_DisplayTrace);

   if(Empty()) return BufferEmpty;

   if(stream == nullptr)
   {
      if(stream_ == nullptr) return CouldNotOpenFile;
      stream = stream_.get();
   }

   auto rc = TraceDump::Generate(*stream, diff);
   stream_.reset();
   return rc;
}

//------------------------------------------------------------------------------

bool TraceBuffer::Empty() const
{
   //  If the buffer is empty, the start_ and end_ indicates both point
   //  to its top.
   //
   return ((start_ == 0) && (end_ == 0));
}

//------------------------------------------------------------------------------

void TraceBuffer::ImmediateDisplay()
{
   //  Create a mask that will find every record.  Starting with the last
   //  record that was created, display records until a nil record or the
   //  end of the buffer is reached.  Normally this will display the last
   //  record that was created, after which the end of the buffer will be
   //  reached, causing lastRecord_ to become nullptr.  However, it could
   //  be that the last record has not been fully constructed.
   //
   auto mask = Flags().set();

   while(lastRecord_ != nullptr)
   {
      if(lastRecord_->rid_ == TraceRecord::InvalidId) return;
      if(lastRecord_->Display(*stream_, false)) *stream_ << CRLF;
      Next(lastRecord_, mask);
   }
}

//------------------------------------------------------------------------------

bool TraceBuffer::IsLocked()
{
   if(!hardLock_.load()) return false;
   ++blocks_;
   return true;
}

//------------------------------------------------------------------------------

void TraceBuffer::Lock()
{
   softLocks_.fetch_add(1);
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_MaxRecords = "TraceBuffer.MaxRecords";

size_t TraceBuffer::MaxRecords() const
{
   Debug::ft(TraceBuffer_MaxRecords);

   if(start_ == end_) return 0;
   if(start_ > end_) return size_ / Memory::Words(sizeof(TraceRecord));
   return (end_ - start_) / Memory::Words(sizeof(TraceRecord));
}

//------------------------------------------------------------------------------

void TraceBuffer::Next(TraceRecord*& record, const Flags& mask) const
{
   uword* next;
   uword incr;

   //  If RECORD is nullptr, start with the first record.
   //
   if(record == nullptr)
   {
      if(!Empty())
         next = &buff_[start_];
      else
         return;
   }
   else
   {
      incr = Memory::Words(record->size_);
      next = (uword*) record + incr;
   }

   //  Look for records until MASK is satisfied.  If the end-of-buffer
   //  marker is reached, there are two possibilities:
   //  (a) It's really the end of the buffer.
   //  (b) We're at the bottom of the buffer but not at its end.  The
   //      buffer is circular and contains variable-length entries, so
   //      an end-of-buffer marker flags unused space at the bottom of
   //      the buffer when wraparound occurs.
   //
   while(next != nullptr)
   {
      if(*next == EndMarker)
      {
         if(next == &buff_[end_])
            record = nullptr;               // case (a)
         else
            record = (TraceRecord*) buff_;  // case (b)
      }
      else
      {
         record = (TraceRecord*) next;
      }

      if(record == nullptr) return;
      if(record->owner_ != NIL_ID)
      {
         if((mask.test(record->owner_)) != 0) return;
      }
      incr = Memory::Words(record->size_);
      next = (uword*) record + incr;
   }
}

//------------------------------------------------------------------------------

void TraceBuffer::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void TraceBuffer::PurgeRecords(size_t end)
{
   while(start_ <= end)
   {
      if(buff_[start_] == EndMarker)
      {
         //  The buffer wraps around here.  Records at the top are purged
         //  separately.
         //
         start_ = 0;
         return;
      }

      auto record = (TraceRecord*) &buff_[start_];
      start_ += Memory::Words(record->size_);
      delete record;
   }
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_Query = "TraceBuffer.Query";

void TraceBuffer::Query(ostream& stream) const
{
   Debug::ft(TraceBuffer_Query);

   auto size = (size_ >> 10) << BYTES_PER_WORD_LOG2;
   auto entries = (ovfl_ ? size_ : end_);
   entries = (entries >> 10) << BYTES_PER_WORD_LOG2;

   stream << "Trace buffer" << CRLF;
   stream << "  size    : " << size << "KB" << CRLF;
   stream << "  entries : " << entries << "KB" << CRLF;
   stream << "  blocked : " << blocks_ << CRLF;

   if(ovfl_) stream << OverflowStr << CRLF;
}

//------------------------------------------------------------------------------

fixed_string TracingOn = "Tracing is ON.";
fixed_string TracingOff = "Tracing is OFF.";
fixed_string ImmediateOn = "Immediate tracing selected.";

fn_name TraceBuffer_QueryTools = "TraceBuffer.QueryTools";

void TraceBuffer::QueryTools(ostream& stream) const
{
   Debug::ft(TraceBuffer_QueryTools);

   if(Debug::TraceOn())
      stream << TracingOn << CRLF;
   else
      stream << TracingOff << CRLF;

   if(immediate_) stream << ImmediateOn << CRLF;

   auto& tools = Singleton< ToolRegistry >::Instance()->Tools();

   for(auto t = tools.First(); t != nullptr; tools.Next(t))
   {
      stream << "  " << t->Name() << ": " << t->Status() << CRLF;
   }
}

//------------------------------------------------------------------------------

void TraceBuffer::RecordInvocation(fn_name_arg func) const
{
   //  Try to add an entry indicating that FUNC has been invoked once.
   //
   auto result = invocations_->insert(FunctionCount(func, 1));

   //  If the entry couldn't be added, FUNC was already in the table,
   //  so increment its invocation count.
   //
   if(!result.second)
   {
      auto& entry = *result.first;
      ++entry.second;
   }
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_SelectAll = "TraceBuffer.SelectAll";

TraceRc TraceBuffer::SelectAll(bool on)
{
   Debug::ft(TraceBuffer_SelectAll);

   if(on)
      SetFilter(TraceAll);
   else
      ClearFilter(TraceAll);

   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_SetSize = "TraceBuffer.SetSize";

TraceRc TraceBuffer::SetSize(size_t kbs)
{
   Debug::ft(TraceBuffer_SetSize);

   //  If tracing has been stopped and the buffer has been cleared, convert
   //  the size (KBS) to words, delete the old buffer, and create a new one.
   //
   if(Debug::TraceOn()) return NotWhileTracing;
   if(!Empty()) return BufferNotEmpty;

   auto size = (kbs << 10) >> BYTES_PER_WORD_LOG2;
   if(size > MaxSize) size = MaxSize;

   Memory::Free(buff_);
   size_ = 0;

   buff_ = (uword*) Memory::Alloc(size << BYTES_PER_WORD_LOG2, MemPerm, false);
   if(buff_ == nullptr) return BufferAllocFailed;

   size_ = size;
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_SetTool = "TraceBuffer.SetTool";

TraceRc TraceBuffer::SetTool(FlagId tid, bool value)
{
   Debug::ft(TraceBuffer_SetTool);

   //u This is invoked well before main(), so avoid creating ToolRegistry that
   //  early.  Doing so causes heap corruption in Windows debug mode, but the
   //  reason has not been determined.
   //
   auto reg = Singleton< ToolRegistry >::Extant();

   if(reg != nullptr)
   {
      auto tool = reg->GetTool(tid);
      if(tool == nullptr) return NoSuchItem;
      if(value && !tool->IsSafe()) return NotInField;
   }

   tools_.set(tid, value);
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_SetTools = "TraceBuffer.SetTools";

TraceRc TraceBuffer::SetTools(const Flags& tools)
{
   Debug::ft(TraceBuffer_SetTools);

   tools_ = tools;
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_Shutdown = "TraceBuffer.Shutdown";

void TraceBuffer::Shutdown(RestartLevel level)
{
   Debug::ft(TraceBuffer_Shutdown);

   auto count = MaxRecords();

   TraceRecord* record = nullptr;
   auto mask = Flags().set();

   Lock();
      for(Next(record, mask); record != nullptr; Next(record, mask))
      {
         record->Shutdown(level);

         if(--count <= 0)
         {
            Debug::SwErr(TraceBuffer_Shutdown, MaxRecords(), 0);
            break;
         }
      }
   Unlock();
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_StartTrace = "TraceBuffer.StartTrace";

TraceRc TraceBuffer::StartTrace(bool immediate)
{
   Debug::ft(TraceBuffer_StartTrace);

   if(Debug::TraceOn() && !Empty()) return AlreadyStarted;

   if(tools_.none()) return NoToolSelected;

   if(filters_.none()) return NoItemSelected;

   if(buff_ == nullptr) return NoBufferAllocated;

   if(!Empty())
   {
      SetTool(ToolBuffer, true);
      new BufferTrace;
   }

   if(immediate)
   {
      auto path = Element::OutputPath() + PATH_SEPARATOR + "immed.trace.txt";
      stream_ = SysFile::CreateOstream(path.c_str());
      if(stream_ == nullptr) return CouldNotOpenFile;
      immediate_ = true;
   }

   if(Empty()) startTime_ = SysTime();
   Debug::FcFlags_.set(Debug::TracingActive);
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_StopTrace = "TraceBuffer.StopTrace";

void TraceBuffer::StopTrace()
{
   Debug::ft(TraceBuffer_StopTrace);

   if(!Debug::FcFlags_.test(Debug::TracingActive)) return;

   SetTool(ToolBuffer, false);
   Debug::FcFlags_.reset(Debug::TracingActive);

   if(hardLock_.exchange(false))
   {
      Debug::SwErr(TraceBuffer_StopTrace, 0, 0);
   }

   //  If trace records are being output immediately, display the last
   //  one and return after closing the trace output file.
   //
   if(immediate_)
   {
      ImmediateDisplay();
      stream_.reset();
      immediate_ = false;
      return;
   }
}

//------------------------------------------------------------------------------

string TraceBuffer::strTimePlace() const
{
   std::ostringstream stream;

   stream << ": " << startTime_.to_str(SysTime::Alpha);
   stream << " on " << Element::Name();
   return stream.str();
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_Unlock = "TraceBuffer.Unlock";

void TraceBuffer::Unlock()
{
   if(softLocks_.load() > 0)
      softLocks_.fetch_sub(1);
   else
      Debug::SwErr(TraceBuffer_Unlock, 0, 1);
}
}
