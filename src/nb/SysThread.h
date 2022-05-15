//==============================================================================
//
//  SysThread.h
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
#ifndef SYSTHREAD_H_INCLUDED
#define SYSTHREAD_H_INCLUDED

#include "Permanent.h"
#include <bitset>
#include <cstddef>
#include <memory>
#include "Duration.h"
#include "Gate.h"
#include "SysDecls.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Thread;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: native thread wrapper.
//
class SysThread : public Permanent
{
   friend std::unique_ptr< SysThread >::deleter_type;
   friend class Debug;
   friend class Thread;
   friend class ThreadRegistry;
   friend class RootThread;
   friend class InitThread;
public:
   //  Deleted to prohibit copying.
   //
   SysThread(const SysThread& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   SysThread& operator=(const SysThread& that) = delete;

   //  Returns the native identifier of the running thread.
   //
   static SysThreadId RunningThreadId();

   //  Thread priorities.
   //
   enum Priority
   {
      LowPriority,       // preemptable threads
      DefaultPriority,   // unpreemptable threads
      SystemPriority,    // InitThread
      WatchdogPriority,  // RootThread
      Priority_N         // number of priorities
   };
private:
   //  The signature of a signal handler.
   //
   typedef void (*sighandler_t)(signal_t sig);

   //  The thread's status flags.
   //
   enum StatusFlag
   {
      SetPriorityFailed,  // failed to set priority
      StackOverflowed,    // caused SIGSTACK1
      IsExiting,          // is about to return
      StatusFlag_N        // number of flags
   };

   typedef std::bitset< StatusFlag_N > StatusFlags;

   //  Creates a native thread for CLIENT.  PRIO is the priority at
   //  which it will run, and SIZE is its stack size).
   //
   SysThread(Thread* client, Priority prio, size_t size);

   //  Releases resources.
   //
   ~SysThread();

   //  For reporting errors from platform-specific functions.
   //
   static bool ReportError(fn_name function, fixed_string expl, int error);

   //  Used by the constructor to create an actual native thread.  CLIENT
   //  and SIZE were passed to the constructor.  Updates NID to the new
   //  thread's native identifier and NTHREAD to its handle.  Returns
   //  false on failure.
   //
   static bool Create(const Thread* client,
      size_t size, SysThreadId& nid, SysThread_t& nthread);

   //  Deletes a native thread and nullifies its handle.
   //
   static void Delete(SysThread_t& thread);

   //  Returns the thread's native identifier.
   //
   SysThreadId Nid() const { return nid_; }

   //  Configures the executable before creating the first thread.
   //
   static void ConfigureProcess();

   //  Performs environment-specific actions upon entering the thread.
   //  Returns a non-zero value if the thread should immediately exit.
   //
   signal_t Start();

   //  Sleeps for TIMEOUT (TIMEOUT_IMMED = yield, TIMEOUT_NEVER = infinite).
   //
   DelayRc Delay(const msecs_t& timeout);

   //  Signals the thread.  If the thread is delaying, it awakens.  If it
   //  is not delaying, it only yields (sleeps for zero seconds, allowing
   //  other threads to run) the next time it delays.
   //
   bool Interrupt();

   //  Invoked by the thread when it is ready to run unpreemptably.
   //
   DelayRc Wait();

   //  Invoked when the thread can resume running unpreemptably.
   //
   bool Proceed();

   //  Invoked to wait on SENTRY until TIMEOUT.  If TIMEOUT is TIMEOUT_NEVER,
   //  the thread will only resume after SENTRY is signalled.
   //
   DelayRc Suspend(Gate& gate, const msecs_t& timeout);

   //  Sets or changes the thread's priority.
   //
   bool SetPriority(Priority prio);

   //  Registers HANDLER against SIG.
   //
   static void RegisterForSignal(signal_t sig, sighandler_t handler);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Reference to the native thread.
   //
   SysThread_t nthread_;

   //  Native identifier for this thread.
   //
   SysThreadId nid_;

   //  The thread's current status.
   //
   StatusFlags status_;

   //  The thread's current priority.
   //
   Priority priority_;

   //  Used to implement Delay and Interrupt.
   //
   Gate alarm_;

   //  Used to implement Wait and Proceed.
   //
   Gate sched_;

   //  The signal that caused the thread to be deleted.
   //
   const signal_t signal_;
};
}
#endif
