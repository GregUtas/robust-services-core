//==============================================================================
//
//  SysThread.linux.cpp
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
#ifdef OS_LINUX

#include "SysThread.h"
#include <csignal>
#include <cstdint>
#include <errno.h>
#include <pthread.h>
#include <sys/resource.h>
#include "Debug.h"
#include "NbSignals.h"
#include "Thread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Mapping of external to internal priorities.
//
const int PriorityMap[SysThread::Priority_N] =
{
   0,  // LowPriority
   1,  // DefaultPriority
   2,  // SystemPriority
   3   // WatchdogPriority
};

//  The Thread signal handler.  We register a different one that invokes
//  this one after it performs some preliminary work.
//
static SysThread::sighandler_t SignalHandler = nullptr;

//------------------------------------------------------------------------------

static void* EnterThread(void* arg)
{
   Debug::ft("NodeBase.EnterThread");

   //  Our argument is a pointer to a Thread.
   //
   auto thread = static_cast<Thread*>(arg);
   uintptr_t rc = thread->Start();
   return (void*) rc;
}

//------------------------------------------------------------------------------

static void SigActionHandler(int signal, siginfo_t* info, void* unused)
{
   //  If the address that caused a SIGSEGV can be read but not written,
   //  change the signal to SIGWRITE.
   //
   if(signal == SIGSEGV)
   {
      if((info->si_code == SEGV_ACCERR) || (info->si_code == SEGV_PKUERR))
      {
         signal = SIGWRITE;
      }
   }

   SignalHandler(signal);
}

//------------------------------------------------------------------------------

fn_name SysThread_ConfigureProcess = "SysThread.ConfigureProcess";

void SysThread::ConfigureProcess()
{
   Debug::ft(SysThread_ConfigureProcess);

   //  If changing our process priority is not allowed, setpriority returns
   //  EACCES.
   //
   if(SetPriorityAllowed())
   {
      auto err = setpriority(PRIO_PROCESS, 0, -1);
      if(err != 0)
      {
         ReportError(SysThread_ConfigureProcess, "setpriority", err);
      }
   }
}

//------------------------------------------------------------------------------

fn_name SysThread_Create = "SysThread.Create";

bool SysThread::Create(const Thread* client, size_t size)
{
   Debug::ft(SysThread_Create);

   //  Initialize the structure that defines a new thread's attributes.
   //  o They start to run detached, since join() is nonsense.
   //  o They usually need a much smaller stack size than in other systems.
   //  o They will be scheduled round-robin, using the real-time scheduler.
   //
   pthread_attr_t attrs;

   auto err = pthread_attr_init(&attrs);
   if(err != 0)
   {
      return ReportError(SysThread_Create, "attr_init", err);
   }

   err = pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
   if(err != 0)
   {
      return ReportError(SysThread_Create, "setdetachstate", err);
   }

   err = pthread_attr_setstacksize(&attrs, size);
   if(err != 0)
   {
      return ReportError(SysThread_Create, "setstacksize", err);
   }

   //  If changing a thread's priority is not allowed, pthread_create
   //  returns EINVAL if the scheduler policy is set to round-robin.
   //
   if(SetPriorityAllowed())
   {
      err = pthread_attr_setinheritsched(&attrs, PTHREAD_EXPLICIT_SCHED);
      if(err != 0)
      {
         return ReportError(SysThread_Create, "setinheritsched", err);
      }

      err = pthread_attr_setschedpolicy(&attrs, SCHED_RR);
      if(err != 0)
      {
         return ReportError(SysThread_Create, "setschedpolicy", err);
      }
   }

   pthread_t thread;

   err = pthread_create(&thread, &attrs, EnterThread, (void*) client);
   if(err != 0)
   {
      return ReportError(SysThread_Create, "create", err);
   }

   //  Unlike Windows, this POSIX/Linux implementation does not need to
   //  distinguish a thread's handle from its identifier.  POSIX only
   //  has pthread_t, which will do double duty.
   //
   nthread_ = thread;
   nid_ = thread;
   return true;
}

//------------------------------------------------------------------------------

void SysThread::Delete()
{
   Debug::ftnt("SysThread.Delete");
}

//------------------------------------------------------------------------------

void SysThread::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SysThread_RegisterForSignal = "SysThread.RegisterForSignal";

bool SysThread::RegisterForSignal(signal_t sig, sighandler_t handler)
{
   //  Save HANDLER so that SigActionHandler can invoke it.
   //
   SignalHandler = handler;

   //  Register SigActionHandler against the signal by setting SA_SIGINFO.
   //  This provides extra information about a signal so that the handler
   //  can map SIGSEGV to SIGWRITE when appropriate.  Also set SA_RESETHAND
   //  and SA_NODEFER to prevent a received signal from being masked when
   //  the handler is invoked.  The masking persists even after the handler
   //  is reinstalled.  Setting these two flags results in behavior that is
   //  similar to that for the basic signal() function.
   //
   struct sigaction action;
   action.sa_sigaction = SigActionHandler;
   sigemptyset(&action.sa_mask);
   action.sa_flags = (SA_SIGINFO | SA_RESETHAND | SA_NODEFER);

   auto err = sigaction(sig, &action, nullptr);
   if(err != 0)
   {
      return ReportError(SysThread_RegisterForSignal, "sigaction", errno);
   }

   return true;
}

//------------------------------------------------------------------------------

SysThreadId SysThread::RunningThreadId() NO_FT
{
   return pthread_self();
}

//------------------------------------------------------------------------------

fn_name SysThread_SetPriority = "SysThread.SetPriority";

bool SysThread::SetPriority(Priority prio)
{
   Debug::ft(SysThread_SetPriority);

   //  If changing thread priority is not allowed, pthread_setschedprio
   //  returns EPERM.
   //
   if(!SetPriorityAllowed()) return true;

   if(priority_ == prio) return true;

   auto err = pthread_setschedprio((pthread_t) nthread_, PriorityMap[prio]);
   if(err != 0)
   {
      ReportError(SysThread_SetPriority, "setschedparam", err);
      return false;
   }

   priority_ = prio;
   return true;
}

//------------------------------------------------------------------------------

bool SysThread::SetPriorityAllowed()
{
   return false;
}

//------------------------------------------------------------------------------

signal_t SysThread::Start()
{
   Debug::ft("SysThread.Start");

   //  This is invoked when recovering from a trap.  If any Linux-specific
   //  actions are required, they should be recorded in status_, but there
   //  are currently none.
   //
   return 0;
}
}
#endif
