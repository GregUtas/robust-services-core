//==============================================================================
//
//  RootThread.cpp
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
#include "RootThread.h"
#include <chrono>
#include <cstdlib>
#include <ratio>
#include <sstream>
#include <string>
#include "Debug.h"
#include "Duration.h"
#include "Gate.h"
#include "InitFlags.h"
#include "InitThread.h"
#include "Log.h"
#include "LogBufferRegistry.h"
#include "ModuleRegistry.h"
#include "NbAppIds.h"
#include "NbLogs.h"
#include "NbSignals.h"
#include "Singleton.h"
#include "SteadyTime.h"
#include "SysStackTrace.h"
#include "SysThread.h"
#include "ThreadAdmin.h"
#include "ThreadRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  After the thread that was created to run main() creates RootThread, it
//  waits forever on this gate.  To initiate a reboot or exit, RootThread
//  sets ExitCode and signals this gate, which unblocks the original thread
//  and allows it to exit with ExitCode.  RscLauncher restarts the executable
//  if ExitCode is not EXIT_SUCCESS.
//
static main_t ExitCode = EXIT_SUCCESS;

static Gate& ExitGate()
{
   static Gate gate;

   return gate;
}

//------------------------------------------------------------------------------

RootThread::RootThread() : Thread(WatchdogFaction),
   state_(Initializing)
{
   Debug::ft("RootThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

RootThread::~RootThread()
{
   Debug::ftnt("RootThread.dtor");
}

//------------------------------------------------------------------------------

c_string RootThread::AbbrName() const
{
   return "root";
}

//------------------------------------------------------------------------------

void RootThread::Destroy()
{
   Debug::ft("RootThread.Destroy");

   Singleton<RootThread>::Destroy();
}

//------------------------------------------------------------------------------

void RootThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "state : " << state_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name RootThread_Enter = "RootThread.Enter";

void RootThread::Enter()
{
   Debug::ft(RootThread_Enter);

   auto timeout = TIMEOUT_IMMED;
   auto lastLog = SteadyTime::TimeZero();

   //  When a thread is entered, it is unpreemptable.  However, we must run
   //  preemptably so that we don't wait for other unpreemptable threads to
   //  yield.  Our high priority ensures that we will run whenever we want.
   //
   MakePreemptable();

   while(true)
   {
      switch(state_)
      {
      case Initializing:
         //
         //  Create InitThread and then sleep to give it time to initialize the
         //  system.  When it's finished, it wakes us up.  If our timer expires,
         //  initialization failed.
         //
         Singleton<InitThread>::Instance();

         //  The following suspends RootThread during breakpoint debugging,
         //  where it would otherwise appear with annoying regularity.
         //
         if(InitFlags::SuspendRoot() || Debug::SwFlagOn(DisableRootThread))
         {
            systhrd_->Wait();
         }

         timeout = ThreadAdmin::InitTimeout();

         if(Pause(timeout) == DelayInterrupted)
         {
            if(Test(InitThread::Restart))
               RestartInitiated();
            else
               state_ = Running;
         }
         else
         {
            //  Initialization timed out, so generate a log.
            //
            auto log = Log::Create(NodeLogGroup, NodeInitTimeout);

            if(log != nullptr)
            {
               *log << " timeout=" << to_string(timeout);
               Log::Submit(log);
            }

            //  Escalate the restart unless doing so is disabled or breakpoint
            //  debugging is enabled.
            //
            if(ThreadAdmin::ReinitOnSchedTimeout() && !InitFlags::AllowBreak())
            {
               //  If InitThread exists, shut it down before looping around to
               //  recreate it.
               //
               auto initThr = Singleton<InitThread>::Extant();

               if(initThr != nullptr)
               {
                  initThr->Raise(SIGCLOSE);
                  Pause(msecs_t(100));
               }
            }
         }
         break;

      case Running:
      default:
         if(state_ != Running)
         {
            Debug::SwLog(RootThread_Enter, "state corruption", state_);
            state_ = Running;
         }

         //  The following suspends RootThread during breakpoint debugging,
         //  when it would otherwise appear with annoying regularity.
         //
         if(Debug::SwFlagOn(DisableRootThread))
         {
            systhrd_->Wait();
         }

         //  The system initialized.  Sleep for the scheduling timeout.
         //
         timeout = ThreadAdmin::SchedTimeout();

         if(Pause(timeout) == DelayInterrupted)
         {
            //  This is usually a heartbeat from InitThread.  But it also
            //  occurs when InitThread is initiating a restart.
            //
            if(Test(InitThread::Restart))
            {
               RestartInitiated();
            }

            break;
         }

         //  A scheduling timeout occurred.  Ignore this if breakpoint
         //  debugging is enabled, else generate a log.  Limit the rate
         //  at which these logs are generated in case BreakEnabled was
         //  not set during debugging.
         //
         if(ThreadAdmin::BreakEnabled()) break;

         nsecs_t elapsed = SteadyTime::Now() - lastLog;

         if((elapsed.count() / NS_TO_SECS) >= 3)
         {
            auto log = Log::Create(NodeLogGroup, NodeSchedTimeout);

            if(log != nullptr)
            {
               *log << " timeout=" << to_string(timeout);
               Log::Submit(log);
            }

            lastLog = SteadyTime::Now();
         }

         //  Initiate a restart unless doing so is disabled or breakpoint
         //  debugging is enabled.
         //
         if(ThreadAdmin::ReinitOnSchedTimeout() && !InitFlags::AllowBreak())
         {
            //  If InitThread exists, tell it to initiate a restart.  If it
            //  doesn't exist, loop around and create it.
            //
            auto initThr = Singleton<InitThread>::Extant();

            if(initThr != nullptr)
            {
               initThr->Interrupt(InitThread::Restart);
            }

            state_ = Initializing;
         }
      }
   }
}

//------------------------------------------------------------------------------

main_t RootThread::Main()
{
   Debug::ft("RootThread.Main");

   //  Load symbol information.
   //
   SysStackTrace::Startup(RestartReboot);

   //  Create the POSIX signals.  They are needed now so that
   //  RootThread can register for signals when it is created.
   //
   CreatePosixSignals();

   //  Create the log buffer, which is used to log the progress
   //  of initialization.
   //
   Singleton<LogBufferRegistry>::Instance();

   //  Set up our process.
   //
   SysThread::ConfigureProcess();

   //  Create ThreadRegistry.  Thread::Start uses its GetState
   //  function to see when a Thread has been fully constructed
   //  and can safely proceed.
   //
   Singleton<ThreadRegistry>::Instance();

   //  Create the root thread and wait for it to exit.
   //
   Singleton<RootThread>::Instance();
   ExitGate().WaitFor(TIMEOUT_NEVER);

   //  If we get here, RootThread wants the system to exit and
   //  possibly get rebooted.
   //
   Debug::Exiting();
   exit(ExitCode);
}

//------------------------------------------------------------------------------

void RootThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void RootThread::RestartInitiated()
{
   Debug::ft("RootThread.RestartInitiated");

   //  If a reboot or exit was requested, unblock the thread that
   //  ran main() and created us so that it can exit.
   //
   auto level = ModuleRegistry::GetLevel();

   if((level == RestartReboot) || (level == RestartExit))
   {
      ExitCode = (level == RestartExit ? EXIT_SUCCESS : EXIT_FAILURE);
      ExitGate().Notify();
      Pause(TIMEOUT_NEVER);
   }

   //  If the system won't exit, just clear the interrupt and let
   //  InitThread continue with the restart.
   //
   Reset(InitThread::Restart);
   state_ = Initializing;
}
}
