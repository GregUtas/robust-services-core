//==============================================================================
//
//  Log.cpp
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
#include "Log.h"
#include "Permanent.h"
#include <atomic>
#include <bitset>
#include <cctype>
#include <ios>
#include <istream>
#include <new>
#include <ostream>
#include <sstream>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "Debug.h"
#include "Element.h"
#include "Exception.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "LogBuffer.h"
#include "LogBufferRegistry.h"
#include "LogGroup.h"
#include "LogGroupRegistry.h"
#include "LogThread.h"
#include "Restart.h"
#include "Singleton.h"
#include "Statistics.h"
#include "SysConsole.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For logging errors that occur before the log system has been initialized.
//
static std::ostringstream StartupLog_;

//------------------------------------------------------------------------------
//
//  Returns StartupLog_ so that a log can be generated before the log system
//  has been initialized.
//
static ostringstreamPtr CreateStartupLog()
{
   auto reg = Singleton<LogGroupRegistry>::Extant();
   if(reg != nullptr) return nullptr;
   return ostringstreamPtr(&StartupLog_);
}

//------------------------------------------------------------------------------
//
//  Generates LOG if it was created before the log system was initialized.
//  Returns true if LOG was nullptr or if LOG was displayed.
//
static bool DisplayStartupLog(ostringstreamPtr& log)
{
   //  LOG is nullptr when no log should be generated.
   //
   if(log == nullptr) return true;

   //  This is a startup log if it references StartupLog_: that's a global,
   //  so release LOG to prevent any attempt to return it to the heap.
   //
   if(log.get() != &StartupLog_) return false;
   log.release();

   //  Display the log.
   //
   auto str = StartupLog_.str();
   StartupLog_.str(EMPTY_STR);
   if(str.back() != CRLF) str.push_back(CRLF);

   auto& console = SysConsole::Out();
   console << CRLF << "LOG during bootup:" << CRLF;
   console << str;
   return true;
}

//------------------------------------------------------------------------------
//
//  Data that changes too frequently to unprotect and reprotect memory
//  when it needs to be modified.
//
struct LogDynamic : public Permanent
{
   //  Initializes members.
   //
   LogDynamic() : interval_(1), sequence_(1) { }

   //  Determines whether the log should be suppressed or throttled.
   //  o 0: don't output the log
   //  o 1: output each log
   //  o n: output every nth log and suppress the others
   //
   uint16_t interval_;

   //  Counts occurrences of the log when interval_ is greater than 1.
   //
   uint16_t sequence_;
};

//------------------------------------------------------------------------------
//
//  Incremented when a log is created; assigned to it as a sequence number.
//
static std::atomic_size_t SeqNo_ = { 0 };

//> The maximum length of the string that explains a log.
//
constexpr size_t MaxExplSize = 48;

const col_t Log::Indent = 4;
const string Log::Tab = spaces(Indent);

//------------------------------------------------------------------------------

fn_name Log_ctor = "Log.ctor";

Log::Log(LogGroup* group, LogId id, c_string expl) :
   group_(group),
   id_(id),
   expl_(expl)
{
   Debug::ft(Log_ctor);

   //  Register the log after creating its statistics and checking
   //  that its identifier is valid and that EXPL isn't too long.
   //
   dyn_.reset(new LogDynamic);
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

   group_->BindLog(*this);
}

//------------------------------------------------------------------------------

fn_name Log_dtor = "Log.dtor";

