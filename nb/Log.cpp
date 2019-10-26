//==============================================================================
//
//  Log.cpp
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
#include "Log.h"
#include <bitset>
#include <cctype>
#include <ios>
#include <sstream>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "LogBuffer.h"
#include "LogBufferRegistry.h"
#include "LogGroup.h"
#include "LogGroupRegistry.h"
#include "LogThread.h"
#include "Restart.h"
#include "Singleton.h"
#include "Statistics.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t Log::MaxExplSize = 48;
const col_t Log::Indent = 4;
const string Log::Tab = spaces(Log::Indent);
std::atomic_size_t Log::SeqNo_ = 0;

//------------------------------------------------------------------------------

fn_name Log_ctor = "Log.ctor";

Log::Log(LogGroup* group, LogId id, fixed_string expl) :
   group_(group),
   id_(id),
   expl_(expl),
   interval_(1),
   sequence_(1)
{
   Debug::ft(Log_ctor);

   //  Register the log after creating its statistics and checking
   //  that its identifier is valid and that EXPL isn't too long.
   //
   bufferCount_.reset(new Counter("buffered"));
   suppressCount_.reset(new Counter("suppressed"));
   discardCount_.reset(new Counter("discarded"));

   if((id_ > MaxId) || (id < TroubleLog))
   {
      Debug::SwLog(Log_ctor, "invalid LogId", id_);
   }

   if(expl_.size() > MaxExplSize)
   {
      Debug::SwLog(Log_ctor, "expl length", expl_.size());
   }

   if(!group_->BindLog(*this))
   {
      Debug::SwLog(Log_ctor, group_->Name(), id_);
   }
}

//------------------------------------------------------------------------------

fn_name Log_dtor = "Log.dtor";

Log::~Log()
{
   Debug::ft(Log_dtor);

   group_->UnbindLog(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Log::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Log* >(&local);
   return ptrdiff(&fake->lid_, fake);
}

//------------------------------------------------------------------------------

fn_name Log_Create1 = "Log.Create";

ostringstreamPtr Log::Create(fixed_string groupName, LogId id)
{
   Debug::ft(Log_Create1);

   //  Find the log's definition.
   //
   LogGroup* group = nullptr;
   auto log = Find(groupName, id, group);
   if(log == nullptr) return nullptr;

   //  Check if the log is to be suppressed or throttled.
   //
   if(group->Suppressed()) return log->Suppressed();

   if(log->interval_ != 1)
   {
      if(log->interval_ == 0) return log->Suppressed();
      if(--log->sequence_ > 0) return log->Suppressed();
      log->sequence_ = log->interval_;
   }

   return log->Format(NoAlarm);
}

//------------------------------------------------------------------------------

fn_name Log_Create2 = "Log.Create(alarm)";

ostringstreamPtr Log::Create(fixed_string groupName,
   LogId id, fixed_string alarmName, AlarmStatus status)
{
   Debug::ft(Log_Create2);

   //  Use the non-alarm version if no alarm is being set or cleared.
   //
   if(alarmName == nullptr) return Create(groupName, id);

   //  Find the log's and the alarm's definition.
   //
   LogGroup* group = nullptr;
   auto log = Find(groupName, id, group);
   if(log == nullptr) return nullptr;

   auto alarm = Singleton< AlarmRegistry >::Instance()->Find(alarmName);
   if(alarm == nullptr) return nullptr;

   //  Create the log's header and add the alarm name on the second line.
   //
   auto stream = log->Format(status);

   if(stream != nullptr)
   {
      *stream << Tab << "Alarm ";
      *stream << (status != NoAlarm ? "ON" : "OFF") << ": ";
      *stream << alarm->Name() << " (" << alarm->Expl() << ')' << CRLF;
   }

   return stream;
}

//------------------------------------------------------------------------------

void Log::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << group_->Name() << id_ << " (" << expl_ << ')';
   if(interval_ != 1) stream << " [INTERVAL=" << interval_ << ']';
   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Log_DisplayStats = "Log.DisplayStats";

void Log::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft(Log_DisplayStats);

   if(!options.test(DispVerbose))
   {
      if(bufferCount_->Overall() +
         suppressCount_->Overall() + discardCount_->Overall() == 0) return;
   }

   stream << spaces(4) << id_ << CRLF;
   bufferCount_->DisplayStat(stream, options);
   suppressCount_->DisplayStat(stream, options);
   discardCount_->DisplayStat(stream, options);
}

