//==============================================================================
//
//  LogBuffer.cpp
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
#include "LogBuffer.h"
#include <bitset>
#include <memory>
#include <ostream>
#include "Debug.h"
#include "Log.h"
#include "LogThread.h"
#include "Memory.h"
#include "Mutex.h"
#include "NbTypes.h"
#include "Restart.h"
#include "Singleton.h"
#include "SystemTime.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Critical section lock for the log buffer.
//
static Mutex LogBufferLock_("LogBufferLock");

//------------------------------------------------------------------------------

class LogsWritten : public CallbackRequest
{
public:
   //  Constructs a callback when LAST was the last log spooled from BUFF.
   //
   LogsWritten(LogBuffer* buff, const LogBuffer::Entry* last);

   void Callback() override;
private:
   //  The buffer from which the logs were spooled.
   //
   LogBuffer* const buff_;

   //  The last log that was spooled from the buffer.
   //
   const LogBuffer::Entry* const last_;
};

//------------------------------------------------------------------------------

LogsWritten::LogsWritten(LogBuffer* buff, const LogBuffer::Entry* last) :
   buff_(buff),
   last_(last)
{
   Debug::ft("LogsWritten.ctor");
}

//------------------------------------------------------------------------------

fn_name LogsWritten_Callback = "LogsWritten.Callback";

void LogsWritten::Callback()
{
   Debug::ft(LogsWritten_Callback);

   buff_->Purge(last_);
}

//==============================================================================
//
//> When bundling logs into a stream, the number of characters that
//  prevents another log from being added to the stream.
//
constexpr size_t BundledLogSizeThreshold = 2048;

//------------------------------------------------------------------------------

LogBuffer::LogBuffer(size_t size) :
   discards_(0),
   spooled_(nullptr),
   unspooled_(nullptr),
   next_(nullptr),
   max_(0),
   size_(0),
   buff_(nullptr)
{
   Debug::ft("LogBuffer.ctor");

   //  During a boot/reboot, the name of the log file includes the system's
   //  startup time.  During a restart, its name contains the time at which
   //  the log buffer was created.  The '.' before the final msecs value is
   //  replaced within a '-'.
   //
   if(Restart::GetLevel() == RestartReboot)
      fileName_ = "logs" + to_string(SystemTime::TimeZero(), FullNumeric);
   else
      fileName_ = "logs" + to_string(SystemTime::Now(), FullNumeric);
   fileName_.append(".txt");

   if(size < (16 * kBs)) size = 16 * kBs;
   buff_ = static_cast<char*>(Memory::Alloc(size, MemPermanent));
   size_ = size;
   SetNext(reinterpret_cast<Entry*>(buff_));
}

//------------------------------------------------------------------------------

LogBuffer::~LogBuffer()
{
   Debug::ftnt("LogBuffer.dtor");

   MutexGuard guard(&LogBufferLock_);

   Memory::Free(buff_, MemPermanent);
   buff_ = nullptr;
}

//------------------------------------------------------------------------------

const LogBuffer::Entry* LogBuffer::Advance()
{
   Debug::ft("LogBuffer.Advance");

   if(unspooled_ == nullptr) return nullptr;

   if(spooled_ == nullptr)
   {
      spooled_ = unspooled_;  // first unspooled log was just spooled
   }

   unspooled_ = unspooled_->header.next;
   return unspooled_;
}

//------------------------------------------------------------------------------

size_t LogBuffer::Count(bool spooled, bool unspooled) const
{
   Debug::ft("LogBuffer.Count");

   if(!spooled && !unspooled) return 0;

   size_t total = 0;

   for(auto curr = First(); curr != nullptr; ++total)
   {
      curr = curr->header.next;
   }

   if(spooled && unspooled) return total;

   size_t unsent = 0;

   for(auto curr = unspooled_; curr != nullptr; ++unsent)
   {
      curr = curr->header.next;
   }

   if(unspooled) return unsent;
   return (total - unsent);
}

//------------------------------------------------------------------------------

void LogBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(!options.test(DispVerbose))
   {
      stream << fileName_ << SPACE
         << "[spooled=" << Count(true, false) << SPACE
         << "unspooled=" << Count(false, true) << ']' << CRLF;
      return;
   }

   Permanent::Display(stream, prefix, options);

   stream << prefix << "fileName   : " << fileName_ << CRLF;
   stream << prefix << "discards   : " << discards_ << CRLF;
   stream << prefix << "spooled    : " << spooled_ << CRLF;
   stream << prefix << "unspooled  : " << unspooled_ << CRLF;
   stream << prefix << "next       : " << next_ << CRLF;
   stream << prefix << "max (kBs)  : " << (max_ / kBs) << CRLF;
   stream << prefix << "size (kBs) : " << (size_ / kBs) << CRLF;
   stream << prefix << "buff       : " << intptr_t(buff_) << CRLF;
}

//------------------------------------------------------------------------------

const LogBuffer::Entry* LogBuffer::First() const
{
   Debug::ftnt("LogBuffer.First");

   return (spooled_ != nullptr ? spooled_ : unspooled_);
}

//------------------------------------------------------------------------------

const LogBuffer::Entry* LogBuffer::FirstSpooled() const
{
   Debug::ft("LogBuffer.FirstSpooled");

   return spooled_;
}

//------------------------------------------------------------------------------

