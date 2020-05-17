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
#include <set>
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "Daemon.h"
#include "DaemonRegistry.h"
#include "Debug.h"
#include "Duration.h"
#include "Formatters.h"
#include "Log.h"
#include "ModuleRegistry.h"
#include "NbLogs.h"
#include "NbTracer.h"
#include "Registry.h"
#include "Restart.h"
#include "RootThread.h"
#include "Singleton.h"
#include "ThreadAdmin.h"
#include "ToolTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const Flags InitThread::RestartMask = Flags(1 << Restart);
const Flags InitThread::RecreateMask = Flags(1 << Recreate);
const Flags InitThread::ScheduleMask = Flags(1 << Schedule);

//------------------------------------------------------------------------------

fn_name InitThread_ctor = "InitThread.ctor";

InitThread::InitThread() : Thread(SystemFaction),
   errval_(0),
   state_(Initializing),
   timeout_(false)
{
   Debug::ft(InitThread_ctor);

   SetInitialized();
}

//------------------------------------------------------------------------------

fn_name InitThread_dtor = "InitThread.dtor";

InitThread::~InitThread()
{
   Debug::ftnt(InitThread_dtor);
}

//------------------------------------------------------------------------------

c_string InitThread::AbbrName() const
{
   return "init";
}

//------------------------------------------------------------------------------

fn_name InitThread_CalculateDelay = "InitThread.CalculateDelay";

Duration InitThread::CalculateDelay() const
{
   Debug::ft(InitThread_CalculateDelay);

   //  Wake up at the earliest of the following:
   //  o the time before which RootThread must be interrupted to
   //    prevent a scheduling timeout;
   //  o the time before which the unpreemptable thread must yield;
   //  o the RTC timeout, if no unpreemptable thread is running (or
   //    if it has already been signalled for running too long).
   //
   Duration timeout;
   auto thr = LockedThread();

   if((thr != nullptr) && !timeout_)
      timeout = thr->TimeLeft();
   else
      timeout = ThreadAdmin::RtcTimeout();

   auto delay = ThreadAdmin::SchedTimeout() >> 1;

   if(timeout < delay)
   {
      delay = timeout;
   }

   //  If our timeout interval was rounded off to zero, sleep briefly.
   //
   if(delay <= TIMEOUT_IMMED) delay = ONE_mSEC;
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
   //
   auto log = Log::Create(NodeLogGroup, NodeRestart);

   if(log != nullptr)
   {
      auto prefix = Log::Tab + spaces(2);
      *log << Log::Tab << "in " << to_str() << CRLF;
      *log << prefix << "level  : " << RestartWarm << CRLF;
      *log << prefix << "reason : " << ThreadPauseFailed << CRLF;
      *log << prefix << "errval : " << strHex(debug64_t(0)) << CRLF;
      Log::Submit(log);
   }

   auto reg = Singleton< ModuleRegistry >::Instance();
   reg->SetLevel(RestartWarm);
   Singleton< RootThread >::Instance()->Interrupt(RestartMask);
   state_ = Initializing;
   Pause(Duration(100, mSECS));
}

//------------------------------------------------------------------------------

fn_name InitThread_ContextSwitch = "InitThread.ContextSwitch";

