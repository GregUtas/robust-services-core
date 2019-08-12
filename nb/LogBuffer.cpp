//==============================================================================
//
//  LogBuffer.cpp
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
#include "LogBuffer.h"
#include <bitset>
#include <cstring>
#include <iosfwd>
#include <sstream>
#include "Clock.h"
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "LogThread.h"
#include "Memory.h"
#include "MutexGuard.h"
#include "NbTypes.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysTime.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
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
   LogBuffer* buff_;

   //  The last log that was spooled from the buffer.
   //
   const LogBuffer::Entry* const last_;
};

//------------------------------------------------------------------------------

fn_name LogsWritten_ctor = "LogsWritten.ctor";

LogsWritten::LogsWritten(LogBuffer* buff, const LogBuffer::Entry* last) :
   buff_(buff),
   last_(last)
{
   Debug::ft(LogsWritten_ctor);
}

//------------------------------------------------------------------------------

fn_name LogsWritten_Callback = "LogsWritten.Callback";

void LogsWritten::Callback()
{
   Debug::ft(LogsWritten_Callback);

   buff_->Purge(last_);
}

//==============================================================================

const size_t LogBuffer::BundledLogSizeThreshold = 2048;

//------------------------------------------------------------------------------

fn_name LogBuffer_ctor = "LogBuffer.ctor";

LogBuffer::LogBuffer(size_t kbs) :
   discards_(0),
   spooled_(nullptr),
   unspooled_(nullptr),
   next_(nullptr),
   max_(0),
   size_(0),
   buff_(nullptr)
{
   Debug::ft(LogBuffer_ctor);

   //  During a boot/reboot, the name of the log file includes the system's
   //  startup time.  During a restart, its name contains the time at which
   //  the log buffer was created.  The '.' before the final msecs value is
   //  replaced within a '-'.
   //
   if(Restart::GetLevel() == RestartReboot)
      fileName_ = "logs" + Clock::TimeZeroStr();
   else
      fileName_ = "logs" + SysTime().to_str(SysTime::Numeric);
   auto pos = fileName_.find('.');
   if(pos != string::npos) fileName_[pos] = '-';
   fileName_.append(".txt");

   buff_ = static_cast< char* >(Memory::Alloc(kbs << 10, MemPerm));
   size_ = kbs << 10;
   SetNext(reinterpret_cast< Entry* >(buff_));
}

//------------------------------------------------------------------------------

fn_name LogBuffer_dtor = "LogBuffer.dtor";