const LogBuffer::Entry* LogBuffer::FirstUnspooled() const
{
   Debug::ft("LogBuffer.FirstUnspooled");

   return unspooled_;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_GetLogs = "LogBuffer.GetLogs";

string LogBuffer::GetLogs(CallbackRequestPtr& callback, bool& periodic)
{
   Debug::ft(LogBuffer_GetLogs);

   //  If the log buffer contains any logs, create a stream to spool them.
   //
   MutexGuard guard(&LogBufferLock_);

   string logs;
   periodic = false;
   auto curr = FirstUnspooled();
   if(curr == nullptr) return logs;

   //  Accumulate logs until they exceed the size limit.  But first, insert
   //  a warning if some logs were discarded because the buffer was full.
   //
   if(discards_ > 0)
   {
      logs.push_back(CRLF);
      logs.append(AlarmStatusSymbol(MinorAlarm));
      logs.push_back(SPACE);
      logs.append("WARNING: ");
      logs.append(std::to_string(discards_));
      logs.append(" log(s) discarded");
      logs.push_back(CRLF);
      discards_ = 0;
   }

   size_t count = 0;
   const Entry* prev = nullptr;

   while((logs.size() < BundledLogSizeThreshold) && (curr != nullptr))
   {
      //  Identify this log so that a periodic log is not bundled with
      //  others.
      //
      auto log = Log::Find(&curr->log[0]);

      if(log == nullptr)
      {
         Debug::SwLog(LogBuffer_GetLogs, "log not found", 0);
         curr = Advance();
         continue;
      }

      periodic = (GetLogType(log->Id()) == PeriodicLog);

      if(periodic && (count > 0))
      {
         periodic = false;
         break;
      }

      logs.append(&curr->log[0]);
      prev = curr;
      curr = Advance();
      if(periodic) break;
      ++count;
   }

   callback.reset(new LogsWritten(this, prev));
   return logs;
}

//------------------------------------------------------------------------------

LogBuffer::Entry* LogBuffer::InsertionPoint(size_t size)
{
   Debug::ftnt("LogBuffer.InsertionPoint");

   //  The log is normally inserted at next_, which will advance to AFTER.
   //  However, the log needs to go at the top of the buffer if it would
   //  overrun it.
   //
   auto where = next_;
   auto after = (ptr_t) next_ + size;
   auto wrap = after >= (buff_ + size_);
   auto first = First();

   //  If the new log would overwrite the first log, discard the new log.
   //  Older logs are preserved because they capture the onset of a problem
   //  when a log flood occurs.
   //
   if(next_ > first)
   {
      //  The first log lies above the new one, so an overwrite can only
      //  occur when wrapping around and running into the first log.
      //
      if(wrap && (after > (const_ptr_t) first)) return nullptr;
   }
   else
   {
      //  The first log lies below the new one, so an overwrite always occurs
      //  if wrapping around.  It also occurs when running into the first log.
      //
      if(wrap || (after > (const_ptr_t) first)) return nullptr;
   }

   //  If the new log will overrun the buffer, insert it at the top.
   //
   if(wrap)
   {
      where = reinterpret_cast<Entry*>(buff_);
      after = buff_ + size;
   }

   auto prev = next_->header.prev;
   if(prev != nullptr) prev->header.next = where;
   where->header.prev = prev;
   where->header.next = nullptr;
   SetNext(reinterpret_cast<Entry*>(after));
   next_->header.prev = where;
   return where;
}

//------------------------------------------------------------------------------

const LogBuffer::Entry* LogBuffer::Last() const
{
   Debug::ft("LogBuffer.Last");

   return next_->header.prev;
}

//------------------------------------------------------------------------------

void LogBuffer::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

const LogBuffer::Entry* LogBuffer::Pop()
{
   Debug::ft("LogBuffer.Pop");

   if(spooled_ == nullptr) return nullptr;

   spooled_ = spooled_->header.next;

   if(spooled_ == nullptr)
   {
      if(unspooled_ == nullptr)
      {
         //  The buffer is empty; add the next entry at the top.
         //
         SetNext(reinterpret_cast<Entry*>(buff_));
      }

      return nullptr;
   }

   spooled_->header.prev = nullptr;

   if(spooled_ == unspooled_)
   {
      //  Only unspooled logs remain.
      //
      spooled_ = nullptr;
   }

   return spooled_;
}

//------------------------------------------------------------------------------

void LogBuffer::Purge(const Entry* last)
{
   Debug::ft("LogBuffer.Purge");

   //  If the LAST log that was written to the log file still exists, free the
   //  logs before it, and then free it as well.
   //
   for(auto curr = Last(); curr != last; curr = curr->header.prev)
   {
      if(curr == nullptr)
      {
         //  LAST no longer exists: requests must have been reordered!?
         //
         Debug::SwLog(LogsWritten_Callback,
            "last not found", debug64_t(size_t(last)));
         return;
      }
   }

   auto curr = FirstSpooled();

   while((curr != last) && (curr != nullptr))
   {
      curr = Pop();
   }

   Pop();
}

//------------------------------------------------------------------------------

fn_name LogBuffer_Push = "LogBuffer.Push";

bool LogBuffer::Push(const std::string& log)
{
   Debug::ftnt(LogBuffer_Push);

   //  This must not be invoked during a restart.
   //  LogThread::Spool should be invoked instead.
   //
   auto level = Restart::GetStage();

   if(level != Running)
   {
      Debug::SwLog(LogBuffer_Push, "invoked during restart", level);
      return false;
   }

   auto count = log.size();
   size_t size = sizeof(Header) + count + 1;

   MutexGuard guard(&LogBufferLock_);

   auto entry = InsertionPoint(size);

   if(entry == nullptr)
   {
      ++discards_;
      return false;
   }

   Memory::Copy(entry->log, log.c_str(), count);
   entry->log[count] = NUL;
   if(unspooled_ == nullptr) unspooled_ = entry;
   UpdateMax();
   guard.Release();

   auto thread = Singleton<LogThread>::Extant();
   if(thread != nullptr) thread->Interrupt();
   return true;
}

//------------------------------------------------------------------------------

void LogBuffer::ResetAllToUnspooled()
{
   Debug::ft("LogBuffer.ResetAllToUnspooled");

   if(spooled_ != nullptr) unspooled_ = spooled_;
   spooled_ = nullptr;
}

//------------------------------------------------------------------------------

void LogBuffer::SetNext(Entry* next)
{
   Debug::ftnt("LogBuffer.SetNext");

   next_ = next;
   next_->header.prev = nullptr;
   next_->header.next = nullptr;
}

//------------------------------------------------------------------------------

void LogBuffer::UpdateMax()
{
   size_t used = 0;

   auto first = First();

   if(first < next_)
      used = (const_ptr_t) next_ - (const_ptr_t) first;
   else
      used = size_ - ((const_ptr_t) first - (const_ptr_t) next_);

   if(used > max_) max_ = used;
}
}