Log::~Log()
{
   Debug::ftnt(Log_dtor);

   Debug::SwLog(Log_dtor, UnexpectedInvocation, 0);
   group_->UnbindLog(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Log::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Log*>(&local);
   return ptrdiff(&fake->lid_, fake);
}

//------------------------------------------------------------------------------

size_t Log::Count()
{
   return SeqNo_;
}

//------------------------------------------------------------------------------

ostringstreamPtr Log::Create(c_string groupName, LogId id)
{
   Debug::ftnt("Log.Create");

   //  Find the log's definition.
   //
   LogGroup* group = nullptr;
   auto log = Find(groupName, id, group);
   if(log == nullptr) return CreateStartupLog();

   //  Check if the log is to be suppressed or throttled.
   //
   if(group->Suppressed()) return log->Suppressed();

   if(log->dyn_->interval_ != 1)
   {
      if(log->dyn_->interval_ == 0) return log->Suppressed();
      if(--log->dyn_->sequence_ > 0) return log->Suppressed();
      log->dyn_->sequence_ = log->dyn_->interval_;
   }

   return log->Format(NoAlarm);
}

//------------------------------------------------------------------------------

ostringstreamPtr Log::Create
   (c_string groupName, LogId id, c_string alarmName, AlarmStatus status)
{
   Debug::ftnt("Log.Create(alarm)");

   //  Use the non-alarm version if no alarm is being set or cleared.
   //
   if(alarmName == nullptr) return Create(groupName, id);

   //  Find the log's and the alarm's definition.
   //
   LogGroup* group = nullptr;
   auto log = Find(groupName, id, group);
   if(log == nullptr) return CreateStartupLog();

   auto reg = Singleton<AlarmRegistry>::Extant();
   if(reg == nullptr) return nullptr;

   auto alarm = reg->Find(alarmName);
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
   if(dyn_->interval_ != 1) stream << " [INTERVAL=" << dyn_->interval_ << ']';
   stream << CRLF;
}

//------------------------------------------------------------------------------

void Log::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft("Log.DisplayStats");

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

Log* Log::Find(c_string groupName, LogId id, LogGroup*& group)
{
   Debug::ftnt("Log.Find");

   auto reg = Singleton<LogGroupRegistry>::Extant();
   if(reg == nullptr) return nullptr;
   group = reg->FindGroup(groupName);
   if(group == nullptr) return nullptr;
   return group->FindLog(id);
}

//------------------------------------------------------------------------------

constexpr size_t LogIdSize = 3;
constexpr size_t NameBegin = 1 + Log::Indent;
constexpr size_t MinNameSize = LogIdSize + 1;
const size_t NameSize = LogGroup::MaxNameSize + MinNameSize;

Log* Log::Find(c_string log)
{
   Debug::ft("Log.Find(log)");

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

ostringstreamPtr Log::Format(AlarmStatus status) const
{
   Debug::ftnt("Log.Format");

   ostringstreamPtr stream(new (std::nothrow) std::ostringstream);
   if(stream == nullptr) return nullptr;

   //  The first line of each log, after any alarm indicator, contains the
   //  log's group name and identifier, followed by the time and node on
   //  which it occurred, a sequence number, and a line feed that precedes
   //  log-specific data.
   //
   *stream << std::boolalpha << std::nouppercase << CRLF;
   *stream << AlarmStatusSymbol(status) << SPACE;
   *stream << group_->Name() << id_ << SPACE;
   *stream << Element::strTimePlace() << SPACE;
   *stream << '{' << ++SeqNo_ << '}' << CRLF;
   return stream;
}

//------------------------------------------------------------------------------

void Log::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Log::SetInterval(uint8_t interval)
{
   Debug::ft("Log.SetInterval");

   dyn_->interval_ = interval;
   dyn_->sequence_ = interval;
}

//------------------------------------------------------------------------------

void Log::Shutdown(RestartLevel level)
{
   Debug::ft("Log.Shutdown");

   //  Stop throttling or suppressing a log after a restart by
   //  using placement new to reset dyn_.
   //
   new (dyn_.get()) LogDynamic();

   //  Release items that may disappear during the restart.
   //
   FunctionGuard guard(Guard_ImmUnprotect);

   Restart::Release(bufferCount_);
   Restart::Release(suppressCount_);
   Restart::Release(discardCount_);
}

//------------------------------------------------------------------------------

void Log::Startup(RestartLevel level)
{
   Debug::ft("Log.Startup");

   //  Create items that may have disappeared during the restart.
   //
   FunctionGuard guard(Guard_ImmUnprotect);

   if(bufferCount_ == nullptr)
      bufferCount_.reset(new Counter("buffered"));
   if(suppressCount_ == nullptr)
      suppressCount_.reset(new Counter("suppressed"));
   if(discardCount_ == nullptr)
      discardCount_.reset(new Counter("discarded"));
}

//------------------------------------------------------------------------------

fn_name Log_Submit = "Log.Submit";

void Log::Submit(ostringstreamPtr& stream)
{
   Debug::ftnt(Log_Submit);

   if(DisplayStartupLog(stream)) return;

   auto str = stream->str();
   stream.reset();

   if(str.empty()) return;
   if(str.back() != CRLF) str.push_back(CRLF);

   auto log = Find(str.c_str());

   //  During a restart, LogThread won't run, so output the log
   //  directly instead of buffering it.
   //
   if(Restart::GetStage() != Running)
   {
      LogThread::Spool(str, log);

      if((log != nullptr) && (log->bufferCount_ != nullptr))
      {
         log->bufferCount_->Incr();
      }

      return;
   }

   if(log == nullptr)
   {
      //  This log has not been registered.
      //
      Debug::SwLog(Log_Submit, str.substr(0, 40), 0);
      return;
   }

   //  Add the log to the active log buffer.
   //
   auto buffer = Singleton<LogBufferRegistry>::Extant()->Active();
   if(buffer == nullptr) return;

   if(buffer->Push(str))
      log->bufferCount_->Incr();
   else
      log->discardCount_->Incr();
}

//------------------------------------------------------------------------------

ostringstreamPtr Log::Suppressed() const
{
   Debug::ftnt("Log.Suppressed");

   suppressCount_->Incr();
   return nullptr;
}

//------------------------------------------------------------------------------

main_t Log::TrapInMain(const Exception* ex,
   const std::exception* e, int code, const std::ostringstream* stack)
{
   auto& outdev = SysConsole::Out();

   outdev << CRLF << "FATAL EXCEPTION" << CRLF;

   if(e != nullptr)
   {
      outdev << spaces(2) << "type=" << e->what() << CRLF;
      if(code != 0) outdev << spaces(2) << "code=" << code << CRLF;
      if(ex != nullptr) ex->Display(outdev, spaces(2));
      if(stack != nullptr) outdev << stack->str();
   }
   else
   {
      outdev << spaces(2) << "unknown exception" << CRLF;
   }

   outdev << "Enter any string to continue\n";
   outdev << std::flush;

   std::string input;
   auto& indev = SysConsole::In();
   std::getline(indev, input);

   //  The system was dead on arrival, so return 0 to prevent automatic
   //  rebooting.
   //
   return 0;
}
}