void InitThread::ContextSwitch()
{
   Debug::ft(InitThread_ContextSwitch);

   //  The current execution flow for context switching is
   //    Thread.Suspend
   //    ..Thread.Schedule
   //    ..InitThread.Interrupt [X]
   //    thread blocks [X]
   //    InitThread.HandleInterrupt [X]
   //    ..InitThread.ContextSwitch [X]
   //    ....InitThread.Reset(Schedule) [X]
   //    ....Thread.SwitchContext
   //    ......Thread.Select
   //    ......Thread.Proceed
   //  So why not take InitThread out of the picture by removing the things
   //  marked with an X, so that the original thread blocks after the call to
   //  Proceed?  Well, doing so resulted in traps when running POTS traffic.
   //  Specifically, UDP and invoker threads ran simultaneously.  And because
   //  both allocate Messages, race conditions eventually caused a corruption
   //  of the Message ObjectPool's free queue.  Instead of debugging this, the
   //  current design was reinstated.  It hasn't caused this kind of problem
   //  because it serializes all scheduling through InitThread.  When threads
   //  initiate context switches themselves, the problem is that more than one
   //  thread can run at a time (when preemptable threads are included).  Even
   //  lowering a thread's priority is no guarantee, at least on Windows, that
   //  it will not run.  Adding the necessary mutexes to fix whatever critical
   //  sections need protecting could easily add more overhead than continuing
   //  to go through InitThread.
   //
   Thread::SwitchContext();
   Reset(Schedule);
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

   Duration delay;
   DelayRc drc;

   //  When a thread is entered, it is unpreemptable.  However, we must run
   //  preemptably so that we don't wait for unpreemptable threads to yield.
   //  Our high priority ensures that we will run whenever we want.
   //
   MakePreemptable();

   //  When we are reentered after a trap, check for unfinished work before
   //  sleeping.
   //
   if(state_ == Running) HandleInterrupt();

   while(true)
   {
      Debug::ft(InitThread_Enter);

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
   //  restart itself.  Update our state so that we will initiate the restart.
   //
   if(Test(Restart))
   {
      ResetFlags();
      state_ = Initializing;
      return;
   }

   //  We also get interrupted
   //  o to recreate critical threads that were forced to exit;
   //  o to initiate a context switch;
   //  o when breakpoint debugging is disabled after being enabled, in which
   //    case no flag is set and we end up looping around and recalculating
   //    our next timeout interval.
   //  In each of these cases, interrupt RootThread so that its watchdog
   //  timer won't expire.
   //
   Singleton< RootThread >::Instance()->Interrupt();

   if(Test(Recreate))
   {
      RecreateThreads();
   }

   if(Test(Schedule))
   {
      ContextSwitch();
   }
}

//------------------------------------------------------------------------------

fn_name InitThread_HandleTimeout = "InitThread.HandleTimeout";

void InitThread::HandleTimeout()
{
   Debug::ft(InitThread_HandleTimeout);

   //  Interrupt RootThread so that its watchdog timer won't expire.
   //
   Singleton< RootThread >::Instance()->Interrupt();
   timeout_ = false;

   //  If there is no locked thread, schedule one.  If the locked thread
   //  is still waiting to proceed, signal it.  Both of these are unusual
   //  situations that occur because of race conditions.
   //
   auto thr = LockedThread();

   if(thr == nullptr)
   {
      ContextSwitch();
      if(ActiveThread() != nullptr) ThreadAdmin::Incr(ThreadAdmin::Delays);
      return;
   }
   else
   {
      if(thr->IsScheduled())
      {
         thr->Proceed();
         ThreadAdmin::Incr(ThreadAdmin::Resignals);
         return;
      }
   }

   //  If the locked thread has run too long, signal it unless breakpoint
   //  debugging is enabled.
   //
   if((thr->TimeLeft() == ZERO_SECS) && !ThreadAdmin::BreakEnabled())
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

   //  Once the system is initialized, notify RootThread so that it
   //  will stop the watchdog timer that runs during initialization.
   //
   Singleton< ModuleRegistry >::Instance()->Restart();
   state_ = Running;
   Singleton< RootThread >::Instance()->Interrupt();

   //  Now that the restart is over, disable  tracing of RootThread
   //  and this thread, which usually cause unwanted noise in traces.
   //  Schedule the first thread before returning to our thread loop
   //  to sleep.
   //
   auto nbt = Singleton< NbTracer >::Instance();
   nbt->SelectFaction(WatchdogFaction, TraceExcluded);
   nbt->SelectFaction(SystemFaction, TraceExcluded);
   ContextSwitch();
}

//------------------------------------------------------------------------------

fn_name InitThread_InitiateRestart = "InitThread.InitiateRestart";

void InitThread::InitiateRestart(RestartLevel level)
{
   Debug::ft(InitThread_InitiateRestart);

   //  Set the restart's level.  Tell RootThread that a restart is
   //  occurring so that it can act as a watchdog on its completion and
   //  then wake up our thread.
   //
   Singleton< ModuleRegistry >::Instance()->SetLevel(level);
   Singleton< RootThread >::Instance()->Interrupt(RestartMask);
   Interrupt(RestartMask);
}

//------------------------------------------------------------------------------

void InitThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name InitThread_RecreateThreads = "InitThread.RecreateThreads";

void InitThread::RecreateThreads()
{
   Debug::ft(InitThread_RecreateThreads);

   //  Invoke daemons with missing threads.
   //
   auto& daemons = Singleton< DaemonRegistry >::Instance()->Daemons();

   for(auto d = daemons.First(); d != nullptr; daemons.Next(d))
   {
      if(d->Threads().size() < d->TargetSize())
      {
         d->CreateThreads();
      }
   }

   //  This is reset after the above so that if a trap occurs, we will
   //  again try to recreate threads when reentered.
   //
   Reset(Recreate);
}
}
