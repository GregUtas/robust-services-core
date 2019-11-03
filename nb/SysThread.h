//==============================================================================
//
//  SysThread.h
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
#ifndef SYSTHREAD_H_INCLUDED
#define SYSTHREAD_H_INCLUDED

#include "Permanent.h"
#include <bitset>
#include <cstddef>
#include <memory>
#include "Clock.h"
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
   friend class RootThread;
   friend class InitThread;
   friend class Orphans;
public:
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
   //  The signature of the Thread entry function.
   //
   typedef main_t (*ThreadEntry)(void* self);

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

   //  Creates a native thread for CLIENT.  ENTRY is its entry function,
   //  PRIO is the priority at which it will run, and SIZE is its stack
   //  size (a size of 0 uses the default size).
   //
   SysThread(const Thread* client,
      const ThreadEntry entry, Priority prio, size_t size);

   //  Wraps an existing native thread.  Used to create RootThread.
   //
   SysThread();

   //  Releases resources.
   //
   ~SysThread();

   //  Deleted to prohibit copying.
   //
   SysThread(const SysThread& that) = delete;
   SysThread& operator=(const SysThread& that) = delete;

   //  Used by the constructor to create an actual native thread.  ENTRY,
   //  CLIENT, and stackSize were passed to the constructor.  Updates NID
   //  to the new thread's native identifier.  Returns the thread's native
   //  handle.
   //
   static SysThread_t Create(const ThreadEntry entry,
      const Thread* client, size_t stackSize, SysThreadId& nid);

   //  Used by the constructor to wrap the thread that is running main().
   //  Returns the thread's native handle after possibly performing some
   //  platform-specific work.
   //
   static SysThread_t Wrap();

   //  Deletes a native thread and nullifies its handle.
   //
   static void Delete(SysThread_t& thread);

   //  Creates a sentry.  A thread waits on a sentry, and other threads
   //  signal it to wake the thread up.
   //
   static SysSentry_t CreateSentry();

   //  Deletes a sentry and nullifies its handle.
   //
   static void DeleteSentry(SysSentry_t& sentry);

   //  Returns the thread's native identifier.
   //
   SysThreadId Nid() const { return nid_; }

   //  Performs environment-specific actions upon entering the thread.
   //  Returns a non-zero value if the thread should immediately exit.
   //
   signal_t Start();

   //  Sleeps for MSECS (0 = yield, -1 = infinite).  The outcomes are
   //  o Error: probably an obscure but serious bug
   //  o Interrupted: was awoken before the requested duration elapsed
   //  o Completed: slept for the requested duration
   //
   DelayRc Delay(msecs_t msecs);

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

   //  Invoked to wait on SENTRY for MSECS.
   //
   DelayRc Suspend(SysSentry_t& sentry, msecs_t msecs);

   //  Invoked to signal SENTRY.
   //
   bool Resume(SysSentry_t& sentry);

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

   //  A reference to a native object that is waited on to implement Delay
   //  and Interrupt.
   //
   SysSentry_t event_;

   //  A reference to a native object that is waited on to implement Wait
   //  and Proceed.
   //
   SysSentry_t guard_;

   //  The thread's current priority.
   //
   Priority priority_;

   //  The signal that caused the thread to be deleted.
   //
   const signal_t signal_;
};
}
#endif
