//==============================================================================
//
//  InitThread.cpp
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
#include "InitThread.h"
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Log.h"
#include "ModuleRegistry.h"
#include "Registry.h"
#include "RootThread.h"
#include "Singleton.h"
#include "ThreadAdmin.h"
#include "ThreadRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const Flags InitThread::RestartMask = Flags(1 << InitThread::Restart);

//------------------------------------------------------------------------------

fn_name InitThread_ctor = "InitThread.ctor";

InitThread::InitThread() : Thread(SystemFaction),
   state_(Initializing),
   timeout_(false),
   errval_(0)
{
   Debug::ft(InitThread_ctor);
}

//------------------------------------------------------------------------------

fn_name InitThread_dtor = "InitThread.dtor";

InitThread::~InitThread()
{
   Debug::ft(InitThread_dtor);
}

//------------------------------------------------------------------------------

const char* InitThread::AbbrName() const
{
   return "init";
}

//------------------------------------------------------------------------------

fn_name InitThread_CalculateDelay = "InitThread.CalculateDelay";

msecs_t InitThread::CalculateDelay() const
{
   Debug::ft(InitThread_CalculateDelay);

   //  If breakpoint debugging is enabled, sleep forever.  Otherwise, wake
   //  up at the earliest of the following:
   //  o the time before which RootThread must be interrupted to prevent a
   //    scheduling timeout;
   //  o the time before which the locked thread must yield;
   //  o the RTC timeout, if no thread is currently locked.
   //
   if(ThreadAdmin::BreakEnabled())
   {
      return TIMEOUT_NEVER;
   }

   ticks_t ticks = 0;
   auto thr = LockedThread();

   if((thr != nullptr) && !timeout_)
      ticks = thr->TicksLeft();
   else
      ticks = Clock::MsecsToTicks(ThreadAdmin::RtcTimeoutMsecs());

   auto delay = ThreadAdmin::SchedTimeoutMsecs() >> 1;

   if(ticks < Clock::MsecsToTicks(delay))
   {
      delay = Clock::TicksToMsecs(ticks);
   }

   //  If our timeout interval was rounded off to zero, sleep briefly.
   //
   if(delay <= 0) delay = 1;
   return delay;
}

//------------------------------------------------------------------------------

fn_name InitThread_CauseRestart = "InitThread.CauseRestart";

void InitThread::CauseRestart()
{
   Debug::ft(InitThread_CauseRestart);

   //  We get here if
   //  o our state gets corrupted (unlikely)
   //  o Delay fails (unlikely)
   //  o a critical thread could not be recreated
   //
   auto log = Log::Create("DEATH OF CRITICAL THREAD");

   if(log != nullptr)
   {
      *log << "errval=" << strHex(errval_) << CRLF;
      Log::Spool(log);
   }

   Singleton< RootThread >::Instance()->Interrupt(RestartMask);
   state_ = Initializing;
   errval_ = 0;
   Pause(100);
}

//------------------------------------------------------------------------------

fn_name InitThread_Destroy = "InitThread.Destroy";

void InitThread::Destroy()
{
   Debug::ft(InitThread_Destroy);

   Singleton< InitThread >::Destroy();
}

//------------------------------------------------------------------------------

void InitThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "state   : " << state_ << CRLF;
   stream << prefix << "timeout : " << timeout_ << CRLF;
   stream << prefix << "errval  : " << errval_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name InitThread_Enter = "InitThread.Enter";

