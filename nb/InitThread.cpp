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
#include "Log.h"
#include "ModuleRegistry.h"
#include "NbLogs.h"
#include "Registry.h"
#include "RootThread.h"
#include "Singleton.h"
#include "SysThread.h"
#include "ThreadAdmin.h"
#include "ThreadRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const Flags InitThread::RestartMask = Flags(1 << InitThread::Restart);
const Flags InitThread::RecreateMask = Flags(1 << InitThread::Recreate);
const Flags InitThread::ScheduleMask = Flags(1 << InitThread::Schedule);

//------------------------------------------------------------------------------

fn_name InitThread_ctor = "InitThread.ctor";

InitThread::InitThread() : Thread(SystemFaction),
   state_(Initializing),
   start_(1),
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

c_string InitThread::AbbrName() const
{
   return "init";
}

//------------------------------------------------------------------------------

fn_name InitThread_CalculateDelay = "InitThread.CalculateDelay";

msecs_t InitThread::CalculateDelay() const
{
   Debug::ft(InitThread_CalculateDelay);

   //  Wake up at the earliest of the following:
   //  o the time before which RootThread must be interrupted to
   //    prevent a scheduling timeout;
   //  o the time before which the unpreemptable thread must yield;
   //  o the RTC timeout, if no unpreemptable thread is running (or
   //    if it has already been signalled for running too long).
   //
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
   auto log = Log::Create(ThreadLogGroup, ThreadCriticalDeath);

   if(log != nullptr)
   {
      *log << Log::Tab << "errval=" << strHex(errval_);
      Log::Submit(log);
   }

   Singleton< RootThread >::Instance()->Interrupt(RestartMask);
   state_ = Initializing;
   errval_ = 0;
   Pause(100);
}

//------------------------------------------------------------------------------

fn_name InitThread_ContextSwitch = "InitThread.ContextSwitch";

void InitThread::ContextSwitch()
{
   Debug::ft(InitThread_ContextSwitch);

   ResetFlag(Schedule);

   auto locked = LockedThread();
   if(locked != nullptr) return;

   locked = SelectThread();

   if(locked != nullptr)
   {
      LockedThread_ = locked;
      locked->systhrd_->Proceed();
   }
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
   stream << prefix << "start   : " << start_ << CRLF;
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

   //  When a thread is entered, it is unpreemptable.  However, we must run
   //  preemptably so that we don't wait for other unpreemptable threads to
   //  yield.  Our high priority ensures that we will run whenever we want.
   //
   MakePreemptable();

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
   //  restart itself.  Update our state so that we will initiate the restart
   //  after sleeping briefly, allowing time for the restart log to be output.
   //
   if(TestFlag(InitThread::Restart))
   {
      ResetFlags();
      state_ = Initializing;
      Pause(100);
      return;
   }

   //  We also get interrupted
   //  o to recreate critical threads that were forced to exit;
   //  o to schedule the next unpreemptable thread;
   //  o when breakpoint debugging is disabled after being enabled, in which
   //    case no flag is set and we end up looping around and recalculating
   //    our next timeout interval.
   //  In each of these cases, interrupt RootThread so that its watchdog
   //  timer won't expire.
   //
   Singleton< RootThread >::Instance()->Interrupt();

   if(TestFlag(InitThread::Recreate))
   {
      RecreateThreads();
   }

   if(TestFlag(InitThread::Schedule))
   {
      ContextSwitch();
   }
}

//------------------------------------------------------------------------------

fn_name InitThread_HandleTimeout = "InitThread.HandleTimeout";

void InitThread::HandleTimeout()
{
   Debug::ft(InitThread_HandleTimeout);

   //  Interrupt RootThread so that its watchdog timer won't expire.  If the
   //  locked thread has run too long, signal it unless breakpoint debugging
   //  is enabled.
   //
   Singleton< RootThread >::Instance()->Interrupt();
   timeout_ = false;

   auto locked = LockedThread();

   if(locked == nullptr)
   {
      ContextSwitch();
      return;
   }

   if((locked->TicksLeft() == 0) && !ThreadAdmin::BreakEnabled())
   {
      locked->RtcTimeout();
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
}

//------------------------------------------------------------------------------

fn_name InitThread_InitiateRestart = "InitThread.InitiateRestart";

void InitThread::InitiateRestart(reinit_t reason, debug32_t errval)
{
   Debug::ft(InitThread_InitiateRestart);

   //  Save the restart reason and error value.  Tell RootThread that
   //  a restart is occurring so that it can act as a watchdog on its
   //  completion, and then inform our thread.
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

//------------------------------------------------------------------------------

fn_name InitThread_Ready = "InitThread.Ready";

void InitThread::Ready(const Thread* thread)
{
   Debug::ft(InitThread_Ready);

   //  This thread is ready to run unpreemptably.  If there is
   //  currently no locked thread, wake InitThread to schedule
   //  this thread in.  The thread then waits to be signalled.
   //
   if(thread->faction_ >= SystemFaction) return;
   if(LockedThread() == nullptr) Interrupt(ScheduleMask);
   thread->systhrd_->Wait();
}

//------------------------------------------------------------------------------

fn_name InitThread_RecreateThreads = "InitThread.RecreateThreads";

void InitThread::RecreateThreads()
{
   Debug::ft(InitThread_RecreateThreads);

   ResetFlag(Recreate);

   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

   for(auto t = threads.First(); t != nullptr; threads.Next(t))
   {
      if(t->HasExited())
      {
         //  Recreate this thread.  If this traps or fails, state_
         //  and errval_ will cause us to initiate a restart.
         //
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

fn_name InitThread_SelectThread = "InitThread.SelectThread";

Thread* InitThread::SelectThread()
{
   Debug::ft(InitThread_SelectThread);

   //  Cycle through all threads, beginning with the one identified by
   //  start_, to find the next one that is ready to run unpreemptably.
   //
   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();
   auto first = threads.First(start_);
   Thread* next = nullptr;

   for(auto t = first; t != nullptr; threads.Next(t))
   {
      if(t->IsReadyAndUnpreemptable())
      {
         next = t;
         break;
      }
   }

   if(next == nullptr)
   {
      for(auto t = threads.First(); t != first; threads.Next(t))
      {
         if(t->IsReadyAndUnpreemptable())
         {
            next = t;
            break;
         }
      }
   }

   //  If a thread was found, start the next search with the thread
   //  that follows it.
   //
   if(next != nullptr)
   {
      auto t = threads.Next(*next);
      start_ = (t != nullptr ? t->Tid() : 1);
   }

   return next;
}

//------------------------------------------------------------------------------

fn_name InitThread_Yielding = "InitThread.Yielding";

void InitThread::Yielding(const Thread* thread)
{
   Debug::ft(InitThread_Yielding);

   //  Generate a log if the thread running unpreemptably was not the
   //  one that yielded.
   //
   auto locked = LockedThread();

   if((locked != thread) && (locked != nullptr))
   {
      Debug::SwLog(InitThread_Yielding, locked->Tid(), thread->Tid());
      return;
   }

   //  No thread is now running unpreemptably.  If an application thread
   //  just yielded, wake InitThread to schedule the next thread.
   //
   LockedThread_ = nullptr;
   if(thread->faction_ >= SystemFaction) return;
   Interrupt(ScheduleMask);
}
}
