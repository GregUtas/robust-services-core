//==============================================================================
//
//  ModuleRegistry.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "ModuleRegistry.h"
#include "CfgStrParm.h"
#include "Deferred.h"
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <ratio>
#include <set>
#include <sstream>
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "Formatters.h"
#include "InitFlags.h"
#include "InitThread.h"
#include "Log.h"
#include "MainArgs.h"
#include "Memory.h"
#include "NbLogs.h"
#include "NbModule.h"
#include "NbSignals.h"
#include "Restart.h"
#include "RootThread.h"
#include "Singleton.h"
#include "SteadyTime.h"
#include "SystemTime.h"
#include "ThisThread.h"
#include "ThreadRegistry.h"
#include "TraceBuffer.h"

using std::ostream;
using std::setw;
using std::string;

namespace NodeBase
{
//  The following return the set of factions that can be scheduled during
//  various scenarios.
//
static const FactionFlags& NoFactions = FactionFlags();

//------------------------------------------------------------------------------

static const FactionFlags& AllFactions()
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
//
//  Extracts the next symbol from the front of SYMBOLS and returns it after
//  erasing it from SYMBOLS.
//
static string GetSymbol(string& symbols)
{
   auto begin = symbols.find_first_not_of(SPACE);
   if(begin == string::npos) return EMPTY_STR;
   symbols = symbols.substr(begin);
   auto end = symbols.find_first_of(SPACE);
   auto symbol = symbols.substr(0, end);

   if(end != string::npos)
      symbols = symbols.substr(end);
   else
      symbols.clear();

   return symbol;
}

//------------------------------------------------------------------------------

fixed_string InstallationGuide =
   "https://github.com/GregUtas/robust-services-core/blob/master/docs/Installing.md";

static void OutputNodeRunningLog()
{
   Debug::ft("NodeBase.OutputNodeRunningLog");

   auto log = Log::Create(NodeLogGroup, NodeRunning);

   if(log != nullptr)
   {
      if(Element::IsUnnamed())
      {
         *log << CRLF;
         *log << "CONFIGURATION FILE NOT FOUND. See" << CRLF;
         *log << spaces(2) << InstallationGuide << CRLF;
         *log << "for instructions on how to install RSC." << CRLF;
      }

      Log::Submit(log);
   }
}

//------------------------------------------------------------------------------

static const FactionFlags& ShutdownFactions()
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
//  A stream for recording the progress of system initialization.
//
static ostringstreamPtr stream_ = nullptr;

//------------------------------------------------------------------------------
//
//  Returns stream_, creating it if it doesn't exist.
//
static std::ostringstream* Stream()
{
   if(stream_ == nullptr)
   {
      stream_.reset(new std::ostringstream);
      *stream_ << std::boolalpha << std::nouppercase;
   }

   return stream_.get();
}

//==============================================================================
//
//  If initialization is being traced, stops it shortly after the system has
//  booted.
//
class StopInitTracing : public Deferred
{
public:
   //  Stops tracing after 1 second.
   //
   StopInitTracing();

