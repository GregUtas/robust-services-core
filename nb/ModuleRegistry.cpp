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
#include <iosfwd>
#include <sstream>
#include <string>
#include "Debug.h"
#include "Duration.h"
#include "Formatters.h"
#include "Log.h"
#include "MainArgs.h"
#include "Memory.h"
#include "Module.h"
#include "NbLogs.h"
#include "Restart.h"
#include "Singleton.h"
#include "ThisThread.h"
#include "Thread.h"
#include "ThreadRegistry.h"
#include "TimePoint.h"

using std::ostream;
using std::setw;
using std::string;

namespace NodeBase
{
//  The following return the set of factions that can be scheduled during
//  various scenarios.
//
const FactionFlags& NoFactions = FactionFlags();

//------------------------------------------------------------------------------

const FactionFlags& AllFactions()
{
   static FactionFlags AllFactions_ = FactionFlags();

   //  RootThread and InitThread are not scheduled but run whenever ready
   //  because of their higher priority.
   //
   if(AllFactions_.none())
   {
      for(auto f = 0; f < SystemFaction; ++f) AllFactions_.set(f, true);
   }

   return AllFactions_;
}

//------------------------------------------------------------------------------

const FactionFlags& ShutdownFactions()
{
   static FactionFlags ShutdownFactions_ = FactionFlags();

   if(ShutdownFactions_.none())
   {
      ShutdownFactions_.set(BackgroundFaction, true);
      ShutdownFactions_.set(OperationsFaction, true);
      ShutdownFactions_.set(MaintenanceFaction, true);
   }

   return ShutdownFactions_;
}

//==============================================================================
//
//  The minimum level specified when a restart was initiated.
//
RestartLevel level_ = RestartNone;

//  A stream for recording the progress of system initialization.
//
ostringstreamPtr stream_ = nullptr;

//------------------------------------------------------------------------------
//
//  Returns stream_, creating it if it doesn't exist.
//
std::ostringstream* Stream()
{
   if(stream_ == nullptr)
   {
      stream_.reset(new std::ostringstream);
      *stream_ << std::boolalpha << std::nouppercase;
   }

   return stream_.get();
}

//==============================================================================

fn_name ModuleRegistry_ctor = "ModuleRegistry.ctor";

ModuleRegistry::ModuleRegistry()
{
   Debug::ft(ModuleRegistry_ctor);

   modules_.Init(Module::MaxId, Module::CellDiff(), MemImmutable);

   //  The creation of this registry means that immutable memory is now
   //  available, so create MainArgs in order to save main()'s arguments
   //  in immutable memory.
   //
   Singleton< MainArgs >::Instance();
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_dtor = "ModuleRegistry.dtor";

ModuleRegistry::~ModuleRegistry()
{
   Debug::ftnt(ModuleRegistry_dtor);

   Debug::SwLog(ModuleRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_BindModule = "ModuleRegistry.BindModule";

void ModuleRegistry::BindModule(Module& module)
{
   Debug::ft(ModuleRegistry_BindModule);

   modules_.Insert(module);
}

//------------------------------------------------------------------------------

void ModuleRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "TimeZero : " << TimePoint::TimeZero().Ticks() << CRLF;
   stream << prefix << "Stage    : " << Restart::Stage_ << CRLF;
   stream << prefix << "Level    : " << Restart::Level_ << CRLF;
   stream << prefix << "stream   : " << stream_.get() << CRLF;

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
      return RestartReboot;

   default:
      return RestartExit;
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
      switch(Restart::Stage_)
      {
      case Launching:
         Thread::EnableFactions(NoFactions);
         reentered = false;
         Restart::Level_ = RestartReboot;
         Restart::Stage_ = StartingUp;
         break;

      case StartingUp:
         Thread::EnableFactions(NoFactions);
         if(reentered)
         {
            Restart::Stage_ = ShuttingDown;
            break;
         }

         Startup(Restart::Level_);
         {
            auto log = Log::Create(NodeLogGroup, NodeRunning);
            if(log != nullptr) Log::Submit(log);
         }
         Restart::Level_ = RestartNone;
         Restart::Stage_ = Running;
         Thread::EnableFactions(AllFactions());
         return;

      case Running:
         reentered = false;
         Restart::Level_ = level_;
         if(Restart::Level_ == RestartNone) return;
         Restart::Stage_ = ShuttingDown;
         break;

      case ShuttingDown:
         Thread::EnableFactions(NoFactions);
         if(reentered) Restart::Level_ = NextLevel();
         Shutdown(Restart::Level_);
         reentered = false;
         Restart::Stage_ = StartingUp;
         break;
      }
   }
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_SetLevel = "ModuleRegistry.SetLevel";

void ModuleRegistry::SetLevel(RestartLevel level)
{
   Debug::ft(ModuleRegistry_SetLevel);

   level_ = level;
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
const size_t ShutdownStrSize = strlen(ShutdownStr);

fn_name ModuleRegistry_Shutdown = "ModuleRegistry.Shutdown";

void ModuleRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(ModuleRegistry_Shutdown);

   if(level >= RestartReload)
   {
      Memory::Unprotect(MemProtected);
   }

   Duration delay(25, mSECS);

   //  Schedule a subset of the factions so that pending logs will be output.
   //
   Thread::EnableFactions(ShutdownFactions());
      for(size_t tries = 120, idle = 0; (tries > 0) && (idle <= 8); --tries)
      {
         ThisThread::Pause(delay);
         if(Thread::SwitchContext() != nullptr)
            idle = 0;
         else
            ++idle;
      }
   Thread::EnableFactions(NoFactions);

   auto zeroTime = TimePoint::Now();
   *Stream() << CRLF << "RESTART TYPE: " << level << CRLF;
   *Stream() << CRLF << ShutdownHeader << CRLF;

   //  Notify all threads of the restart.
   //
   *Stream() << NotifyingThreadsStr << setw(52 - strlen(NotifyingThreadsStr));
   *Stream() << zeroTime.to_str() << CRLF;
   Log::Submit(stream_);

   auto reg = Singleton< ThreadRegistry >::Instance();
   auto before = reg->Threads().Size();
   auto planned = reg->Restarting(level);
   size_t actual = 0;

   //  Report PLANNED, the number of threads that plan to exit.  Schedule
   //  threads until the planned number have exited.  If some fail to exit,
   //  RootThread will time out and escalate the restart.
   //
   *Stream() << ExitingThreadsStr << setw(2) << planned;
   *Stream() << setw(36 - (strlen(ExitingThreadsStr) + 2));
   auto elapsed = TimePoint::Now() - zeroTime;
   *Stream() << elapsed.To(mSECS) << CRLF;
   Log::Submit(stream_);

   Thread::EnableFactions(AllFactions());
      while(actual < planned)
      {
         Thread::SwitchContext();
         ThisThread::Pause(delay);
         actual = before - reg->Threads().Size();
      }
   Thread::EnableFactions(NoFactions);

   actual = before - reg->Threads().Size();
   *Stream() << CRLF << ExitedThreadsStr << setw(2) << actual;
   *Stream() << setw(36 - (strlen(ExitedThreadsStr) + 2));
   elapsed = TimePoint::Now() - zeroTime;
   *Stream() << elapsed.To(mSECS) << CRLF;
   Log::Submit(stream_);

   //  Modules must be shut down in reverse order of their initialization.
   //
   for(auto m = modules_.Last(); m != nullptr; modules_.Prev(m))
   {
      auto time = TimePoint::Now();
      auto name = strClass(m) + "...";
      *Stream() << name << setw(52 - name.size());
      *Stream() << time.to_str() << CRLF;
      Log::Submit(stream_);

      m->Shutdown(level);

      *Stream() << ShutdownStr << setw(36 - ShutdownStrSize);
      elapsed = TimePoint::Now() - time;
      *Stream() << elapsed.To(mSECS) << CRLF;
      Log::Submit(stream_);
   }

   *Stream() << setw(36) << string(5, '-') << CRLF;
   auto width = strlen(ShutdownTotalStr);
   elapsed = TimePoint::Now() - zeroTime;
   *Stream() << ShutdownTotalStr;
   *Stream() << setw(36 - width) << elapsed.To(mSECS) << CRLF;
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
const size_t InitializedStrSize = strlen(InitializedStr);

fn_name ModuleRegistry_Startup = "ModuleRegistry.Startup";

void ModuleRegistry::Startup(RestartLevel level)
{
   Debug::ft(ModuleRegistry_Startup);

   auto zeroTime =
      (level >= RestartReboot ? TimePoint::TimeZero() : TimePoint::Now());
   *Stream() << CRLF << StartupHeader << CRLF;

   if(level >= RestartReboot)
   {
      auto elapsed = TimePoint::Now() - zeroTime;
      *Stream() << PreModuleStr;
      *Stream() << setw(36 - strlen(PreModuleStr)) << elapsed.To(mSECS);
      *Stream() << setw(16) << zeroTime.to_str() << CRLF;
   }

   for(auto m = modules_.First(); m != nullptr; modules_.Next(m))
   {
      auto time = TimePoint::Now();
      auto name = strClass(m) + "...";
      *Stream() << name << setw(52 - name.size());
      *Stream() << time.to_str() << CRLF;
      Log::Submit(stream_);

      m->Startup(level);

      auto elapsed = TimePoint::Now() - time;
      *Stream() << InitializedStr << setw(36 - InitializedStrSize);
      *Stream() << elapsed.To(mSECS) << CRLF;
      Log::Submit(stream_);
   }

   //  Write-protect memory segments that are read-only while in service.
   //
   Memory::Protect(MemImmutable);
   Memory::Protect(MemProtected);

   *Stream() << setw(36) << string(5, '-') << CRLF;
   auto width = strlen(StartupTotalStr);
   auto elapsed = TimePoint::Now() - zeroTime;
   *Stream() << StartupTotalStr;
   *Stream() << setw(36 - width) << elapsed.To(mSECS) << CRLF;
   Log::Submit(stream_);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_UnbindModule = "ModuleRegistry.UnbindModule";

void ModuleRegistry::UnbindModule(Module& module)
{
   Debug::ftnt(ModuleRegistry_UnbindModule);

   modules_.Erase(module);
}
}