//------------------------------------------------------------------------------

fn_name Log_Find = "Log.Find";

Log* Log::Find(fixed_string groupName, LogId id, LogGroup*& group)
{
   Debug::ft(Log_Find);

   auto reg = Singleton< LogGroupRegistry >::Instance();
   group = reg->FindGroup(groupName);
   if(group == nullptr) return nullptr;
   return group->FindLog(id);
}

//------------------------------------------------------------------------------

const size_t LogIdSize = 3;
const size_t NameBegin = 1 + Log::Indent;
const size_t NameSize = LogGroup::MaxNameSize + LogIdSize + 1;
const size_t MinNameSize = 1 + LogIdSize;

Log* Log::Find(fixed_string log)
{
   Debug::ft(Log_Find);

   //  The log's name starts at LOG[Indent + 1], after the <CRLF> at the
   //  start of the log and the field for an alarm status.  Find END, the
   //  position of the space that follows the name, and then extract the
   //  log's number and group name.  Convert the number to a LogId and
   //  find the log's definition.
   //
   string front(&log[NameBegin], NameSize);
   auto end = front.find(SPACE);
   if(end == string::npos) return nullptr;
   if(end < MinNameSize) return nullptr;
   auto number = front.substr(end - LogIdSize, LogIdSize);
   auto name = front.substr(0, end - LogIdSize);

   LogId id = 0;

   for(size_t i = 0; i < number.size(); ++i)
   {
      if(!isdigit(number[i])) return nullptr;
      id = (id * 10) + (number[i] - '0');
   }

   LogGroup* group = nullptr;
   return Find(name.c_str(), id, group);
}

//------------------------------------------------------------------------------

fn_name Log_Format = "Log.Format";

ostringstreamPtr Log::Format(AlarmStatus status) const
{
   Debug::ft(Log_Format);

   ostringstreamPtr stream(new std::ostringstream);

   //  The first line of each log, after any alarm indicator, contains the
   //  log's group name and identifier, followed by the time and node on
   //  which it occurred, a sequence number, and a line feed that precedes
   //  log-specific data.
   //
   if(stream != nullptr)
   {
      *stream << std::boolalpha << std::nouppercase << CRLF;
      *stream << AlarmStatusSymbol(status);
      *stream << group_->Name() << id_ << SPACE;
      *stream << Element::strTimePlace() << SPACE;
      *stream << '{' << ++SeqNo_ << '}' << CRLF;
   }

   return stream;
}

//------------------------------------------------------------------------------

void Log::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Log_SetInterval = "Log.SetInterval";

void Log::SetInterval(uint8_t interval)
{
   Debug::ft(Log_SetInterval);

   interval_ = interval;
   sequence_ = interval;
}

//------------------------------------------------------------------------------

fn_name Log_Shutdown = "Log.Shutdown";

void Log::Shutdown(RestartLevel level)
{
   Debug::ft(Log_Shutdown);

   //  A log survives a warm restart but should not continue to be throttled
   //  or suppressed.
   //
   interval_ = 1;
   sequence_ = 1;
}

//------------------------------------------------------------------------------

fn_name Log_Submit = "Log.Submit";

void Log::Submit(ostringstreamPtr& stream)
{
   Debug::ft(Log_Submit);

   if(stream == nullptr) return;
   if(stream->str().back() != CRLF) *stream << CRLF;

   auto log = Find(stream->str().c_str());

   //  During a restart, LogThread won't run, so output the log
   //  directly instead of buffering it.
   //
   if(Restart::GetStatus() != Running)
   {
      LogThread::Spool(stream);
      if(log != nullptr) log->bufferCount_->Incr();
      return;
   }

   if(log == nullptr)
   {
      //  This log has not been registered.
      //
      Debug::SwLog(Log_Submit, stream->str().substr(0, 40), 0);
      return;
   }

   //  Add the log to the active log buffer.
   //
   auto buffer = Singleton< LogBufferRegistry >::Instance()->Active();

   if(buffer->Push(stream))
      log->bufferCount_->Incr();
   else
      log->discardCount_->Incr();
}

//------------------------------------------------------------------------------

fn_name Log_Suppressed = "Log.Suppressed";

ostringstreamPtr Log::Suppressed() const
{
   Debug::ft(Log_Suppressed);

   suppressCount_->Incr();
   return nullptr;
}
}