   //  Not subclassed.
   //
   ~StopInitTracing();
private:
   //  Stops tracing.
   //
   void EventHasOccurred(Event event) override;
};

//------------------------------------------------------------------------------

StopInitTracing::StopInitTracing() :
   Deferred(*Singleton<ModuleRegistry>::Instance(), 1, false)
{
   Debug::ft("StopInitTracing.ctor");
}

//------------------------------------------------------------------------------

StopInitTracing::~StopInitTracing()
{
   Debug::ftnt("StopInitTracing.dtor");
}

//------------------------------------------------------------------------------

void StopInitTracing::EventHasOccurred(Event event)
{
   Debug::ft("StopInitTracing.EventHasOccurred");

   Singleton<TraceBuffer>::Instance()->StopTracing();
}

//==============================================================================
//
//  Configuration parameter for enabling optional components.
//
class ModulesCfg : public CfgStrParm
{
public:
   ModulesCfg();
   ~ModulesCfg();
private:
   RestartLevel RestartRequired() const override { return RestartReboot; }
};

//------------------------------------------------------------------------------

ModulesCfg::ModulesCfg() : CfgStrParm("OptionalModules", EMPTY_STR,
   "modules to enable (edit config file and reboot to change)")
{
   Debug::ft("ModulesCfg.ctor");
}

//------------------------------------------------------------------------------

ModulesCfg::~ModulesCfg()
{
   Debug::ftnt("ModulesCfg.dtor");
}

//==============================================================================

ModuleRegistry::ModuleRegistry()
{
   Debug::ft("ModuleRegistry.ctor");

   modules_.Init(Module::MaxId, Module::CellDiff(), MemImmutable);

   //  The creation of this registry means that immutable memory is now
   //  available, so create MainArgs in order to save main()'s arguments
   //  in immutable memory.
   //
   Singleton<MainArgs>::Instance();

   //  Create the configuration parameter for enabling optional components.
   //
   modulesCfg_.reset(new ModulesCfg);
   Singleton<CfgParmRegistry>::Instance()->BindParm(*modulesCfg_);
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

   const auto& symbol = module.Symbol();

   if(!symbol.empty() && (FindModule(symbol) != nullptr))
   {
      string expl = "module symbol \"" + symbol + "\" duplicated";
      Debug::SwLog(ModuleRegistry_BindModule, expl, 0);
   }

   modules_.Insert(module);
}

//------------------------------------------------------------------------------

void ModuleRegistry::CheckForExit()
{
   Debug::ft("ModuleRegistry.CheckForExit");

   switch(Restart::Level_)
   {
   case RestartReboot:
   case RestartExit:
      //
      //  Force the system to exit.  If this occurs during startup, the
      //  system failed to even boot, so exit without trying to reboot.
      //
      if(Restart::Stage_ == StartingUp) Restart::Level_ = RestartExit;
      Singleton<RootThread>::Instance()->Interrupt(InitThread::Restart);
      ThisThread::Pause(TIMEOUT_NEVER);
   }
}

//------------------------------------------------------------------------------

void ModuleRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "Stage  : " << Restart::Stage_ << CRLF;
   stream << prefix << "Level  : " << Restart::Level_ << CRLF;
   stream << prefix << "stream : " << stream_.get() << CRLF;

