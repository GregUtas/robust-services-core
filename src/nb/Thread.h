//==============================================================================
//
//  Thread.h
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
#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include "Permanent.h"
#include <atomic>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <memory>
#include <new>
#include <string>
#include "Duration.h"
#include "NbTypes.h"
#include "Q1Way.h"
#include "SysDecls.h"
#include "SysThread.h"
#include "SysTypes.h"
#include "ToolTypes.h"

namespace NodeBase
{
   class Daemon;
   class MsgBuffer;
   class Mutex;
   class ThreadPriv;
   class ThreadStats;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for threads.  All threads should derive from this, as it
//  provides functions that support RSC's programming model.
//
class Thread : public Permanent
{
   friend class Debug;
   friend class Exception;
   friend class FunctionGuard;
   friend class InitThread;
   friend class ModuleRegistry;
   friend class RootThread;
   friend class SchedCommand;
   friend class Mutex;
   friend class ThreadRegistry;
public:
   //  Deleted to prohibit copying.
   //
   Thread(const Thread& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Thread& operator=(const Thread& that) = delete;

   //> Highest valid thread identifier.
   //
   static const ThreadId MaxId;

   //  Schedules the current thread out for TIME.  If TIME is TIMEOUT_IMMED,
   //  the thread is scheduled out but can run again immediately, though other
   //  threads may get to run first.  If TIME is TIMEOUT_NEVER, the thread
   //  sleeps forever, although it can be interrupted.  Returns the reason why
   //  the pause ended.
   //
   static DelayRc Pause(msecs_t time = TIMEOUT_IMMED);

   //  Returns how long the running thread has run as a percentage (0 to 100)
   //  of the run-to-completion timeout.  Returns 0 for a preemptable thread.
   //
   static word RtcPercentUsed();

   //  Invokes Pause(TIMEOUT_IMMED) if RtcPercentUsed() returns LIMIT or more.
   //
   static void PauseOver(word limit);

   //  Must be called immediately before using a blocking operation.  WHY is
   //  the reason for blocking.  FUNC identifies the function that wants to
   //  call the blocking operation.  Returns false if the operation must not
   //  be performed.
   //
   static bool EnterBlockingOperation(BlockingReason why, fn_name_arg func);

   //  Must be called immediately after a blocking operation completes.
   //  FUNC identifies the function that has resumed execution.
   //
   static void ExitBlockingOperation(fn_name_arg func);

   //  Returns the reason, if any, that the thread is blocked.
   //
   BlockingReason GetBlockingReason() const;

   //  Queues MSG for processing by the thread.  May be overridden, but the
   //  base class version must be invoked.
   //
   virtual bool EnqMsg(MsgBuffer& msg);

   //  Flags for Interrupt.  Thread-specific flags must start with TS_Flag.
   //
   static const FlagId Signalled = 0;         // POSIX signal sent to thread
   static const FlagId RestartIsOver = 1;     // system is back in service
   static const FlagId MessageAvailable = 2;  // message queued on thread
   static const FlagId ResumeExecution = 3;   // wake from hibernation
   static const FlagId WorkAvailable = 4;     // handle work item
   static const FlagId TS_Flag = 8;           // first thread-specific flag

   //  Awakens a thread if it has paused.  If it has not paused, it will be
   //  immediately reawakened if it pauses.  If it is blocked for some other
   //  reason, it remains blocked.  REASON is used to set the flags that the
   //  thread can access with Vector() when it resumes execution.  How these
   //  flags are interpreted is thread-specific, but they allow a thread to
   //  determine why it was woken up.
   //
   bool Interrupt(FlagId reason);

   //  Returns the running thread's interrupt vector.
   //
   static std::atomic_uint32_t* Vector();

   //  Returns a flag in the running thread's interrupt vector.
   //
   static bool TestFlag(FlagId fid);

   //  Clears a flag in the running thread's interrupt vector.
   //
   static void ResetFlag(FlagId fid);

   //  Clears the flags in the running thread's interrupt vector.
   //
   static void ResetFlags();

   //  Returns the thread that is currently running.  Throws an
   //  exception if the thread is not found unless the nothrow
   //  version is used.
   //
   static Thread* RunningThread();
   static Thread* RunningThread(const std::nothrow_t&);

   //  Returns the thread's identifier within ThreadRegistry.
   //
   ThreadId Tid() const { return tid_; }

   //  Returns the native thread's identifier.
   //
   SysThreadId NativeThreadId() const;

   //  Returns the thread's scheduler faction.
   //
   Faction GetFaction() const { return faction_; }

   //  Changes the thread to FACTION.  Returns true on success.
   //
   bool ChangeFaction(Faction faction);

   //  Used to explicitly include or exclude the thread from a trace.
   //
   void SetStatus(TraceStatus status);

   //  Returns the thread's trace status.  To determine if the thread
   //  should actually be traced, use CalcStatus or FindStatus (below),
   //  which take additional criteria into account.
   //
   TraceStatus GetStatus() const;

   //  Calculates whether the thread should be traced.  DYNAMIC is true when
   //  the thread is running and deciding whether to trace work in progress;
   //  it is false when generating trace output, at which time trace records
   //  can be filtered to generate a subset report.
   //
   virtual TraceStatus CalcStatus(bool dynamic) const;

   //  Starts tracing unless it is already on.  The thread must be unpreemptable
   //  and must enable the desired trace tools and select the items to be traced
   //  before invoking this function.  If OPTS includes TraceAutostop ('a'),
   //  tracing stops on the next context switch.  See TraceBuffer::StartTracing
   //  for additional options.
   //
   static TraceRc StartTracing(const std::string& opts);

   //  Stops a trace that was started by StartTracing.
   //
   static void StopTracing();

   //  Causes the thread to trap on SIGNAL.  If the thread is not critical,
   //  or if SIGNAL is final, the thread exits.  In other cases, the thread
   //  is reentered or recreated, resuming execution as if it was being
   //  entered for the first time.
   //
   void Raise(signal_t sig);

   //  Invoked by SignalHandler (and, on Windows, SE_Handler) to handle SIG.
   //  CODE is for debugging.  Returns false if the running thread is unknown
   //  and SIG is not a break signal (that is, does not have PosixSignal::Break
   //  attribute set).
   //
   static bool HandleSignal(signal_t sig, uint32_t code);

   //  Clears status flags that prevent stack overflows.  This function is
   //  invoked during exception and signal handling so that logging and
   //  function tracing do not remain permanently disabled.
   //
   static void ResetDebugFlags();

   //  Returns true if the thread has been scheduled to run.
   //
   bool IsScheduled() const;

   //  How long the thread has run since it was scheduled in.
   //
   msecs_t CurrTimeRunning() const;

   //  Returns the percentage of idle time during the most recent statistics
   //  short interval.
   //
   static double PercentIdle();

   //  Returns a string containing the thread's class name and identifiers.
   //
   std::string to_str() const;

   //  Contains most of the entry code that is common to all threads.
   //  Implements the safety net that can recover from all exceptions.
   //  This is public for expediency and should be treated as private.
   //
   main_t Start();

   //  Displays statistics.  May be overridden to include thread-specific
   //  statistics, but the base class version must be invoked.
   //
   virtual void DisplayStats(std::ostream& stream, const Flags& options) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.  This is only invoked on threads that did
   //  not exit when the restart began.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.  This is only invoked on threads that did
   //  not exit and survived the restart.
   //
   void Startup(RestartLevel level) override;
protected:
   //  Creates a thread that runs in the specified scheduler FACTION and
   //  is managed by DAEMON.  Protected because this class is virtual.
   //
   Thread(Faction faction, Daemon* daemon = nullptr);

   //  Protected to restrict deletion, which must always be done through
   //  Destroy, and exclusively within this class.
   //
   virtual ~Thread();

   //  Invoked as the last thing done in a leaf class constructor.  When
   //  a Thread is created, its native thread may actually start to run
   //  before the Thread has been fully constructed, so the thread is held
   //  up until it has invoked this function.
   //
   void SetInitialized();

   //  Dequeues the next message.  If no message is available, pauses for
   //  TIMEOUT to wait for a message.  May be overridden, but the base class
   //  version must be invoked.  Protected to restrict usage to subclasses.
   //
   virtual MsgBuffer* DeqMsg(const msecs_t& timeout);

   //  Returns the amount of time that the thread receives when it begins
   //  to run unpreemptably.  The default should only be overridden for
   //  compelling reasons.
   //
   virtual msecs_t InitialTime() const;

   //  Returns the signal, if any, that is currently being handled.
   //
   signal_t GetSignal() const;

   //  Starts the next short interval for thread statistics.
   //
   static void StartShortInterval();

   //  Dereferences a bad pointer during testing.
   //
   static void CauseTrap();

   //  Overridden to claim queued messages.  May be overridden, but this
   //  version must be invoked.
   //
   void ClaimBlocks() override;
private:
   //  Returns an abbreviated version of the thread's name, which must be
   //  at most 7 characters long.
   //
   virtual c_string AbbrName() const = 0;

   //  The entry point to the thread.  If the thread needs an argument,
   //  it must be provided using a data member defined by the thread's
   //  subclass and set by its constructor.  Before this function is
   //  invoked, every thread is made unpreemptable (see MakeUnpreemptable).
   //  If this function returns, the thread is deleted and exits with a
   //  code of 0.
   //
   virtual void Enter() = 0;

   //  Invoked by EnterBlockingOperation to determine whether the thread is
   //  allowed to block.  The default version returns true.  This function
   //  exists to avoid making EnterBlockingOperation virtual.  That way, if
   //  a thread's vptr gets corrupted, EnterBlockingOperation will still be
   //  invoked.  WHY and FUNC are the same as for EnterBlockingOperation.
   //
   virtual bool BlockingAllowed
      (BlockingReason why, fn_name_arg func) { return true; }

   //  Invoked by ExitBlockingOperation to inform the thread that it is
   //  running again.  The default version does nothing.  This function's
   //  purpose is to effectively make ExitBlockingOperation virtual.  It is
   //  implemented as a separate function so that, if a thread's vptr gets
   //  corrupted, ExitBlockingOperation will still be invoked.  FUNC is the
   //  same as for ExitBlockingOperation.
   //
   virtual void ScheduledIn(fn_name_arg func) { }

   //  Invoked when a blocked thread will receive a signal (from Raise) that
   //  forces it to exit.  If possible, the thread should unblock itself so
   //  that it can exit before completing its blocking operation.  The default
   //  implementation does nothing and can be overridden as required.
   //
   virtual void Unblock();

   //  Invoked at the outset of a restart so that the thread can decide whether
   //  to exit or sleep until the restart is over.  The default version returns
   //  true.  Because it is desirable to delete and recreate all threads during
   //  a restart, this should only be overridden for compelling reasons.
   //
   virtual bool ExitOnRestart(RestartLevel level) const;

   //  Invoked when a thread is reentered after a signal or exception.
   //  This allows the thread to clean up work in progress and
   //  o return false to simply exit the thread and delete it, after
   //    which its Daemon (if any) can recreate it, or
   //  o return true to reinvoke the thread's Enter function.
   //  The default version returns true and may be overridden as required.
   //  However, a thread is forced to exit if
   //  o it receives a signal that forces it to exit, or
   //  o it traps too often, or
   //  o it traps during trap recovery, except in this function.
   //
   virtual bool Recover();

   //  Invoked to destroy a thread.  The default version simply invokes
   //  delete but may be overridden to properly delete a Singleton.
   //
   virtual void Destroy();

   //  Sets the thread's internal identifier when it is added to ThreadRegistry.
   //
   void SetTid(ThreadId tid) { tid_ = tid; }

   //  Used by both versions of RunningThread.
   //
   static Thread* FindRunningThread();

   //  Causes the current thread to run unpreemptably (run to completion).
   //  When a thread is entered, it is made unpreemptable before its Enter
   //  function is invoked.  Must be invoked via FunctionGuard.
   //
   static void MakeUnpreemptable();

   //  Causes the current thread to run preemptably.  Must be invoked
   //  via FunctionGuard.
   //
   static void MakePreemptable();

   //  Write-enables MemProtected.  Must be invoked via FunctionGuard.
   //
   static void MemUnprotect();

   //  Write-disables MemProtected.  Must be invoked via FunctionGuard.
   //
   static void MemProtect();

   //  Write-enables MemImmutable.  Must be invoked via FunctionGuard.
   //
   static void ImmUnprotect();

   //  Write-disables MemImmutable.  Must be invoked via FunctionGuard.
   //
   static void ImmProtect();

   //  Invoked when a thread is entered.  A new thread can start to run
   //  before its Thread object has been fully constructed, so it must
   //  wait until its leaf class has invoked SetInitialized.  Returns
   //  SIGNIL when the thread can proceed.  Any other signal indicates
   //  that an error has occurred, in which case the thread exits.
   //
   static signal_t WaitUntilConstructed();

   //  Returns a flag in the thread's interrupt vector.  See also TestFlag.
   //
   bool Test(FlagId fid) const;

   //  Clears a flag in the thread's interrupt vector.  See also ResetFlag.
   //
   void Reset(FlagId fid);

   //  Invoked by a thread when it is ready to run.
   //
   void Ready();

   //  Preempts a running thread.
   //
   void Preempt();

   //  Schedules a thread out when it yields or blocks.
   //
   void Suspend();

   //  Schedules the next thread when this one is suspending.
   //
   void Schedule();

   //  Schedules another thread after a thread yields or blocks, or
   //  after a preemptable thread has run for its allotted time.
   //  Returns the scheduled thread.  Returns nullptr if no thread
   //  is running or ready.
   //
   static Thread* SwitchContext();

   //  Invoked to signal the thread to run.
   //
   void Proceed();

   //  Invoked by a thread when it resumes execution.  FUNC is from
   //  ExitBlockingOperation.
   //
   void Resume(fn_name_arg func);

   //  Kills the thread.  Returns nullptr when trying to kill the thread.
   //  If the thread cannot be killed, returns a C string explaining why.
   //
   c_string Kill();

   //  Returns true if the thread is unpreemptable.
   //
   bool IsLocked() const;

   //  Returns true if the thread can be traced.
   //
   bool IsTraceable() const;

   //  Returns the active thread.
   //
   static Thread* ActiveThread();

   //  Returns the active thread if it is running unpreemptably.
   //
   static Thread* LockedThread();

   //  Returns true if the thread can be scheduled to run.
   //
   bool CanBeScheduled() const;

   //  Returns the thread's daemon.
   //
   Daemon* GetDaemon() const { return daemon_; }

   //  Notes that the thread is trying to acquire MUTEX, which is nullptr
   //  if the mutex has been acquired.
   //
   void UpdateMutex(Mutex* mutex);

   //  Returns the mutex that the thread is trying to acquire.
   //
   Mutex* BlockingMutex() const;

   //  Notes that the thread has acquired (if true) or released a mutex.
   //
   void UpdateMutexCount(bool acquired);

   //  Returns the thread's mutex count.
   //
   uint8_t MutexCount() const;

   //  Returns true if the thread can generate a software log.  Returns false
   //  if a nested software log is already being generated, which is likely to
   //  eventually cause a stack overflow if further nesting is not prevented.
   //
   static bool EnterSwLog();

   //  Invoked when a software log has been generated (all = false) or a trap
   //  has occurred (all = true).
   //
   static void ExitSwLog(bool all);

   //  Invoked to exit a thread when it is safe to do so.  Does nothing if it
   //  currently unsafe to exit the thread.  OFFSET indicates where the exit
   //  was forced.
   //
   void ExitIfSafe(debug64_t offset);

   //  The signal handler that is registered for all threads.  It turns a
   //  signal into a C++ exception that is caught in Thread::Start.
   //
   static void SignalHandler(signal_t sig);

   //  Registers SignalHandler against all native signals.
   //
   static void RegisterForSignals();

   //  What to do after handling a signal or exception.
   //
   enum TrapAction
   {
      Continue,  // check if the thread should be recovered
      Release,   // return by invoking Exit
      Return     // return immediately
   };

   //  Handles a trap (signal or exception).  EX is nullptr if the exception
   //  is not a subclass of Exception.  E is nullptr if the exception is not
   //  derived from exception.  SIG is SIGNIL unless EX is a SignalException.
   //  STACK contains the thread's stack, if available.
   //
   TrapAction TrapHandler(const Exception* ex,
      const std::exception* e, signal_t sig, const std::ostringstream* stack);

   //  The same arguments as TrapHandler.  Generates the trap log.  Returns
   //  true if the thread has exceeded the trap limit.
   //
   bool LogTrap(const Exception* ex,
      const std::exception* e, signal_t sig, const std::ostringstream* stack);

   //  Invoked at the outset of a restart.  It invokes ExitOnRestart (above)
   //  to see if the thread should exit now or sleep until the restart ends.
   //  Returns true if the thread plans to exit, and false if it will sleep.
   //
   bool Restarting(RestartLevel level) const;

   //  Returns the amount of time still available to the locked thread.
   //  Returns ZERO_SECS if the thread has used up all of its time.
   //
   msecs_t TimeLeft() const;

   //  Gives the running thread more TIME to run unpreemptably.
   //
   static void ExtendTime(const msecs_t& time);

   //  Invoked when a thread did not yield before the run-to-completion
   //  timer expired.
   //
   void RtcTimeout();

   //  Coordinates activities associated with invoking FUNC, namely
   //  o capturing FUNC in a trace tool
   //  o throwing an exception when the running thread is to be trapped
   //  o checking stack usage by the running thread
   //
   static void FunctionInvoked(fn_name_arg func);
   static void FunctionInvoked(fn_name_arg func, const std::nothrow_t&);

   //  Determines whether the running thread (THR) should be traced.  If THR
   //  is nullptr, it is updated to the running thread if this function must
   //  actually find the running thread to make its determination.
   //
   static bool TraceRunningThread(Thread*& thr);
   static bool TraceRunningThread(Thread*& thr, const std::nothrow_t&);

   //  Records the thread event associated with RID if the running thread (THR)
   //  is being traced.  If THR is nullptr, the running thread will be found.
   //  FUNC is the function in which the event occurred, and INFO (optional)
   //  provides debugging information.
   //
   static void Trace(Thread* thr, fn_name_arg func,
      TraceRecordId rid, int32_t info = 0);

   //  Invoked to set or clear trap_.
   //
   void SetTrap(bool on);

   //  Invoked when a trap request is pending.
   //
   void TrapCheck();

   //  Invoked to check stack usage.
   //
   void StackCheck();

   //  Sets the signal to be raised or that is being handled.
   //
   void SetSignal(signal_t sig);

   //  Returns true if SIGNAL should be logged.
   //
   bool LogSignal(signal_t sig) const;

   //  Invoked when returning from Thread::Start.  SIG indicates why
   //  the thread is exiting.
   //
   main_t Exit(signal_t sig);

   //  Frees the thread's resources.  ORPHANED is set if the thread is
   //  not running but had its Thread object deleted.
   //
   void ReleaseResources(bool orphaned);

   //  Invoked when the thread will be exited without deleting its
   //  Thread object.  Returns SIGNAL as the thread's exit code.
   //
   main_t AbnormalExit(signal_t signal);

   //  Performs cleanup actions when deleting or exiting a thread.
   //  If a Thread object is deleted when the thread is not running,
   //  this gets invoked twice.  The first time, ORPHANED is set to
   //  indicate that the thread is not currently running.  The second
   //  time, DELETED is set to indicate that the Thread object has
   //  already been deleted.
   //
   void Purge(bool orphaned, bool deleted);

   //  Used during initializations and restarts to enable/disable the
   //  scheduling of specific factions.
   //
   static void EnableFactions(const FactionFlags& enabled);

   //  Displays a summary of the thread's statistics in STREAM.  TIME0 is
   //  the time that has elapsed during the current statistics measurement
   //  period.  TIME1 was the duration of the most recent short interval
   //  for thread statistics.
   //
   void DisplaySummary(std::ostream& stream,
      const nsecs_t& time0, const nsecs_t& time1) const;

   //  Displays a summary of all threads' statistics in STREAM.
   //
   static void DisplaySummaries(std::ostream& stream);

   //  Starts (stops) logging context switches if ON is true (false).
   //
   static TraceRc LogContextSwitches(bool on);

   //  Logs information about a context switch.
   //
   void LogContextSwitch() const;

   //  Displays context switches in STREAM.
   //
   static void DisplayContextSwitches(std::ostream& stream);

   //  The wrapper for the native thread.
   //
   std::unique_ptr<SysThread> systhrd_;

   //  The thread's manager, if any.
   //
   Daemon* const daemon_;

   //  The thread's identifier in ThreadRegistry.
   //
   ThreadId tid_;

   //  The thread's scheduler faction.
   //
   Faction faction_;

   //  Set if the thread is in the process of being deleted.
   //
   bool deleting_;

   //  The thread's message queue.
   //
   Q1Way<MsgBuffer> msgq_;

   //  Per-thread data that is not required in the header.
   //
   std::unique_ptr<ThreadPriv> priv_;

   //  Per-thread statistics.
   //
   std::unique_ptr<ThreadStats> stats_;
};

//------------------------------------------------------------------------------
//
//  Options for the StartTracing function.
//
constexpr char TraceAutostop = 'a';
}
#endif
