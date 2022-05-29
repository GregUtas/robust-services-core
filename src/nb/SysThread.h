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
//  Consideration was given to using <thread> and wrapping a std::thread,
//  but this was abandoned for a few reasons:
//  o It would only eliminate functions RunningThreadId, Create, and Delete.
//  o It would make it impossible to specify a thread's stack size.  Most
//    systems allocate large thread stacks, which are seldom appropriate.
//  o thread::id is opaque.
//  o thread::native_handle_type is platform-specific.
//
class SysThread : public Permanent
{
   friend std::unique_ptr<SysThread>::deleter_type;
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
   //  Creates a native thread for CLIENT.  PRIO is the priority at which
   //  it will run, and SIZE is its stack size).
   //
   SysThread(Thread* client, Priority prio, size_t size);

   //  Releases resources.
   //
   ~SysThread();

   //  For reporting errors from platform-specific functions.
   //
   static bool ReportError(fn_name function, fixed_string expl, int error);

   //  Platform-specific.  Configures the executable before creating the
   //  first thread.
   //
   static void ConfigureProcess();

   //  Platform-specific.  Invoked by the constructor to create a thread.
   //  CLIENT and SIZE were passed to the constructor.  Updates nid_ to
   //  the thread's native identifier and nthread_ to its native handle.
   //  The thread must be created detached.  Returns false on failure.
   //
   bool Create(const Thread* client, size_t size);

   //  Platform-specific.  Sets or changes the thread's priority.  Invoked
   //  immediately after Create and also if a thread switches from running
   //  unpreemptably to preemptably or vice versa.
   //
   bool SetPriority(Priority prio);

   //  The signature of a signal handler.  SIG is the signal that occurred.
   //
   typedef void (*sighandler_t)(signal_t sig);

   //  Platform-specific.  Registers HANDLER against SIG.  Invoked when
   //  the thread is entered or reentered after trap recovery.
   //
   static void RegisterForSignal(signal_t sig, sighandler_t handler);

   //  Platform-specific.  Invoked when the thread is entered or reentered
   //  after trap recovery.  Returns a non-zero value if the thread should
   //  immediately exit.
   //
   signal_t Start();

   //  Platform-specific.  Invoked by the destructor so that resources can
   //  be released.
   //
   void Delete();

   //  Returns the thread's native identifier.
   //
   SysThreadId Nid() const { return nid_; }

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

   //  Invoked to wait on GATE until TIMEOUT.  If TIMEOUT is TIMEOUT_NEVER,
   //  the thread will only resume after GATE is signalled.
   //
   DelayRc Suspend(Gate& gate, const msecs_t& timeout);

   //  The thread's status flags.
   //
   enum StatusFlag
   {
      StackOverflowed,  // caused SIGSTACK1
      IsExiting,        // is about to return
      StatusFlag_N      // number of flags
   };

   typedef std::bitset<StatusFlag_N> StatusFlags;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.  Must be implemented as platform-specific.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  The thread's native identifier.
   //
   SysThreadId nid_;

   //  The thread's native handle.
   //
   SysThread_t nthread_;

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
