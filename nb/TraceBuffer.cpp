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
#include <cstdint>
#include <sstream>
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "FunctionTrace.h"
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

   //  Constructs a trace record to indicate when tracing resumed after
   //  being stopped.
   //
   BufferTrace();

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, bool diff) override;
};

//------------------------------------------------------------------------------

BufferTrace::BufferTrace() : TraceRecord(ToolBuffer)
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

const size_t TraceBuffer::MinSize = 16;  // 64K TraceRecords
const size_t TraceBuffer::MaxSize = 20;  // 1M TraceRecords
fixed_string TraceBuffer::NoneSelected = "none";

fixed_string StartOfTrace = "START OF TRACE";
fixed_string BlockedStr = "Functions not captured because buffer was locked: ";
fixed_string BuffFullStr =
   "The buffer is full. The latter part of the trace was lost.";
fixed_string BuffOvflStr =
   "The buffer wrapped around. Older entries were lost.";

//------------------------------------------------------------------------------

TraceBuffer::TraceBuffer() :
   buff_(nullptr),
   funcs_(nullptr),
   size_(0),
   bnext_(0),
   fnext_(0),
   wrap_(false),
   ovfl_(false),
   lastRecord_(nullptr),
   dtorDepth_(-1),
   softLocks_(0),
   immediate_(false),
   stream_(nullptr),
   blocks_(0)
{
   AllocBuffers(MinSize, true);
   invocations_.reset(new InvocationsTable);

   if(InitFlags::TraceInit())
   {
      string options;
      SetTool(FunctionTracer, true);
      SetFilter(TraceAll);
      options.push_back('s');
      if(InitFlags::ImmediateTrace()) options.push_back('i');
      StartTracing(options);
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
   Memory::Free(funcs_);
   funcs_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_AddFunction = "TraceBuffer.AddFunction";

FunctionTrace OverflowSlot(TraceBuffer_AddFunction, 1);

void* TraceBuffer::AddFunction()
{
   //  If no more records can be added to the trace buffer, construct
   //  FunctionTrace records in the scratch location OverflowSlot.
   //
   if(ovfl_ && !wrap_) return &OverflowSlot;
   auto slot = fnext_.fetch_add(1);
   slot = slot & (size_ - 1);
   return &funcs_[slot];
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_AllocBuffers = "TraceBuffer.AllocBuffers";

bool TraceBuffer::AllocBuffers(size_t n, bool ex)
{
   Debug::ft(TraceBuffer_AllocBuffers);

   //  This should only be invoked after any trace records have been deleted.
   //
   if(!Empty())
   {
      Debug::SwLog(TraceBuffer_AllocBuffers, "buffer not empty", 0);
      return false;
   }

   //  If wraparound is enabled, AllocSlot increments bnext_ past the
   //  buffer's size when allocating the next slot.  The slot's value
   //  must then be brought into range, which can be done with a masking
   //  operation rather than a modulo division if size_ is a power of 2.
   //
   if(n < MinSize) n = MinSize;
   if(n > MaxSize) n = MaxSize;
   auto size = 1 << n;

   Memory::Free(buff_);
   buff_ = nullptr;
   Memory::Free(funcs_);
   funcs_ = nullptr;
   size_ = 0;

   buff_ = (TraceRecord**)
      Memory::Alloc(size * sizeof(TraceRecord*), MemPerm, ex);
   if(buff_ == nullptr) return false;

   funcs_ = (FunctionTrace*)
      Memory::Alloc(size * sizeof(FunctionTrace), MemPerm, ex);
   if(funcs_ == nullptr)
   {
      Memory::Free(buff_);
      buff_ = nullptr;
      return false;
   }

   size_ = size;
   for(size_t i = 0; i < size_; ++i) buff_[i] = nullptr;
   return true;
}

//------------------------------------------------------------------------------

size_t TraceBuffer::AllocSlot()
{
   //  If the buffer is full, this fails if the buffer is locked
   //  (because it would have to delete an existing record to add
   //  a new one) or if wraparound is not enabled.
   //
   if(buff_ == nullptr) return SIZE_MAX;

   if(bnext_ >= size_)
   {
      ovfl_ = true;
      if(!wrap_) return SIZE_MAX;

      if(softLocks_ > 0)
      {
         ++blocks_;
         return SIZE_MAX;
      }
   }

   auto slot = bnext_.fetch_add(1);
   return (slot & (size_ - 1));
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_ClaimBlocks = "TraceBuffer.ClaimBlocks";

void TraceBuffer::ClaimBlocks()
{
   Debug::ft(TraceBuffer_ClaimBlocks);

   //  Function trace records don't need to claim anything, so skip
   //  them for efficiency.
   //
   TraceRecord* rec = nullptr;
   Flags mask;
   mask.set();
   mask.reset(FunctionTracer);

   Lock();
      for(Next(rec, mask); rec != nullptr; Next(rec, mask))
      {
         rec->ClaimBlocks();
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
   if(buff_ == nullptr) return NoBufferAllocated;
   if(Debug::TraceOn()) return NotWhileTracing;

   auto count = 0;

   Lock();
      auto last = (ovfl_ ? size_ : bnext_);

      for(size_t i = 0; i < last; ++i)
      {
         auto rec = buff_[i];

         if(rec != nullptr)
         {
            buff_[i] = nullptr;
            delete rec;

            if(++count >= 100)
            {
               ThisThread::PauseOver(90);
               count = 0;
            }
         }
      }
   Unlock();

   bnext_ = 0;
   fnext_ = 0;
   ovfl_ = false;
   lastRecord_ = nullptr;
   dtorDepth_ = -1;
   softLocks_ = 0;
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

fn_name TraceBuffer_DisplayStart = "TraceBuffer.DisplayStart";

void TraceBuffer::DisplayStart(ostream& stream) const
{
   Debug::ft(TraceBuffer_DisplayStart);

   stream << StartOfTrace << strTimePlace() << CRLF << CRLF;
   if(blocks_ > 0) stream << BlockedStr << blocks_ << CRLF;

   if(ovfl_)
   {
      if(wrap_)
         stream << BuffOvflStr << CRLF;
      else
         stream << BuffFullStr << CRLF;
   }

   if((blocks_ > 0) || ovfl_) stream << CRLF;
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
   return (bnext_ == 0);
}

//------------------------------------------------------------------------------

bool TraceBuffer::Insert(TraceRecord* record)
{
   //  If immediate tracing is active, display the record now.
   //
   if(immediate_)
   {
      if(record->Display(*stream_, false)) *stream_ << CRLF;
   }

   //  Delete the record if no slot is available.
   //
   auto slot = AllocSlot();

   if(slot == SIZE_MAX)
   {
      delete record;
      return false;
   }

   //  If wraparound is allowed, there could already be a record
   //  in the slot.  If so, delete it.
   //
   auto prev = buff_[slot];

   if(prev != nullptr)
   {
      buff_[slot] = nullptr;
      delete prev;
   }

   record->slot_ = slot;
   buff_[slot] = record;
   lastRecord_ = record;
   return true;
}

//------------------------------------------------------------------------------

void TraceBuffer::Lock()
{
   softLocks_.fetch_add(1);
}

//------------------------------------------------------------------------------

void TraceBuffer::Next(TraceRecord*& record, const Flags& mask) const
{
   if(Empty())
   {
      record = nullptr;
      return;
   }

   //  The current slot is tracked by I.  NEXT is the last allocated slot.
   //
   size_t i = SIZE_MAX;
   auto last = ((bnext_ - 1) & (size_ - 1));

   //  Update RECORD to where the search for the next entry will begin.
   //  If RECORD is nullptr, start with the first entry, else continue
   //  with the next entry.
   //
   if(record == nullptr)
   {
      i = (wrap_ && ovfl_ ? bnext_ & (size_ - 1) : 0);
      record = buff_[i];
   }
   else
   {
      i = record->slot_;

      if(i == last)
      {
         record = nullptr;
         return;
      }

      if(++i >= size_) i = 0;
      record = buff_[i];
   }

   while(true)
   {
      //  If wraparound is disabled, there is a small possibility that a
      //  FunctionTrace record could be constructed on top of a previous
      //  one and then fail to get inserted in the trace buffer.  Should
      //  this occur, its slot will be invalid, so skip it.
      //
      if((record != nullptr) &&
         (record->slot_ != TraceRecord::InvalidSlot) &&
         (mask.test(record->owner_)))
      {
         return;
      }

      if(i == last)
      {
         record = nullptr;
         return;
      }

      if(++i >= size_) i = 0;
      record = buff_[i];
   }
}

//------------------------------------------------------------------------------

void TraceBuffer::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_Query = "TraceBuffer.Query";

void TraceBuffer::Query(ostream& stream) const
{
   Debug::ft(TraceBuffer_Query);

   auto indent = spaces(2);
   auto entries = (ovfl_ ? size_ : bnext_);

   stream << strClass(this) << CRLF;
   stream << indent << "size    : " << size_ << CRLF;
   stream << indent << "entries : " << entries << CRLF;
   stream << indent << "blocked : " << blocks_ << CRLF;
   stream << indent << "wraparound enabled : " << (wrap_ ? "Y" : "N") << CRLF;
   if(ovfl_) stream << (wrap_ ? BuffOvflStr : BuffFullStr) << CRLF;
}

//------------------------------------------------------------------------------

fixed_string TracingOn = "Tracing is ON.";
fixed_string TracingOff = "Tracing is OFF.";
fixed_string ImmediateOn = "IMMEDIATE tracing is enabled.";
fixed_string SlowOn = "SLOW tracing is enabled.";

fn_name TraceBuffer_QueryTools = "TraceBuffer.QueryTools";

void TraceBuffer::QueryTools(ostream& stream) const
{
   Debug::ft(TraceBuffer_QueryTools);

   if(Debug::TraceOn())
   {
      stream << TracingOn << CRLF;
      if(immediate_) stream << ImmediateOn << CRLF;
      if(Debug::SlowTraceOn()) stream << SlowOn << CRLF;
   }
   else
   {
      stream << TracingOff << CRLF;
   }

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

TraceRc TraceBuffer::SetSize(size_t n)
{
   Debug::ft(TraceBuffer_SetSize);

   //  Buffer resizing is only allowed when tracing has been stopped
   //  and all trace records have been cleared.
   //
   if(Debug::TraceOn()) return NotWhileTracing;
   if(!Empty()) return BufferNotEmpty;
   if(!AllocBuffers(n, false)) return BufferAllocFailed;
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

fn_name TraceBuffer_SetWrap = "TraceBuffer.SetWrap";

TraceRc TraceBuffer::SetWrap(bool wrap)
{
   Debug::ft(TraceBuffer_SetWrap);

   //  Although wraparound could be enabled/disabled while the buffer contains
   //  entries, it appears to be of little value and would result in confusing
   //  results.
   //
   if(Debug::TraceOn()) return NotWhileTracing;
   if(!Empty()) return BufferNotEmpty;
   wrap_ = wrap;
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_Shutdown = "TraceBuffer.Shutdown";

void TraceBuffer::Shutdown(RestartLevel level)
{
   Debug::ft(TraceBuffer_Shutdown);

   TraceRecord* rec = nullptr;
   auto mask = Flags().set();

   Lock();
      for(Next(rec, mask); rec != nullptr; Next(rec, mask))
      {
         rec->Shutdown(level);
      }
   Unlock();
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_StartTracing = "TraceBuffer.StartTracing";

TraceRc TraceBuffer::StartTracing(const string& options)
{
   Debug::ft(TraceBuffer_StartTracing);

   if(Debug::TraceOn() && !Empty()) return AlreadyStarted;
   if(tools_.none()) return NoToolSelected;
   if(filters_.none()) return NoItemSelected;
   if(buff_ == nullptr) return NoBufferAllocated;

   if(!Empty())
   {
      SetTool(ToolBuffer, true);
      Insert(new BufferTrace);
   }

   if(options.find('i') != string::npos)
   {
      auto path = Element::OutputPath();
      if(!path.empty()) path.push_back(PATH_SEPARATOR);
      path += "immed.trace.txt";
      stream_ = SysFile::CreateOstream(path.c_str());
      if(stream_ == nullptr) return CouldNotOpenFile;
      immediate_ = true;
   }

   auto slow = (options.find('s') != string::npos);
   Debug::SetSlowTrace(slow);

   if(Empty()) startTime_ = SysTime();
   Debug::FcFlags_.set(Debug::TracingActive);
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_StopTracing = "TraceBuffer.StopTracing";

void TraceBuffer::StopTracing()
{
   Debug::ft(TraceBuffer_StopTracing);

   if(!Debug::FcFlags_.test(Debug::TracingActive)) return;

   SetTool(ToolBuffer, false);
   Debug::FcFlags_.reset(Debug::TracingActive);

   //  If trace records are being output immediately, return after
   //  closing the trace output file.
   //
   if(immediate_)
   {
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

void TraceBuffer::Swap(TraceRecord* record1, TraceRecord* record2) const
{
   auto slot1 = record1->slot_;
   auto slot2 = record2->slot_;
   record1->slot_ = slot2;
   record2->slot_ = slot1;
   buff_[slot1] = record2;
   buff_[slot2] = record1;
}

//------------------------------------------------------------------------------

fn_name TraceBuffer_Unlock = "TraceBuffer.Unlock";

void TraceBuffer::Unlock()
{
   if(softLocks_ > 0)
      softLocks_.fetch_sub(1);
   else
      Debug::SwLog(TraceBuffer_Unlock, "not locked", 0);
}
}