void InitThread::Enter()
{
   Debug::ft(InitThread_Enter);

   msecs_t delay;
   DelayRc drc;

   //  Threads are unpreemptable when entered, but we run preemptably when
   //  the system is in service.  If we didn't, then once our sleep interval
   //  expired, we'd have to acquire the RTC lock before we could run again.
   //  This would make it impossible to enforce the timeout on the RTC lock,
   //  which is one of our responsibilities.
   //
   MakePreemptable();

   while(true)
   {
      switch(state_)
      {
      case Initializing:
         InitializeSystem();
         break;

      case Running:
         delay = CalculateDelay();
         timeout_ = false;
         drc = Pause(delay);

         switch(drc)
         {
         case DelayCompleted:
            HandleTimeout();
            break;

         case DelayInterrupted:
            HandleInterrupt();
            break;

         default:
            state_ = Restarting;
            errval_ = pack2(Tid(), NativeThreadId());
         }
         break;

      case Restarting:
      default:
         CauseRestart();
      }
   }
}

//------------------------------------------------------------------------------

fn_name InitThread_HandleInterrupt = "InitThread.HandleInterrupt";

void InitThread::HandleInterrupt()
{
   Debug::ft(InitThread_HandleInterrupt);

   //  See if we were interrupted to initiate a restart.  In this case, our
   //  InitiateRestart function has already interrupted RootThread to inform
   //  it of the restart.  RootThread is now running a watchdog timer on the
   //  restart itself.  Update our state so that we will initiate the restart
   //  after sleeping briefly, allowing time for the restart log to be output.
   //
   if(TestFlag(InitThread::Restart))
   {
      ResetFlag(InitThread::Restart);
      state_ = Initializing;
      Pause(100);
      return;
   }

   //  We also get interrupted
   //  o to recreate critical threads that were forced to exit.  Initiate a
   //    restart if re-creation fails.
   //  o when breakpoint debugging is disabled after being enabled.  After
   //    failing to find a thread that needs to be recrated, we simply end
   //    up looping around and recalculating our next timeout interval.
   //  In both of these cases, interrupt RootThread so that its watchdog
   //  timer won't expire.
   //
   Singleton< RootThread >::Instance()->Interrupt();

   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

   for(auto t = threads.First(); t != nullptr; threads.Next(t))
   {
      if(t->HasExited())
      {
         state_ = Restarting;
         errval_ = pack2(t->Tid(), t->NativeThreadId());

         if(t->Recreate())
         {
            state_ = Running;
            errval_ = 0;
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name InitThread_HandleTimeout = "InitThread.HandleTimeout";

void InitThread::HandleTimeout()
{
   Debug::ft(InitThread_HandleTimeout);

   //  Interrupt RootThread so that its watchdog timer won't expire.
   //  If the locked thread has run too long, signal it.
   //
   Singleton< RootThread >::Instance()->Interrupt();
   timeout_ = false;

   auto thr = LockedThread();

   if((thr != nullptr) && (thr->TicksLeft() == 0))
   {
      thr->RtcTimeout();
      timeout_ = true;
   }
}

//------------------------------------------------------------------------------

fn_name InitThread_InitializeSystem = "InitThread.InitializeSystem";

void InitThread::InitializeSystem()
{
   Debug::ft(InitThread_InitializeSystem);

   //  Make this thread unpreeeptable to prevent other threads from running
   //  before the system is initialized.  When the system is initialized,
   //  notify the root thread so that it will stop the watchdog timer that
   //  runs during initialization.
   //
   FunctionGuard guard(FunctionGuard::MakeUnpreemptable);
   Singleton< ModuleRegistry >::Instance()->Restart();
   state_ = Running;
   Singleton< RootThread >::Instance()->Interrupt();
}

//------------------------------------------------------------------------------

fn_name InitThread_InitiateRestart = "InitThread.InitiateRestart";

void InitThread::InitiateRestart(reinit_t reason, debug32_t errval)
{
   Debug::ft(InitThread_InitiateRestart);

   //  Save the restart reason and error value.  Tell RootThread that a restart
   //  is occurring so that it can act as a watchdog on its completion, and then
   //  inform our thread.
   //
   Singleton< ModuleRegistry >::Instance()->SetReason(reason, errval);
   Singleton< RootThread >::Instance()->Interrupt(RestartMask);
   Interrupt(RestartMask);
}

//------------------------------------------------------------------------------

void InitThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}
