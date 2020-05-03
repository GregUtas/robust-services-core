//==============================================================================
//
//  Daemon.cpp
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
#include "Daemon.h"
#include <sstream>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "DaemonRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "InitThread.h"
#include "Log.h"
#include "NbLogs.h"
#include "Restart.h"
#include "Singleton.h"
#include "ThreadAdmin.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Daemon_ctor = "Daemon.ctor";

Daemon::Daemon(fixed_string name, size_t size) :
   name_(name),
   size_(size),
   traps_(0),
   alarm_(nullptr)
{
   Debug::ft(Daemon_ctor);

   if(name == nullptr)
   {
      Debug::SwLog(Daemon_ctor, "null name", 0);
      return;
   }

   Singleton< DaemonRegistry >::Instance()->BindDaemon(*this);
   EnsureAlarm();
}

//------------------------------------------------------------------------------

fn_name Daemon_dtor = "Daemon.dtor";

Daemon::~Daemon()
{
   Debug::ft(Daemon_dtor);

   Debug::SwLog(Daemon_dtor, UnexpectedInvocation, 0);
   Singleton< DaemonRegistry >::Instance()->UnbindDaemon(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Daemon::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Daemon* >(&local);
   return ptrdiff(&fake->did_, fake);
}

//------------------------------------------------------------------------------

fn_name Daemon_CreateThread = "Daemon.CreateThread";

Thread* Daemon::CreateThread()
{
   Debug::ft(Daemon_CreateThread);

   Debug::SwLog(Daemon_CreateThread, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Daemon_CreateThreads = "Daemon.CreateThreads";

void Daemon::CreateThreads()
{
   Debug::ft(Daemon_CreateThreads);

   switch(traps_)
   {
   case 0:
      break;

   case 1:
      //  CreateThread trapped.  Give the subclass a chance to
      //  repair any data before invoking CreateThread again.
      //
      ++traps_;
      Recover();
      --traps_;
      break;

   default:
      //  Either Recover trapped or CreateThread trapped again.
      //  Raise an alarm.
      //
      RaiseAlarm(GetAlarmLevel());
      return;
   }

   //  Try to create new threads to replace those that exited.
   //  Incrementing traps_, and clearing it on success, allows
   //  us to detect traps.
   //
   while(threads_.size() < size_)
   {
      ++traps_;
      auto thread = CreateThread();
      traps_ = 0;

      if(thread == nullptr)
      {
         RaiseAlarm(GetAlarmLevel());
         return;
      };

      threads_.insert(thread);
      ThreadAdmin::Incr(ThreadAdmin::Recreations);
   }

   RaiseAlarm(NoAlarm);
}

//------------------------------------------------------------------------------

fn_name Daemon_Disable = "Daemon.Disable";

void Daemon::Disable()
{
   Debug::ft(Daemon_Disable);

   //  This is a bit of a kludge but fits in will with the overall logic.
   //
   traps_ = 2;
}

//------------------------------------------------------------------------------

void Daemon::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "name  : " << name_ << CRLF;
   stream << prefix << "did   : " << did_.to_str() << CRLF;
   stream << prefix << "size  : " << size_ << CRLF;
   stream << prefix << "traps : " << int(traps_) << CRLF;
   stream << prefix << "alarm : " << strObj(alarm_) << CRLF;
   stream << prefix << "threads [ThreadId]" << CRLF;

   auto lead = prefix + spaces(2);

   for(auto t = threads_.cbegin(); t != threads_.cend(); ++t)
   {
      stream << lead << strIndex((*t)->Tid()) << strObj(*t) << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name Daemon_Enable = "Daemon.Enable";

void Daemon::Enable()
{
   Debug::ft(Daemon_Enable);

   auto enabling = (traps_ >= 2);
   traps_ = 0;

   if(enabling)
   {
      CreateThreads();
   }
}

//------------------------------------------------------------------------------

fn_name Daemon_EnsureAlarm = "Daemon.EnsureAlarm";

void Daemon::EnsureAlarm()
{
   Debug::ft(Daemon_EnsureAlarm);

   //  If the thread unavailable alarm is not registered, create it.
   //
   auto reg = Singleton< AlarmRegistry >::Instance();
   auto alarmName = "DAEMON" + std::to_string(Did());
   alarm_ = reg->Find(alarmName);

   if(alarm_ == nullptr)
   {
      auto alarmExpl = "Thread(s) unavailable: " + alarmName;
      FunctionGuard guard(Guard_ImmUnprotect);
      alarm_ = new Alarm(alarmName.c_str(), alarmExpl.c_str(), 0);
   }
}

//------------------------------------------------------------------------------

fn_name Daemon_Find = "Daemon.Find";

Daemon::Iterator Daemon::Find(Thread* thread)
{
   Debug::ft(Daemon_Find);

   if(thread == nullptr)
   {
      Debug::SwLog(Daemon_Find, "null thread", 0);
      return threads_.end();
   }

   return threads_.find(thread);
}

//------------------------------------------------------------------------------

fn_name Daemon_GetAlarmLevel = "Daemon.GetAlarmLevel";

AlarmStatus Daemon::GetAlarmLevel() const
{
   Debug::ft(Daemon_GetAlarmLevel);

   return (threads_.empty() ? MajorAlarm : MinorAlarm);
}

//------------------------------------------------------------------------------

void Daemon::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Daemon_RaiseAlarm = "Daemon.RaiseAlarm";

void Daemon::RaiseAlarm(AlarmStatus level) const
{
   Debug::ft(Daemon_RaiseAlarm);

   auto id = (level == CriticalAlarm ? ThreadCriticalDeath : ThreadUnavailable);

   auto log = alarm_->Create(ThreadLogGroup, id, level);

   if(log != nullptr)
   {
      *log << Log::Tab << "daemon=" << name_;
      *log << " target=" << size_;
      *log << " actual=" << threads_.size() << CRLF;
      Log::Submit(log);
   }

   if(level == CriticalAlarm)
   {
      Restart::Initiate(DeathOfCriticalThread, Did());
   }
}

//------------------------------------------------------------------------------

fn_name Daemon_Startup = "Daemon.Startup";

void Daemon::Startup(RestartLevel level)
{
   Debug::ft(Daemon_Startup);

   traps_ = 0;
   EnsureAlarm();
}

//------------------------------------------------------------------------------

fn_name Daemon_ThreadCreated = "Daemon.ThreadCreated";

void Daemon::ThreadCreated(Thread* thread)
{
   Debug::ft(Daemon_ThreadCreated);

   threads_.insert(thread);
}

//------------------------------------------------------------------------------

fn_name Daemon_ThreadDeleted = "Daemon.ThreadDeleted";

void Daemon::ThreadDeleted(Thread* thread)
{
   Debug::ft(Daemon_ThreadDeleted);

   //  This does not immediately recreate the deleted thread.  We only create
   //  threads when invoked by InitThread, which is not the case here.  So we
   //  must ask InitThread to invoke us.  During a restart, however, threads
   //  often exit, so there is no point doing this, and InitThread will soon
   //  invoke our Startup function so that we can create threads.
   //
   auto item = Find(thread);

   if(item != threads_.end())
   {
      threads_.erase(item);
      if(Restart::GetStatus() != Running) return;
      Singleton< InitThread >::Instance()->Interrupt(InitThread::RecreateMask);
   }
}
}