   stream << prefix << "modules [ModuleId]" << CRLF;
   modules_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name ModuleRegistry_EnableModules = "ModuleRegistry.EnableModules";

void ModuleRegistry::EnableModules() const
{
   Debug::ft(ModuleRegistry_EnableModules);

   if(Restart::GetLevel() != RestartReboot) return;

   //  NbModule is enabled by default, and its Startup function actually
   //  invokes us.  Enable it explicitly for cosmetic purposes, so that
   //  it will be marked enabled in the output of the >modules command.
   //
   Singleton<NbModule>::Instance()->Enable();

   string enabled = modulesCfg_->CurrValue();

   for(auto sym = GetSymbol(enabled); !sym.empty(); sym = GetSymbol(enabled))
   {
      auto mod = FindModule(sym);

      if(mod != nullptr)
      {
         mod->Enable();
      }
      else
      {
         string expl = "module symbol \"" + sym + "\" not found";
         Debug::SwLog(ModuleRegistry_EnableModules, expl, 0);
      }
   }
}

//------------------------------------------------------------------------------

Module* ModuleRegistry::FindModule(const string& symbol) const
{
   Debug::ft("ModuleRegistry.FindModule");

   for(auto m = modules_.First(); m != nullptr; modules_.Next(m))
   {
      if(m->Symbol() == symbol) return m;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

RestartLevel ModuleRegistry::GetLevel()
{
   return Restart::Level_;
}

//------------------------------------------------------------------------------

RestartLevel ModuleRegistry::NextLevel()
{
   Debug::ft("ModuleRegistry.NextLevel");

   switch(Restart::Level_)
   {
   case RestartNone:
      return RestartWarm;
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

void ModuleRegistry::Restart()
{
   Debug::ft("ModuleRegistry.Restart");

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
         {
            if(reentered)
            {
               CheckForExit();
               Restart::Stage_ = ShuttingDown;
               break;
            }

            Startup(Restart::Level_);
            OutputNodeRunningLog();
            Restart::Level_ = RestartNone;
            Restart::Stage_ = Running;
         }
         Thread::EnableFactions(AllFactions());
         return;

      case Running:
         reentered = false;
         if(Restart::Level_ == RestartNone) return;
         Restart::Stage_ = ShuttingDown;
         break;

      case ShuttingDown:
         Thread::EnableFactions(NoFactions);
         if(reentered) Restart::Level_ = NextLevel();
         CheckForExit();
         Shutdown(Restart::Level_);
         reentered = false;
         Restart::Stage_ = StartingUp;
         break;
      }
   }
}

//------------------------------------------------------------------------------

void ModuleRegistry::SetLevel(RestartLevel level)
{
   Debug::ft("ModuleRegistry.SetLevel");

   Restart::Level_ = level;
}

//------------------------------------------------------------------------------

fixed_string ShutdownHeader =
   "MODULE SHUTDOWN                msecs      invoked at";
// |                                  36              52

fixed_string ShutdownTotalStr = "total shutdown time";
fixed_string NotifyingThreadsStr = "Notifying threads...";
fixed_string ExitingThreadsStr = "...threads to exit: ";
fixed_string ExitedThreadsStr = "...threads exited: ";
fixed_string ShutdownStr = "...shut down";

void ModuleRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("ModuleRegistry.Shutdown");

   if(level >= RestartReload)
   {
      Memory::Unprotect(MemProtected);
   }

   msecs_t delay(25);

   //  Schedule a subset of the factions so that pending logs will be output.
   //
   Thread::EnableFactions(ShutdownFactions());
   {
      for(size_t tries = 120, idle = 0; (tries > 0) && (idle <= 8); --tries)
      {
         ThisThread::Pause(delay);
         if(Thread::SwitchContext() != nullptr)
            idle = 0;
         else
            ++idle;
      }
   }
   Thread::EnableFactions(NoFactions);

   auto zeroTime = SystemTime::Now();
   auto zeroPoint = SteadyTime::Now();
   *Stream() << CRLF << "RESTART TYPE: " << level << CRLF;
   *Stream() << CRLF << ShutdownHeader << CRLF;

   //  Notify all threads of the restart.
   //
   *Stream() << NotifyingThreadsStr << setw(52 - strlen(NotifyingThreadsStr));
   *Stream() << to_string(zeroTime, LowAlpha) << CRLF;
   Log::Submit(stream_);

   auto reg = Singleton<ThreadRegistry>::Instance();
   auto exiting = reg->Restarting(level);
   auto target = exiting.size();

   //  Report the number of threads that plan to exit.  Signal the threads
   //  that will exit and schedule threads until the planned number have
   //  exited.  If some fail to exit, RootThread will time out and escalate
   //  the restart.
   //
   *Stream() << ExitingThreadsStr << setw(2) << target;
   *Stream() << setw(36 - (strlen(ExitingThreadsStr) + 2));

   for(auto t = exiting.cbegin(); t != exiting.cend(); ++t)
   {
      (*t)->Raise(SIGCLOSE);
   }

   nsecs_t elapsed = SteadyTime::Now() - zeroPoint;
   *Stream() << elapsed.count() / NS_TO_MS << CRLF;
   Log::Submit(stream_);

   Thread::EnableFactions(AllFactions());
   {
      for(auto prev = exiting.size(); prev > 0; prev = exiting.size())
      {
         Thread::SwitchContext();
         ThisThread::Pause(delay);
         reg->TrimThreads(exiting);

         if(prev == exiting.size())
         {
            //  No thread exited while we were paused.  Resignal the remaining
            //  threads.  This is similar to code in InitThread::HandleTimeout
            //  and Thread::SwitchContext, where a thread occasionally misses
            //  its Proceed() and must be resignalled.
            //
            for(auto t = exiting.cbegin(); t != exiting.cend(); ++t)
            {
               (*t)->Raise(SIGCLOSE);
            }
         }
      }
   }
   Thread::EnableFactions(NoFactions);

   auto actual = target - exiting.size();
   *Stream() << CRLF << ExitedThreadsStr << setw(2) << actual;
   *Stream() << setw(36 - (strlen(ExitedThreadsStr) + 2));
   elapsed = SteadyTime::Now() - zeroPoint;
   *Stream() << elapsed.count() / NS_TO_MS << CRLF;
   Log::Submit(stream_);

   //  Modules must be shut down in reverse order of their initialization.
   //
   for(auto m = modules_.Last(); m != nullptr; modules_.Prev(m))
   {
      if(m->IsEnabled() || (m->Mid() == 1))
      {
         auto time = SystemTime::Now();
         auto point = SteadyTime::Now();
         auto name = strClass(m) + "...";
         *Stream() << name << setw(52 - name.size());
         *Stream() << to_string(time, LowAlpha) << CRLF;
         Log::Submit(stream_);

         m->Shutdown(level);

         *Stream() << ShutdownStr << setw(36 - strlen(ShutdownStr));
         elapsed = SteadyTime::Now() - point;
         *Stream() << elapsed.count() / NS_TO_MS << CRLF;
         Log::Submit(stream_);
      }
   }

   *Stream() << setw(36) << string(5, '-') << CRLF;
   auto width = strlen(ShutdownTotalStr);
   elapsed = SteadyTime::Now() - zeroPoint;
   *Stream() << ShutdownTotalStr;
   *Stream() << setw(36 - width) << elapsed.count() / NS_TO_MS << CRLF;
   Log::Submit(stream_);
}

//------------------------------------------------------------------------------

fixed_string StartupHeader =
   "MODULE INITIALIZATION          msecs      invoked at";
// |                                  36              52

fixed_string StartupTotalStr = "total initialization time";
fixed_string PreModuleStr = "pre-Module.Startup";
fixed_string InitializedStr = "...initialized";

void ModuleRegistry::Startup(RestartLevel level)
{
   Debug::ft("ModuleRegistry.Startup");

   auto zeroTime =
      (level >= RestartReboot ? SystemTime::TimeZero() : SystemTime::Now());
   auto zeroPoint =
      (level >= RestartReboot ? SteadyTime::TimeZero() : SteadyTime::Now());
   *Stream() << CRLF << StartupHeader << CRLF;

   if(level >= RestartReboot)
   {
      nsecs_t elapsed = SteadyTime::Now() - zeroPoint;
      *Stream() << PreModuleStr;
      *Stream() << setw(36 - strlen(PreModuleStr))
         << elapsed.count() / NS_TO_MS;
      *Stream() << setw(16) << to_string(zeroTime, LowAlpha) << CRLF;
   }

   for(auto m = modules_.First(); m != nullptr; modules_.Next(m))
   {
      if(m->IsEnabled() || (m->Mid() == 1))
      {
         auto time = SystemTime::Now();
         auto point = SteadyTime::Now();
         auto name = strClass(m) + "...";
         *Stream() << name << setw(52 - name.size());
         *Stream() << to_string(time, LowAlpha) << CRLF;
         Log::Submit(stream_);

         m->Startup(level);

         nsecs_t elapsed = SteadyTime::Now() - point;
         *Stream() << InitializedStr << setw(36 - strlen(InitializedStr));
         *Stream() << elapsed.count() / NS_TO_MS << CRLF;
         Log::Submit(stream_);
      }
   }

   //  Write-protect memory segments that are read-only while in service.
   //
   Memory::Protect(MemImmutable);
   Memory::Protect(MemProtected);

   //  If initialization is being traced, create a work item to stop
   //  tracing momentarily.
   //
   if((Restart::Level_ == RestartReboot) && InitFlags::TraceInit())
   {
      new StopInitTracing;
   }

   *Stream() << setw(36) << string(5, '-') << CRLF;
   auto width = strlen(StartupTotalStr);
   nsecs_t elapsed = SteadyTime::Now() - zeroPoint;
   *Stream() << StartupTotalStr;
   *Stream() << setw(36 - width) << elapsed.count() / NS_TO_MS << CRLF;
   Log::Submit(stream_);
}

//------------------------------------------------------------------------------

fixed_string ModuleHeader = " Id  Symbol  Enabled  Module";
//                          |  3..     6..      7..<class>

size_t ModuleRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << ModuleHeader << CRLF;

   for(auto m = modules_.First(); m != nullptr; modules_.Next(m))
   {
      stream << setw(3) << m->Mid();
      stream << spaces(2) << setw(6) << m->Symbol();
      stream << spaces(2) << setw(7) << m->IsEnabled();
      stream << spaces(2) << strClass(m) << CRLF;
   }

   return modules_.Size();
}

//------------------------------------------------------------------------------

void ModuleRegistry::UnbindModule(Module& module)
{
   Debug::ftnt("ModuleRegistry.UnbindModule");

   modules_.Erase(module);
}
}