LogBuffer::~LogBuffer()
{
   Debug::ft(LogBuffer_dtor);

   MutexGuard guard(&lock_);

   Memory::Free(buff_);
   buff_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_Advance = "LogBuffer.Advance";

const LogBuffer::Entry* LogBuffer::Advance()
{
   Debug::ft(LogBuffer_Advance);

   if(unspooled_ == nullptr) return nullptr;

   if(spooled_ == nullptr)
   {
      spooled_ = unspooled_;  // first unspooled log was just spooled
   }

   unspooled_ = unspooled_->next;
   return unspooled_;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_Count = "LogBuffer.Count";

size_t LogBuffer::Count(bool spooled, bool unspooled) const
{
   Debug::ft(LogBuffer_Count);

   if(!spooled && !unspooled) return 0;

   size_t total = 0;

   for(auto curr = First(); curr != nullptr; ++total)
   {
      curr = curr->next;
   }

   if(spooled & unspooled) return total;

   size_t unsent = 0;

   for(auto curr = unspooled_; curr != nullptr; ++unsent)
   {
      curr = curr->next;
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
   stream << prefix << "max (KBs)  : " << (max_ >> 10) << CRLF;
   stream << prefix << "size (KBs) : " << (size_ >> 10) << CRLF;
   stream << prefix << "buff       : " << intptr_t(buff_) << CRLF;

   auto lead = prefix + spaces(2);
   stream << prefix << "lock : " << CRLF;
   lock_.Display(stream, lead, options);
}

//------------------------------------------------------------------------------

fn_name LogBuffer_First = "LogBuffer.First";

const LogBuffer::Entry* LogBuffer::First() const
{
   Debug::ft(LogBuffer_First);

   return (spooled_ != nullptr ? spooled_ : unspooled_);
}

//------------------------------------------------------------------------------

fn_name LogBuffer_FirstSpooled = "LogBuffer.FirstSpooled";

const LogBuffer::Entry* LogBuffer::FirstSpooled() const
{
   Debug::ft(LogBuffer_FirstSpooled);

   return spooled_;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_FirstUnspooled = "LogBuffer.FirstUnspooled";

const LogBuffer::Entry* LogBuffer::FirstUnspooled() const
{
   Debug::ft(LogBuffer_FirstUnspooled);

   return unspooled_;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_GetLogs = "LogBuffer.GetLogs";

ostringstreamPtr LogBuffer::GetLogs
   (CallbackRequestPtr& callback, bool& periodic)
{
   Debug::ft(LogBuffer_GetLogs);

   //  If the log buffer contains any logs, create a stream to spool them.
   //
   MutexGuard guard(&lock_);

   periodic = false;
   auto curr = FirstUnspooled();
   if(curr == nullptr) return nullptr;
   ostringstreamPtr stream(new std::ostringstream);
   if(stream == nullptr) return nullptr;

   //  Accumulate logs until they exceed the size limit.  But first, insert
   //  a warning if some logs were discarded because the buffer was full.
   //
   if(discards_ > 0)
   {
      *stream << CRLF << AlarmStatusSymbol(MinorAlarm) << "WARNING: ";
      *stream << discards_ << " log(s) discarded";
      *stream << CRLF;
      discards_ = 0;
   }

   size_t count = 0;
   const LogBuffer::Entry* prev = nullptr;

   while((stream->str().size() < BundledLogSizeThreshold) && (curr != nullptr))
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

      *stream << &curr->log[0];
      prev = curr;
      curr = Advance();
      if(periodic) break;
      ++count;
   }

   callback.reset(new LogsWritten(this, prev));
   return stream;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_InsertionPoint = "LogBuffer.InsertionPoint";

LogBuffer::Entry* LogBuffer::InsertionPoint(size_t size)
{
   Debug::ft(LogBuffer_InsertionPoint);

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
      where = reinterpret_cast< Entry* >(buff_);
      after = buff_ + size;
   }

   auto prev = next_->prev;
   if(prev != nullptr) prev->next = where;
   where->prev = prev;
   where->next = nullptr;
   SetNext(reinterpret_cast< Entry* >(after));
   next_->prev = where;
   return where;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_Last = "LogBuffer.Last";

const LogBuffer::Entry* LogBuffer::Last() const
{
   Debug::ft(LogBuffer_Last);

   return next_->prev;
}

//------------------------------------------------------------------------------

void LogBuffer::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name LogBuffer_Pop = "LogBuffer.Pop";

const LogBuffer::Entry* LogBuffer::Pop()
{
   Debug::ft(LogBuffer_Pop);

   if(spooled_ == nullptr) return nullptr;

   spooled_ = spooled_->next;

   if(spooled_ == nullptr)
   {
      if(unspooled_ == nullptr)
      {
         //  The buffer is empty; add the next entry at the top.
         //
         SetNext(reinterpret_cast< Entry* >(buff_));
      }

      return nullptr;
   }

   spooled_->prev = nullptr;

   if(spooled_ == unspooled_)
   {
      //  Only unspooled logs remain.
      //
      spooled_ = nullptr;
   }

   return spooled_;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_Purge = "LogBuffer.Purge";

void LogBuffer::Purge(const Entry* last)
{
   Debug::ft(LogBuffer_Purge);

   //  If the LAST log that was written to the log file still exists, free the
   //  logs before it, and then free it as well.
   //
   for(auto curr = Last(); curr != last; curr = curr->prev)
   {
      if(curr == nullptr)
      {
         //  LAST no longer exists: requests must have been reordered!?
         //
         Debug::SwLog(LogsWritten_Callback, debug64_t(last), debug64_t(Last()));
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

LogBuffer::Entry* LogBuffer::Push(const ostringstreamPtr& log)
{
   Debug::ft(LogBuffer_Push);

   //  This must not be invoked during a restart.
   //  LogThread::Spool should be invoked instead.
   //
   auto level = Restart::GetStatus();
   Debug::Assert(level == Running, level);

   MutexGuard guard(&lock_);

   auto count = log->str().size();
   auto size = 2 * sizeof(intptr_t) + count + 1;
   auto entry = InsertionPoint(size);

   if(entry == nullptr)
   {
      ++discards_;
      return nullptr;
   }

   strcpy(entry->log, log->str().c_str());
   if(unspooled_ == nullptr) unspooled_ = entry;
   UpdateMax();
   Singleton< LogThread >::Instance()->Interrupt();
   return entry;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_ResetAllToUnspooled = "LogBuffer.ResetAllToUnspooled";

void LogBuffer::ResetAllToUnspooled()
{
   Debug::ft(LogBuffer_ResetAllToUnspooled);

   if(spooled_ != nullptr) unspooled_ = spooled_;
   spooled_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name LogBuffer_SetNext = "LogBuffer.SetNext";

void LogBuffer::SetNext(Entry* next)
{
   Debug::ft(LogBuffer_SetNext);

   next_ = next;
   next_->prev = nullptr;
   next_->next = nullptr;
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
