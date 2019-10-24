//==============================================================================
//
//  ModuleRegistry.cpp
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
#include "ModuleRegistry.h"
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include "Clock.h"
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "Module.h"
#include "NbLogs.h"
#include "Restart.h"
#include "Singleton.h"
#include "ThisThread.h"
#include "Thread.h"
#include "ThreadRegistry.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name ModuleRegistry_ctor = "ModuleRegistry.ctor";

ModuleRegistry::ModuleRegistry() :
   reason_(NilRestart),
   errval_(0),
   stream_(nullptr)
{
   Debug::ft(ModuleRegistry_ctor);

   modules_.Init(Module::MaxId + 1, Module::CellDiff(), MemImm);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_dtor = "ModuleRegistry.dtor";

ModuleRegistry::~ModuleRegistry()
{
   Debug::ft(ModuleRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_BindModule = "ModuleRegistry.BindModule";

void ModuleRegistry::BindModule(Module& module)
{
   Debug::ft(ModuleRegistry_BindModule);

   modules_.Insert(module);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_CalcLevel = "ModuleRegistry.CalcLevel";

RestartLevel ModuleRegistry::CalcLevel() const
{
   Debug::ft(ModuleRegistry_CalcLevel);

   //  A restart was initiated when the system was in service.  Determine
   //  the restart level based on what caused the restart.
   //
   if(reason_ == ManualRestart)
   {
      //  CLI >restart command.  errval_ is the restart level.
      //
      switch(errval_)
      {
      case RestartNil:
         Debug::SwLog(ModuleRegistry_CalcLevel, reason_, errval_);
         return RestartNil;

      case RestartWarm:
      case RestartCold:
      case RestartReload:
      case RestartReboot:
      case RestartExit:
         return RestartLevel(errval_);

      default:
         Debug::SwLog(ModuleRegistry_CalcLevel, reason_, errval_);
         return RestartWarm;
      }
   }

   //  PayloadFaction queue corruptions require a cold restart to reset
   //  these queues and delete all objects associated with them.
   //
   if(reason_ == WorkQueueCorruption) return RestartCold;
   if(reason_ == TimerQueueCorruption) return RestartCold;

   if((reason_ == RestartTimeout) || (reason_ == ModuleStartupFailed))
   {
      //  These reasons should not occur when the system is in service.
      //
      Debug::SwLog(ModuleRegistry_CalcLevel, reason_, errval_);
      return RestartReboot;
   }

   //  A warm restart is the default.
   //
   return RestartWarm;
}

//------------------------------------------------------------------------------

void ModuleRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "TicksZero : " << Clock::TicksZero() << CRLF;
   stream << prefix << "Status    : " << Restart::Status_ << CRLF;
   stream << prefix << "Level     : " << Restart::Level_ << CRLF;
   stream << prefix << "reason    : " << reason_ << CRLF;
   stream << prefix << "errval    : " << errval_ << CRLF;
   stream << prefix << "stream    : " << stream_.get() << CRLF;
   stream << prefix << "modules [ModuleId]" << CRLF;
   modules_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Module* ModuleRegistry::GetModule(ModuleId mid) const
{
   return modules_.At(mid);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_NextLevel = "ModuleRegistry.NextLevel";

RestartLevel ModuleRegistry::NextLevel()
{
   Debug::ft(ModuleRegistry_NextLevel);

   switch(Restart::Level_)
   {
   case RestartWarm:
      return RestartCold;
   case RestartCold:
      return RestartReload;

   case RestartReload:
   case RestartReboot:
      //
      //  These cause a reboot.
      //
   case RestartNil:
   case RestartExit:
   default:
      //
      //  This functions is invoked when a restart is already underway,
      //  so these values should not occur.
      //
      return RestartReboot;
   }
}

//------------------------------------------------------------------------------

void ModuleRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_Restart = "ModuleRegistry.Restart";

void ModuleRegistry::Restart()
{
   Debug::ft(ModuleRegistry_Restart);

   auto reentered = true;

   while(true)
   {
      switch(Restart::Status_)
      {
      case Initial:
         Thread::RestrictFactions(true);
         reentered = false;
         Restart::Level_ = RestartReboot;
         Restart::Status_ = StartingUp;
         break;

      case StartingUp:
         Thread::RestrictFactions(true);
         if(reentered)
         {
            Restart::Status_ = ShuttingDown;
            break;
         }

         Startup(Restart::Level_);
         {
            auto log = Log::Create(NodeLogGroup, NodeRunning);
            if(log != nullptr) Log::Submit(log);
         }
         Restart::Level_ = RestartNil;
         Restart::Status_ = Running;
         Thread::RestrictFactions(false);
         return;

      case Running:
         reentered = false;
         Restart::Level_ = CalcLevel();
         if(Restart::Level_ == RestartNil) return;
         Restart::Status_ = ShuttingDown;
         break;

      case ShuttingDown:
         Thread::RestrictFactions(true);
         if(reentered) Restart::Level_ = NextLevel();
         Shutdown(Restart::Level_);
         reentered = false;
         Restart::Status_ = StartingUp;
         break;
      }
   }
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_SetReason = "ModuleRegistry.SetReason";

void ModuleRegistry::SetReason(reinit_t reason, debug32_t errval)
{
   Debug::ft(ModuleRegistry_SetReason);

   reason_ = reason;
   errval_ = errval;
}

//------------------------------------------------------------------------------

fixed_string ShutdownHeader =
   "MODULE SHUTDOWN                msecs      invoked at";
// 0         1         2         3         4         5
// 01234567890123456789012345678901234567890123456789012

fixed_string ShutdownTotalStr = "total shutdown time";
fixed_string NotifyingThreadsStr = "Notifying threads...";
fixed_string ExitingThreadsStr = "...threads to exit: ";
fixed_string ExitedThreadsStr = "...threads exited: ";
fixed_string ShutdownStr = "...shut down";

fn_name ModuleRegistry_Shutdown = "ModuleRegistry.Shutdown";

void ModuleRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(ModuleRegistry_Shutdown);

   for(size_t tries = 80, idle = 0; (tries > 0) && (idle <= 8); --tries)
   {
      ThisThread::Pause(25);
      if(Thread::SwitchContext())
         idle = 0;
      else
         ++idle;
   }

   auto zeroTime = Clock::TicksNow();
   *Stream() << CRLF << "RESTART TYPE: " << strRestartLevel(level) << CRLF;
   *Stream() << CRLF << ShutdownHeader << CRLF;

   //  Notify all threads of the restart.
   //
   *Stream() << NotifyingThreadsStr << setw(52 - strlen(NotifyingThreadsStr));
   *Stream() << Clock::TicksToTime(zeroTime) << CRLF;
   Log::Submit(stream_);

   auto reg = Singleton< ThreadRegistry >::Instance();
   auto before = reg->Threads().Size();
   auto planned = reg->Restarting(level);
   size_t actual = 0;

   //  Report PLANNED, the number of threads that plan to exit.  Sleep and
   //  try to schedule another thread upon waking up.  Stop when the planned
   //  number of threads have exited or after trying to schedule exiting
   //  threads for 3 seconds.
   //
   *Stream() << ExitingThreadsStr << setw(2) << planned;
   *Stream() << setw(36 - (strlen(ExitingThreadsStr) + 2));
   *Stream() << Clock::TicksToMsecs(Clock::TicksSince(zeroTime)) << CRLF;
   Log::Submit(stream_);

   Thread::RestrictFactions(false);
      for(size_t tries = 120; tries > 0; --tries)
      {
         Thread::SwitchContext();
         ThisThread::Pause(25);
         actual = before - reg->Threads().Size();
         if(actual >= planned) break;
      }
   Thread::RestrictFactions(true);

   actual = before - reg->Threads().Size();
   *Stream() << CRLF << ExitedThreadsStr << setw(2) << actual;
   *Stream() << setw(36 - (strlen(ExitedThreadsStr) + 2));
   *Stream() << Clock::TicksToMsecs(Clock::TicksSince(zeroTime)) << CRLF;
   Log::Submit(stream_);

   //  Modules must be shut down in reverse order of their initialization.
   //
   for(auto m = modules_.Last(); m != nullptr; modules_.Prev(m))
   {
      auto time = Clock::TicksNow();
      auto name = strClass(m) + "...";
      *Stream() << name << setw(52 - name.size());
      *Stream() << Clock::TicksToTime(time) << CRLF;
      Log::Submit(stream_);

      m->Shutdown(level);

      *Stream() << ShutdownStr << setw(36 - strlen(ShutdownStr));
      *Stream() << Clock::TicksToMsecs(Clock::TicksSince(time)) << CRLF;
      Log::Submit(stream_);
   }

   *Stream() << setw(36) << string(5, '-') << CRLF;
   auto width = strlen(ShutdownTotalStr);
   auto msecs = Clock::TicksToMsecs(Clock::TicksSince(zeroTime));
   *Stream() << ShutdownTotalStr << setw(36 - width) << msecs << CRLF;
   Log::Submit(stream_);
}

//------------------------------------------------------------------------------

fixed_string StartupHeader =
   "MODULE INITIALIZATION          msecs      invoked at";
// 0         1         2         3         4         5
// 01234567890123456789012345678901234567890123456789012

fixed_string StartupTotalStr = "total initialization time";
fixed_string PreModuleStr = "pre-Module.Startup";
fixed_string InitializedStr = "...initialized";

fn_name ModuleRegistry_Startup = "ModuleRegistry.Startup";

void ModuleRegistry::Startup(RestartLevel level)
{
   Debug::ft(ModuleRegistry_Startup);

   auto zeroTime =
      (level >= RestartReboot ? Clock::TicksZero() : Clock::TicksNow());
   *Stream() << CRLF << StartupHeader << CRLF;

   if(level >= RestartReboot)
   {
      auto msecs = Clock::TicksToMsecs(Clock::TicksSince(zeroTime));
      *Stream() << PreModuleStr << setw(36 - strlen(PreModuleStr)) << msecs;
      *Stream() << setw(16) << Clock::TicksToTime(zeroTime) << CRLF;
   }

   for(auto m = modules_.First(); m != nullptr; modules_.Next(m))
   {
      auto time = Clock::TicksNow();
      auto name = strClass(m) + "...";
      *Stream() << name << setw(52 - name.size());
      *Stream() << Clock::TicksToTime(time) << CRLF;
      Log::Submit(stream_);

      m->Startup(level);

      *Stream() << InitializedStr << setw(36 - strlen(InitializedStr));
      *Stream() << Clock::TicksToMsecs(Clock::TicksSince(time)) << CRLF;
      Log::Submit(stream_);
   }

   *Stream() << setw(36) << string(5, '-') << CRLF;
   auto width = strlen(StartupTotalStr);
   auto msecs = Clock::TicksToMsecs(Clock::TicksSince(zeroTime));
   *Stream() << StartupTotalStr << setw(36 - width) << msecs << CRLF;
   Log::Submit(stream_);
}

//------------------------------------------------------------------------------

std::ostringstream* ModuleRegistry::Stream()
{
   if(stream_ == nullptr)
   {
      stream_.reset(new std::ostringstream);
      *stream_ << std::boolalpha << std::nouppercase;
   }

   return stream_.get();
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_UnbindModule = "ModuleRegistry.UnbindModule";

void ModuleRegistry::UnbindModule(Module& module)
{
   Debug::ft(ModuleRegistry_UnbindModule);

   modules_.Erase(module);
}
}
