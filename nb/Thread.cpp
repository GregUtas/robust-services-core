//==============================================================================
//
//  Thread.cpp
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
#include "Thread.h"
#include "Dynamic.h"
#include "FunctionTrace.h"
#include "Permanent.h"
#include <bitset>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <ios>
#include <sstream>
#include "Algorithms.h"
#include "Array.h"
#include "CliThread.h"
#include "CoutThread.h"
#include "Debug.h"
#include "Element.h"
#include "ElementException.h"
#include "Formatters.h"
#include "InitThread.h"
#include "LeakyBucketCounter.h"
#include "Log.h"
#include "MsgBuffer.h"
#include "MutexGuard.h"
#include "NbLogs.h"
#include "NbPools.h"
#include "NbSignals.h"
#include "NbTracer.h"
#include "PosixSignal.h"
#include "PosixSignalRegistry.h"
#include "Registry.h"
#include "Restart.h"
#include "SignalException.h"
#include "Singleton.h"
#include "Statistics.h"
#include "StatisticsRegistry.h"
#include "SysMutex.h"
#include "SysThreadStack.h"
#include "ThreadAdmin.h"
#include "ThreadRegistry.h"
#include "Tool.h"
#include "TraceBuffer.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Records invocations of Pause.
//
class ThreadTrace : public FunctionTrace
{
public:
   static const Id PauseEnter = 1;  // entering Pause
   static const Id PauseExit  = 2;  // returning from Pause

   //  Creates a trace record for the event identified by RID, which
   //  occurred in function FUNC.  INFO is any debugging information.
   //
   static void CaptureEvent(fn_name_arg func, Id rid, word info = 0);

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, bool diff) override;
private:
   //  Private to restrict creation to CaptureEvent.
   //
   ThreadTrace(fn_name_arg func, fn_depth depth, Id rid, word info);

   //  Additional debug information.
   //
   const word info_;
};

//------------------------------------------------------------------------------

ThreadTrace::ThreadTrace(fn_name_arg func, fn_depth depth, Id rid, word info) :
   FunctionTrace(func, depth, sizeof(ThreadTrace)),
   info_(info)
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

fn_name Thread_EnterThread = "Thread.EnterThread";
fn_name Thread_ExitBlockingOperation = "Thread.ExitBlockingOperation";
fn_name ThreadTrace_CaptureEvent = "ThreadTrace.CaptureEvent";

void ThreadTrace::CaptureEvent(fn_name_arg func, Id rid, word info)
{
   //  Do nothing if only invocation counts are being obtained.
   //
   if(GetScope() == CountsOnly) return;

   //  The possible traces are
   //
   //  (1) Pause                     (2) Pause
   //        Trace(PauseEnter)             Trace(PauseExit)
   //          CaptureEvent                  CaptureEvent
   //
   //  Adjust FuncDepth accordingly.
   //
   auto depth = SysThreadStack::FuncDepth();

   switch(rid)
   {
   case PauseExit:
   case PauseEnter:
      new ThreadTrace(func, depth - 2, rid, info);
      break;
   default:
      Debug::SwLog(ThreadTrace_CaptureEvent, "unexpected event", rid);
   }
}

//------------------------------------------------------------------------------

bool ThreadTrace::Display(ostream& stream, bool diff)
{
   if(!FunctionTrace::Display(stream, diff)) return false;

   switch(rid_)
   {
   case PauseEnter:
      stream << " (msecs=" << info_ << ')';
      break;
   case PauseExit:
      stream << " (";

      switch(info_)
      {
      case DelayError:
         stream << "error";
         break;
      case DelayInterrupted:
         stream << "interrupted";
         break;
      case DelayCompleted:
         stream << "completed";
         break;
      default:
         stream << info_;
      }

      stream << ')';
      break;
   }

   return true;
}

//==============================================================================
//
//  Statistics for each thread.
//
class ThreadStats : public Dynamic
{
public:
   ThreadStats();
   ~ThreadStats();

   CounterPtr       traps_;
   CounterPtr       exceeds_;
   CounterPtr       yields_;
   CounterPtr       interrupts_;
   HighWatermarkPtr maxMsgs_;
   HighWatermarkPtr maxStack_;
   HighWatermarkPtr maxUsecs_;
   AccumulatorPtr   totUsecs_;
};

//------------------------------------------------------------------------------

fn_name ThreadStats_ctor = "ThreadStats.ctor";

ThreadStats::ThreadStats()
{
   Debug::ft(ThreadStats_ctor);

   traps_.reset(new Counter("traps"));
   exceeds_.reset(new Counter("running unpreemptable too long"));
   yields_.reset(new Counter("yields"));
   interrupts_.reset(new Counter("interrupts"));
   maxMsgs_.reset(new HighWatermark("longest length of message queue"));
   maxStack_.reset(new HighWatermark("highest stack usage (words)"));
   maxUsecs_.reset(new HighWatermark("longest time scheduled in (usecs)"));
   totUsecs_.reset(new Accumulator("total execution time (msecs)", 1000));
}

//------------------------------------------------------------------------------

fn_name ThreadStats_dtor = "ThreadStats.dtor";

ThreadStats::~ThreadStats()
{
   Debug::ft(ThreadStats_dtor);
}

//==============================================================================
//
//  Information about a context switch.
//
struct ContextSwitch
{
public:
   //  Initializes the record.
   //
   ContextSwitch();

   //  When the thread started to run.
   //
   ticks_t in;

   //  When the context switch occurred.
   //
   ticks_t out;

   //  The thread being scheduled out.
   //
   ThreadId tid;

   //  The native identifier for the thread being scheduled out.
   //
   SysThreadId nid;

   //  Set if unpreemptable when scheduled out.
   //
   bool locked;

   //  Set if execution overlapped with a previous thread.
   //
   bool overlap;
};

//------------------------------------------------------------------------------

ContextSwitch::ContextSwitch() :
   in(0),
   out(0),
   tid(0),
   nid(0),
   locked (false),
   overlap(false)
{
}

//==============================================================================
//
//  For recording context switches.
//
class ContextSwitches : public Permanent
{
   friend class Singleton< ContextSwitches >;
public:
   //  Deleted to prohibit copying.
   //
   ContextSwitches(const ContextSwitches& that) = delete;
   ContextSwitches& operator=(const ContextSwitches& that) = delete;

   //  Returns true if context switches are being logged.
   //
   bool LoggingOn() const { return log_; }

   //  Starts (stops) logging context switches if ON is true (false).
   //
   TraceRc LogSwitches(bool on);

   //  Returns a pointer to where the context switch record should be
   //  filled in.
   //
   ContextSwitch* AddSwitch();

   //  Displays context switches in STREAM.
   //
   void DisplaySwitches(ostream& stream) const;
private:
   //  Private because this singleton is not subclassed.
   //
   ContextSwitches();

   //  Private because this singleton is not subclassed.
   //
   ~ContextSwitches();

   //  The size of the context switch array.
   //
   size_t capacity_;

   //  The next available entry in the array of context switches.
   //
   size_t next_;

   //  The array of context switches (recent history).
   //
   std::unique_ptr< ContextSwitch[] > switches_;

   //  Set if the array wrapped around (circular buffer).
   //
   bool full_;

   //  Set if context switches are to be logged.
   //
   bool log_;
};

//------------------------------------------------------------------------------
//
//  Critical section lock for the array of context switches.
//
SysMutex ContextSwitchesLock_("ContextSwitchesLock");

//------------------------------------------------------------------------------

fn_name ContextSwitches_ctor = "ContextSwitches.ctor";

ContextSwitches::ContextSwitches() :
   capacity_(0),
   next_(0),
   switches_(nullptr),
   full_(false),
   log_(false)
{
   Debug::ft(ContextSwitches_ctor);

   switches_.reset(new ContextSwitch[4096]);
   capacity_ = 4096;
}

//------------------------------------------------------------------------------

fn_name ContextSwitches_dtor = "ContextSwitches.dtor";

ContextSwitches::~ContextSwitches()
{
   Debug::ft(ContextSwitches_dtor);
}

//------------------------------------------------------------------------------

ContextSwitch* ContextSwitches::AddSwitch()
{
   if(!log_) return nullptr;

   ContextSwitch* cs;

   if(ContextSwitchesLock_.Acquire(TIMEOUT_IMMED) == SysMutex::Acquired)
   {
      cs = &switches_[next_];

      if(++next_ >= capacity_)
      {
         next_ = 0;
         full_ = true;
      }

      ContextSwitchesLock_.Release();
      return cs;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fixed_string SwitchHeader =
   "Timestamp  Id  Host              Ticks In            Ticks Out      uSecs";
// 0         1         2         3         4         5         6         7
// 01234567890123456789012345678901234567890123456789012345678901234567890123

fixed_string SwitchFooter =
   "Symbols: - (preemptable)  ^ (overlap)";

void ContextSwitches::DisplaySwitches(ostream& stream) const
{
   //  For iteration purposes, determine the indices of the FIRST and LAST
   //  elements, as well as the number of entries (ELEMS) in the array.
   //
   if((next_ == 0) && !full_)
   {
      stream << "There were no context switches to display." << CRLF;
      return;
   }

   MutexGuard guard(&ContextSwitchesLock_);

   size_t first = 0;
   size_t last = next_ - 1;
   auto elems = next_;

   if(full_)
   {
      first = next_;
      last = (first == 0 ? capacity_ - 1 : first - 1);
      elems = capacity_;
   }

   //  Selection-sort the context switches by time scheduled in.  Starting at
   //  the second entry, make each successive entry (NEXT) a candidate.  All
   //  of the entries that precede the candidate are sorted.  If the candidate
   //  is less than the highest sorted entry (PREV), shift PREV up one slot so
   //  that the candidate can move down.  Repeat until the previous entries
   //  have shifted to let the candidate move to the correct slot.  Because
   //  the array is almost sorted, check that the candidate will have to move
   //  before bothering to make a copy of it.
   //
   for(auto next = first, count = elems - 1; count > 0; --count)
   {
      next = (next == capacity_ - 1 ? 0 : next + 1);

      auto curr = next;
      auto prev = (next == 0 ? capacity_ - 1 : next - 1);

      if(switches_[prev].in > switches_[curr].in)
      {
         auto candidate = switches_[next];

         while(switches_[prev].in > candidate.in)
         {
            switches_[curr] = switches_[prev];
            prev = (prev == 0 ? capacity_ - 1 : prev - 1);
            curr = (curr == 0 ? capacity_ - 1 : curr - 1);
            if(curr == first) break;
         }

         switches_[curr] = candidate;
      }
   }

   //  Look for overlapped executions.  Look at the time that each entry
   //  was scheduled out.  Flag an overlap while any following entries
   //  were scheduled in before this time.
   //
   for(size_t i = first, count = elems - 1; count > 0; --count)
   {
      auto j = (i == capacity_ - 1 ? 0 : i + 1);

      while(switches_[i].out > switches_[j].in)
      {
         switches_[j].overlap = true;
         if(j == last) break;
         j = (j == capacity_ - 1 ? 0 : j + 1);
      }

      i = (i == capacity_ - 1 ? 0 : i + 1);
   }

   stream << CRLF;
   stream << "Context switches: " << elems << CRLF;
   stream << SwitchHeader << CRLF;

   for(size_t i = first; elems > 0; --elems)
   {
      auto curr = &switches_[i];

      auto time = Clock::TicksToTime(curr->in, MinsField);
      stream << time;
      stream << setw(4) << curr->tid << spaces(2);
      stream << setw(4) << strHex(curr->nid, 4, false);

      auto c = SPACE;
      if(!curr->locked) c = '-';
      stream << c;

      stream << setw(21) << curr->in;
      stream << setw(21) << curr->out;

      c = SPACE;
      if(curr->overlap) c = '^';
      stream << c;

      auto ticks = curr->out - curr->in;
      auto usecs = Clock::TicksToUsecs(ticks);
      stream << setw(10) << usecs << spaces(2) << CRLF;

      i = (i == capacity_ - 1 ? 0 : i + 1);
   }

   stream << SwitchFooter << CRLF;
}

//------------------------------------------------------------------------------

fn_name ContextSwitches_LogSwitches = "ContextSwitches.LogSwitches";

TraceRc ContextSwitches::LogSwitches(bool on)
{
   Debug::ft(ContextSwitches_LogSwitches);

   if(on)
   {
      if(log_) return AlreadyStarted;
      next_ = 0;
      full_ = false;
      log_ = true;
      return TraceOk;
   }

   log_ = false;
   return TraceOk;
}

//==============================================================================
//
//  Registry for orphans.  An orphan is a deleted Thread that is not yet
//  exited.  Its SysThread object still exists, and it must be exited at
//  the first opportunity.
//
class Orphans : public Permanent
{
   friend class Singleton< Orphans >;
public:
   //  Deleted to prohibit copying.
   //
   Orphans(const Orphans& that) = delete;
   Orphans& operator=(const Orphans& that) = delete;

   //  Adds THR to the list of orphans when a Thread object is unexpectedly
   //  deleted.
   //
   void Register(SysThread* thr);

   //  If the running thread is in the list of orphans, returns true after
   //  deleting its SysThread object.
   //
   bool ExitNow();
private:
   //  Private because this singleton is not subclassed.
   //
   Orphans();

   //  Private because this singleton is not subclassed.
   //
   ~Orphans();

   //  The orphans array.
   //
   Array< SysThread* > orphans_;
};

//------------------------------------------------------------------------------
//
//  Critical section lock for the array of orphans.
//
SysMutex OrphansLock_("OrphansLock");

//------------------------------------------------------------------------------

fn_name Orphans_ctor = "Orphans.ctor";

Orphans::Orphans()
{
   Debug::ft(Orphans_ctor);

   orphans_.Init(32, MemPerm);
}

//------------------------------------------------------------------------------

fn_name Orphans_dtor = "Orphans.dtor";

Orphans::~Orphans()
{
   Debug::ft(Orphans_dtor);
}

//------------------------------------------------------------------------------

fn_name Orphans_ExitNow = "Orphans.ExitNow";

bool Orphans::ExitNow()
{
   Debug::ft(Orphans_ExitNow);

   //  Search the list of orphans for one whose native thread identifier is
   //  the same as the running thread.  If such a thread exists, delete it
   //  and move the last entry up to keep the entries contiguous.
   //
   auto pid = SysThread::RunningThreadId();

   MutexGuard guard(&OrphansLock_);

   for(size_t i = 0; i < orphans_.Size(); ++i)
   {
      auto orphan = orphans_[i];

      if(orphan->Nid() == pid)
      {
         //  The orphan's entry must be overwritten before invoking other
         //  functions so that we don't cause infinite recursion.  If the
         //  orphan holds a lock, the platform kernel must recover it now,
         //  when the orphan is deleted, or in a moment, when the orphan
         //  exits.
         //
         orphans_.Erase(i);
         ThreadAdmin::Incr(ThreadAdmin::Orphans);
         delete orphan;
         Debug::SwLog(Orphans_ExitNow, "orphan exited", pid);
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Orphans_Register = "Orphans.Register";

void Orphans::Register(SysThread* thr)
{
   Debug::ft(Orphans_Register);

   if(thr == nullptr) return;

   MutexGuard guard(&OrphansLock_);

   if(!orphans_.PushBack(thr))
   {
      Debug::SwLog(Orphans_Register, "failed to register orphan", thr->Nid());
   }
}

//==============================================================================
//
//  What to do with a thread on the next scheduling operation.
//
enum SchedulingAction
{
   RunThread,    // default value
   SleepThread,  // force thread to sleep
   ExitThread    // force thread to exit
};

//==============================================================================
//
//  Per-thread data not required in the header (PIMPL idiom).  Declaring member
//  data here reduces the number of #includes in the header and sometimes allows
//  new capabilities to be added without significant recompilation.  Member data
//  is declared in the header
//  o to survive deletion of this object (see comment about deleted_)
//  o for performance (to allow inlining or avoid an extra dereference)
//
class ThreadPriv : public Permanent
{
public:
   //  Creates the thread's private data.
   //
   ThreadPriv();

   //  Deletes the thread's private data.
   //
   ~ThreadPriv();

   //  Invoked to recreate a thread.
   //
   void Recreate();

   //  Overridden to display member variables.
   //
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   //  The thread's stack pointer after entering Thread::Start.
   //
   const signal_t* stackBase_;

   //  Calls to MakeUnpreemptable minus calls to MakePreemptable.
   //
   uint8_t unpreempts_;

   //  Calls to MemUnprotect minus calls to MemProtect.
   //
   uint8_t unprotects_;

   //  The depth of nested software logs.
   //
   uint8_t swlogs_;

   //  Determines whether the thread has failed to yield too often.
   //
   LeakyBucketCounter rtcLbc_;

   //  Determines whether the thread has trapped too often.
   //
   LeakyBucketCounter trapLbc_;

   //  Set if the thread has been entered.
   //
   bool entered_;

   //  Set when StartTracing begins a trace.
   //
   bool tracing_;

   //  Set if tracing is to be stopped on the next context switch.
   //
   bool autostop_;

   //  Set if the thread was sent a SIGYIELD when traps on
   //  SIGYIELD were disabled.
   //
   bool warned_;

   //  Set if the thread is to be trapped.
   //
   bool trap_;

   //  Set if thread is undergoing recovery after a trap.
   //
   bool recovering_;

   //  Set if the thread is exiting.
   //
   bool exiting_;

   //  Set if the thread is waiting to be recreated.
   //
   bool recreate_;

   //  Determines what happens to the thread on a scheduling operation.
   //
   SchedulingAction action_;

   //  The signal to be raised or that is being handled.
   //
   signal_t signal_;

   //  Flags set when Interrupt was invoked on the thread.
   //
   std::atomic_uint32_t vector_;

   //  The number of microseconds that the thread ran during the previous
   //  short interval for thread statistics.  This provides a view of how
   //  thread behavior has recently changed.
   //
   usecs_t prevUsecs_;

   //  The number of microseconds that the thread has run during the
   //  current short interval for thread statistics.
   //
   usecs_t currUsecs_;

   //  The last time at which the thread started to run unpreemptably.
   //  If the thread is preemptable, the last time it exited a blocking
   //  operation, although it may have been scheduled out and back in
   //  several times since then.
   //
   ticks_t currStart_;

   //  The time at which the thread will be trapped if it has not yielded.
   //  Reset to 0 when the thread yields, and set when it resumes running
   //  unpreemptably.
   //
   ticks_t currEnd_;

   //  Causes a stack check each time it counts down to one.
   //
   static size_t StackCheckCounter_;

   //  The time when the previous short interval for thread statistics began.
   //
   static ticks_t PrevIntervalStart_;

   //  The time when the current short interval for thread statistics began.
   //
   static ticks_t CurrIntervalStart_;

   //  The amount of idle time during the most recent short interval.
   //
   static usecs_t TimeIdle_;

   //  The time spent in threads during the most recent short interval.
   //
   static usecs_t TimeUsed_;
};

//------------------------------------------------------------------------------

size_t ThreadPriv::StackCheckCounter_ = 1;
ticks_t ThreadPriv::PrevIntervalStart_ = 0;
ticks_t ThreadPriv::CurrIntervalStart_ = 0;
usecs_t ThreadPriv::TimeIdle_ = 0;
usecs_t ThreadPriv::TimeUsed_ = 0;

//------------------------------------------------------------------------------

fn_name ThreadPriv_ctor = "ThreadPriv.ctor";

ThreadPriv::ThreadPriv() :
   stackBase_(nullptr),
   unpreempts_(1),
   unprotects_(0),
   swlogs_(0),
   entered_(false),
   tracing_(false),
   autostop_(false),
   warned_(false),
   trap_(false),
   recovering_(false),
   exiting_(false),
   recreate_(false),
   action_(RunThread),
   signal_(SIGNIL),
   vector_(0),
   prevUsecs_(0),
   currUsecs_(0),
   currStart_(0),
   currEnd_(0)
{
   Debug::ft(ThreadPriv_ctor);

   rtcLbc_.Initialize(ThreadAdmin::RtcLimit(), ThreadAdmin::RtcInterval());
   trapLbc_.Initialize(ThreadAdmin::TrapLimit(), ThreadAdmin::TrapInterval());
}

//------------------------------------------------------------------------------

fn_name ThreadPriv_dtor = "ThreadPriv.dtor";

ThreadPriv::~ThreadPriv()
{
   Debug::ft(ThreadPriv_dtor);
}

//------------------------------------------------------------------------------

void ThreadPriv::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "stackBase  : " << stackBase_ << CRLF;
   stream << prefix << "unpreempts : " << int(unpreempts_) << CRLF;
   stream << prefix << "unprotects : " << int(unprotects_) << CRLF;
   stream << prefix << "swlogs     : " << int(swlogs_) << CRLF;
   stream << prefix << "rtcLbc     : " << CRLF;
   rtcLbc_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "trapLbc    : " << CRLF;
   trapLbc_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "entered    : " << entered_ << CRLF;
   stream << prefix << "tracing    : " << tracing_ << CRLF;
   stream << prefix << "autostop   : " << autostop_ << CRLF;
   stream << prefix << "warned     : " << warned_ << CRLF;
   stream << prefix << "trap       : " << trap_ << CRLF;
   stream << prefix << "recovering : " << recovering_ << CRLF;
   stream << prefix << "exiting    : " << exiting_ << CRLF;
   stream << prefix << "recreate   : " << recreate_ << CRLF;
   stream << prefix << "action     : " << action_ << CRLF;
   stream << prefix << "signal     : " << signal_ << CRLF;
   stream << prefix << "vector     : " << std::hex << vector_
      << std::dec << CRLF;
   stream << prefix << "prevUsecs  : " << prevUsecs_ << CRLF;
   stream << prefix << "currUsecs  : " << currUsecs_ << CRLF;
   stream << prefix << "currStart  : " << currStart_ << CRLF;
   stream << prefix << "currEnd    : " << currEnd_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name ThreadPriv_Recreate = "ThreadPriv.Recreate";

void ThreadPriv::Recreate()
{
   Debug::ft(ThreadPriv_Recreate);

   //  Reset a subset of the thread's data.
   //
   stackBase_ = nullptr;
   unpreempts_ = 1;
   unprotects_ = 0;
   swlogs_ = 0;
   entered_ = false;
   tracing_ = false;
   autostop_ = false;
// warned_ = (preserved)
// trap_ = (preserved)
   recovering_ = false;
   exiting_ = false;
   recreate_ = false;
// action_ = (preserved)
   signal_ = SIGNIL;
// vector_ = (preserved)
// prevUsecs_ = (preserved)
// currUsecs_ = (preserved)
// currStart_ = (preserved)
// currEnd_ = (preserved)

   rtcLbc_.Initialize(ThreadAdmin::RtcLimit(), ThreadAdmin::RtcInterval());
   trapLbc_.Initialize(ThreadAdmin::TrapLimit(), ThreadAdmin::TrapInterval());
}

//==============================================================================

fixed_string UnknownExceptionStr = "unknown exception";
fixed_string ThreadDataStr = "Thread Data:";
fixed_string TrapDuringRecoveryStr = "Trap during recovery.";
fixed_string TrapLimitReachedStr = "Trap limit exceeded.";
fixed_string ClosingConsoleStr = "Closing console in 10 seconds...";

//------------------------------------------------------------------------------
//
//  Mapping of scheduler factions to thread priorities.
//
//  The payload through audit factions have the same priority.  At present,
//  proportional scheduling must be approximated by engineering the number of
//  threads in each faction and the average time that each one runs.  However,
//  the design of RootThread (watchdog faction) and InitThread (system faction)
//  requires higher priorities.  The overall priority scheme is therefore
//
//    watchdog > system > payload/maintenance/provisioning/background/audit
//
const SysThread::Priority FactionMap[Faction_N] =
{
   SysThread::LowPriority,      // IdleFaction
   SysThread::DefaultPriority,  // AuditFaction
   SysThread::DefaultPriority,  // BackgroundFaction
   SysThread::DefaultPriority,  // OperationsFaction
   SysThread::DefaultPriority,  // MaintenanceFaction
   SysThread::DefaultPriority,  // PayloadFaction
   SysThread::DefaultPriority,  // LoadTestFaction
   SysThread::SystemPriority,   // SystemFaction
   SysThread::WatchdogPriority  // WatchdogFaction
};

//------------------------------------------------------------------------------
//
//  The thread that is running or which has been scheduled to run.
//  Excludes RootThread and InitThread.
//
Thread* ActiveThread_ = nullptr;

//  The thread at which to start searching for the thread to be
//  scheduled in.  Scheduling is currently round-robin but will
//  eventually be changed to support proportional scheduling.
//
id_t Start_ = 1;

//  Set to restrict scheduling to specific factions during a restart.
//
bool FactionsRestricted_ = true;

//------------------------------------------------------------------------------

bool FactionAllowed(Faction f)
{
   if(FactionsRestricted_)
   {
      //  Some factions are not scheduled while preparing to shut down.
      //
      switch(f)
      {
      case PayloadFaction:
      case LoadTestFaction:
      case AuditFaction:
         return false;
      }
   }

   return true;
}

//------------------------------------------------------------------------------

const Thread::Id Thread::MaxId = 99;

//------------------------------------------------------------------------------

fn_name Thread_ctor = "Thread.ctor";

Thread::Thread(Faction faction) :
   faction_(faction),
   blocked_(NotBlocked),
   status_(TraceDefault),
   waiting_(false),
   traceMsg_(false),
   deleted_(false),
   trapped_(false),
   readyTime_(UINT64_MAX)
{
   Debug::ft(Thread_ctor);

   priv_.reset(new ThreadPriv);
   stats_.reset(new ThreadStats);

   msgq_.Init(Pooled::LinkDiff());

   auto reg = Singleton< ThreadRegistry >::Instance();

   if(reg->Threads().Empty())
   {
      //  There are no threads, so we must be wrapping the root thread.
      //
      ThreadPriv::CurrIntervalStart_ = Clock::TicksNow();

      Singleton< Orphans >::Instance();
      Singleton< ContextSwitches >::Instance();

      systhrd_.reset(new SysThread);
      priv_->currStart_ = Clock::TicksZero();
      priv_->entered_ = true;
      tid_.SetId(1);
   }
   else
   {
      //  Create a new thread.  StackUsageLimit is in words, so convert
      //  it to bytes.
      //
      auto prio = FactionToPriority(faction_);
      systhrd_.reset(new SysThread(this, EnterThread, prio,
         ThreadAdmin::StackUsageLimit() << BYTES_PER_WORD_LOG2));
   }

   ThreadAdmin::Incr(ThreadAdmin::Creations);
   Debug::Assert(reg->BindThread(*this));
}

//------------------------------------------------------------------------------

fn_name Thread_dtor = "Thread.dtor";

Thread::~Thread()
{
   Debug::ft(Thread_dtor);

   ThreadAdmin::Incr(ThreadAdmin::Deletions);

   //  If the thread doesn't have a native thread, it can be safely
   //  deleted.
   //
   if((systhrd_ == nullptr) || (systhrd_->Nid() == NIL_ID))
   {
      ReleaseResources(false);
      return;
   }

   //  If the running thread invoked Thread::Exit and did not want to be
   //  recreated, it is about to return, so delete its native thread.
   //
   if(priv_->exiting_)
   {
      systhrd_->status_.set(SysThread::IsExiting);
      Suspend();
      ReleaseResources(false);
      return;
   }

   //  This thread is still active and did not invoke Thread::Exit.  This
   //  is a serious error, so output a log now.
   //
   auto log = Log::Create(ThreadLogGroup, ThreadDeleted);

   if(log != nullptr)
   {
      *log << Log::Tab << "thread=" << to_str() << CRLF;
      SysThreadStack::Display(*log, 0);
      *log << Log::Tab << ThreadDataStr << CRLF;
      Display(*log, Log::Tab + spaces(2), NoFlags);
      Log::Submit(log);
   }

   ReleaseResources(true);
}

//------------------------------------------------------------------------------

fn_name Thread_AbbrName = "Thread.AbbrName";

c_string Thread::AbbrName() const
{
   Debug::ft(Thread_AbbrName);

   Debug::SwLog(Thread_AbbrName, strOver(this), 0);
   return "unknown";
}

//------------------------------------------------------------------------------

Thread* Thread::ActiveThread()
{
   auto thr = ActiveThread_;
   if(thr == nullptr) return nullptr;
   if(thr->deleted_) return nullptr;
   return thr;
}

//------------------------------------------------------------------------------

TraceStatus Thread::CalcStatus(bool dynamic) const
{
   if(dynamic && traceMsg_) return TraceIncluded;
   if(status_ != TraceDefault) return status_;

   auto nbt = Singleton< NbTracer >::Instance();
   auto status = nbt->FactionStatus(faction_);
   if(status != TraceDefault) return status;

   auto buff = Singleton< TraceBuffer >::Instance();
   if(buff->FilterIsOn(TraceAll)) return TraceIncluded;
   return TraceExcluded;
}

//------------------------------------------------------------------------------

ptrdiff_t Thread::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Thread* >(&local);
   return ptrdiff(&fake->tid_, fake);
}

//------------------------------------------------------------------------------

fn_name Thread_ChangeFaction = "Thread.ChangeFaction";

bool Thread::ChangeFaction(Faction faction)
{
   Debug::ft(Thread_ChangeFaction);

   if(faction == faction_) return true;

   if(faction >= SystemFaction)
   {
      Debug::SwLog(Thread_ChangeFaction, AbbrName(), faction);
      return false;
   }

   //  Currently, application factions only have two priorities; the lower
   //  one prevents the platform from scheduling a preemptable thread that
   //  we have scheduled out.  Consequently, a thread's priority does not
   //  change when its faction changes.  If our use of priorities changes,
   //  it may also be necessary to adjust the thread's priority here.

   faction_ = faction;
   return true;
}

//------------------------------------------------------------------------------

fn_name Thread_Claim = "Thread.Claim";

void Thread::Claim()
{
   Debug::ft(Thread_Claim);

   Pooled::Claim();

   //  Claim messages on the queue.  Sometimes there are hundreds of these,
   //  so trying to add them all to GetSubtended's array isn't possible.
   //
   for(auto m = msgq_.First(); m != nullptr; msgq_.Next(m))
   {
      m->Claim();
   }
}

//------------------------------------------------------------------------------

fn_name Thread_Cleanup = "Thread.Cleanup";

void Thread::Cleanup()
{
   Debug::ft(Thread_Cleanup);

   //  When an object is recovered by the object pool audit, its destructor
   //  is not invoked.  It must therefore free resources that the destructor
   //  would normally free.
   //
   stats_.reset();
   priv_.reset();
   ReleaseResources(true);
   Pooled::Cleanup();
}

//------------------------------------------------------------------------------

void Thread::ClearActiveThread(const Thread* thr)
{
   if(ActiveThread_ == thr) ActiveThread_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name Thread_DeqMsg = "Thread.DeqMsg";

MsgBuffer* Thread::DeqMsg(msecs_t timeout)
{
   Debug::ft(Thread_DeqMsg);

   auto buff = msgq_.Deq();

   if(buff == nullptr)
   {
      if(timeout == TIMEOUT_IMMED) return nullptr;

      switch(Pause(timeout))
      {
      case DelayError:
         Restart::Initiate(ThreadPauseFailed, Tid());
         return nullptr;

      case DelayCompleted:
      case DelayInterrupted:
         buff = msgq_.Deq();
         if(buff != nullptr) break;
         //  [[fallthrough]]
      default:
         return nullptr;
      }
   }

   traceMsg_ = (buff->GetStatus() == TraceIncluded);
   return buff;
}

//------------------------------------------------------------------------------

fn_name Thread_Destroy = "Thread.Destroy";

void Thread::Destroy()
{
   Debug::ft(Thread_Destroy);

   delete this;
}

//------------------------------------------------------------------------------

void Thread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   auto lead = prefix + spaces(2);
   stream << prefix << "systhrd   : " << systhrd_.get() << CRLF;
   if(systhrd_ != nullptr) systhrd_->Display(stream, lead, options);
   stream << prefix << "tid       : " << tid_.to_str() << CRLF;
   stream << prefix << "faction   : " << int(faction_) << CRLF;
   stream << prefix << "blocked   : " << blocked_ << CRLF;
   stream << prefix << "status    : " << status_ << CRLF;
   stream << prefix << "waiting   : " << waiting_ << CRLF;
   stream << prefix << "traceMsg  : " << traceMsg_ << CRLF;
   stream << prefix << "deleted   : " << deleted_ << CRLF;
   stream << prefix << "trapped   : " << trapped_ << CRLF;
   stream << prefix << "readyTime : " << readyTime_ << CRLF;
   stream << prefix << "msgq      : " << CRLF;
   msgq_.Display(stream, lead, options);
   stream << prefix << "priv      : " << CRLF;
   priv_->Display(stream, lead, options);
}

//------------------------------------------------------------------------------

void Thread::DisplayContextSwitches(ostream& stream)
{
   Singleton< ContextSwitches >::Instance()->DisplaySwitches(stream);
}

//------------------------------------------------------------------------------

fn_name Thread_DisplayStats = "Thread.DisplayStats";

void Thread::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft(Thread_DisplayStats);

   stream << spaces(2) << AbbrName()
      << SPACE << strIndex(Tid(), 0, false) << CRLF;

   stats_->traps_->DisplayStat(stream, options);
   stats_->exceeds_->DisplayStat(stream, options);
   stats_->yields_->DisplayStat(stream, options);
   stats_->interrupts_->DisplayStat(stream, options);
   stats_->maxMsgs_->DisplayStat(stream, options);
   stats_->maxStack_->DisplayStat(stream, options);
   stats_->maxUsecs_->DisplayStat(stream, options);
   stats_->totUsecs_->DisplayStat(stream, options);
}

//------------------------------------------------------------------------------

const size_t SchedHeaderSize = 3;

fixed_string SchedHeader[SchedHeaderSize] =
{
// 0      1         2         3         4         5         6         7
// 34567890123456789012345678901234567890123456789012345678901234567890123456
"      THREADS          |    SINCE START OF CURRENT 15-MIN INTERVAL    | LAST",
"                       |            rtc  max   max     max  total     |5 SEC",
"id    name     host f b| ex yields  t/o msgs stack   usecs  msecs %cpu| %cpu"
};

void Thread::DisplaySummaries(ostream& stream)
{
   string line(strlen(SchedHeader[0]), '-');

   ticks_t ticks;     // start of current interval
   usecs_t time0;     // duration of current interval
   size_t idle0;      // idle time during current interval
   size_t used0 = 0;  // time in all threads during current interval

   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

   for(auto t = threads.First(); t != nullptr; threads.Next(t))
   {
      used0 += t->stats_->totUsecs_->Curr();
   }

   ticks = Clock::TicksSince(StatisticsRegistry::StartTicks());
   time0 = Clock::TicksToUsecs(ticks);
   idle0 = (time0 > used0 ? time0 - used0 : 0);

   stream << std::setprecision(1) << std::fixed;

   stream << "SCHEDULER REPORT: " << Element::strTimePlace() << CRLF;
   stream << "for interval beginning at ";
   stream << Clock::TicksToTime(StatisticsRegistry::StartTicks()) << CRLF;

   stream << line << CRLF;
   for(auto i = 0; i < SchedHeaderSize; ++i) stream << SchedHeader[i] << CRLF;
   stream << line << CRLF;

   stream << setw(10) << "idle";
   stream << setw(55) << (idle0 + 500) / 1000;
   stream << setw(5) << 100 * (double(idle0) / time0);

   //  Set TIME1 to the length of the previous short interval.
   //
   auto time1 = ThreadPriv::TimeIdle_ + ThreadPriv::TimeUsed_;

   if(time1 > 0)
   {
      stream << setw(6) << 100 * (double(ThreadPriv::TimeIdle_) / time1);
   }

   stream << CRLF;

   for(auto t = threads.First(); t != nullptr; threads.Next(t))
   {
      t->DisplaySummary(stream, time0, time1);
   }

   stream << line << CRLF;

   if(Singleton< ContextSwitches >::Instance()->LoggingOn())
   {
      stream << "Context switch logging is ON." << CRLF;
   }
}

//------------------------------------------------------------------------------

void Thread::DisplaySummary
   (ostream& stream, usecs_t time0, usecs_t time1) const
{
   auto currUsecs = stats_->totUsecs_->Curr();

   stream << setw(2) << Tid();
   stream << setw(8) << AbbrName() << SPACE;
   stream << setw(8) << std::hex << NativeThreadId() << std::dec;

   auto f = FactionChar(faction_);
   if(priv_->unpreempts_ == 0) f = tolower(f);
   stream << setw(2) << f;

   auto r = BlockingReasonChar(blocked_);
   r = (blocked_ == NotBlocked ? SPACE : toupper(r));
   stream << setw(2) << r;

   stream << setw(4) << stats_->traps_->Curr();
   stream << setw(7) << stats_->yields_->Curr();
   stream << setw(5) << stats_->exceeds_->Curr();
   stream << setw(5) << stats_->maxMsgs_->Curr();
   stream << setw(6) << stats_->maxStack_->Curr();

   auto usecs = stats_->maxUsecs_->Curr();

   if(usecs <= 9999999)
      stream << setw(8) << usecs;
   else
      stream << " 10+ sec";

   stream << setw(7) << (currUsecs + 500) / 1000;
   stream << setw(5) << 100 * (double(currUsecs) / time0);

   if(time1 > 0)
   {
      stream << setw(6) << 100 * (double(priv_->prevUsecs_) / time1);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Thread_EnqMsg = "Thread.EnqMsg";

bool Thread::EnqMsg(MsgBuffer& msg)
{
   Debug::ft(Thread_EnqMsg);

   if(msgq_.Enq(msg))
   {
      auto size = msgq_.Size();
      stats_->maxMsgs_->Update(size);
      Interrupt();
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Thread_Enter = "Thread.Enter";

void Thread::Enter()
{
   Debug::ft(Thread_Enter);

   Debug::SwLog(Thread_Enter, strOver(this), 0);
}

//------------------------------------------------------------------------------

fn_name Thread_EnterBlockingOperation = "Thread.EnterBlockingOperation";

bool Thread::EnterBlockingOperation(BlockingReason why, fn_name_arg func)
{
   Debug::ft(Thread_EnterBlockingOperation);

   if(why == NotBlocked)
   {
      Debug::SwLog(Thread_EnterBlockingOperation, "invalid reason", why);
      return false;
   }

   auto thr = RunningThread();

   if(thr->priv_->action_ == ExitThread)
   {
      thr->ExitIfSafe(2);
   }

   if(!thr->BlockingAllowed(why, func)) return false;

   thr->blocked_ = why;
   thr->Suspend();
   return true;
}

//------------------------------------------------------------------------------

fn_name Thread_EnterSwLog = "Thread.EnterSwLog";

bool Thread::EnterSwLog()
{
   Debug::ft(Thread_EnterSwLog);

   //  If the thread is already generating nested software logs, prevent
   //  further nesting.
   //
   auto thr = RunningThread(false);
   if(thr == nullptr) return true;
   if(thr->priv_ == nullptr) return true;
   if(++thr->priv_->swlogs_ <= 2) return true;
   --thr->priv_->swlogs_;
   return false;
}

//------------------------------------------------------------------------------

main_t Thread::EnterThread(void* arg)
{
   Debug::ft(Thread_EnterThread);

   auto self = static_cast< Thread* >(arg);

   //  A thread may start to run before its Thread object is fully constructed.
   //  This causes a trap, so the thread must wait until it is constructed and
   //  has been signalled to run.
   //
   while((self->systhrd_ == nullptr) ||
         (self->priv_ == nullptr) ||
         (self->stats_ == nullptr))
   {
      Debug::noop();
   }

   self->Ready();

   //  If the thread is orphaned, it must exit immediately.  This occurs if
   //  the thread's constructor trapped, which resulted in its deletion but
   //  the survival of its native thread (systhrd_), which is now about to
   //  run without a Thread object.
   //
   if(Singleton< Orphans >::Instance()->ExitNow())
   {
      self->Schedule();
      return SIGDELETED;
   }

   //  Our argument (self) is a pointer to a Thread.  Invoke its entry
   //  function after configuring it to "resume" execution and to catch
   //  signals.
   //
   self->Resume(Thread_EnterThread);
   RegisterForSignals();
   return self->Start();
}

//------------------------------------------------------------------------------

fn_name Thread_Exit = "Thread.Exit";

main_t Thread::Exit(signal_t sig)
{
   Debug::ft(Thread_Exit);

   //  During trap recovery, force the thread to exit immediately.
   //
   if(trapped_)
   {
      return ForceExit(sig);
   }

   auto reg = Singleton< PosixSignalRegistry >::Instance();

   SetSignal(sig);

   if(LogSignal(sig) || Element::RunningInLab())
   {
      auto log = Log::Create(ThreadLogGroup, ThreadExited);

      if(log != nullptr)
      {
         *log << Log::Tab << "thread=" << to_str() << CRLF;
         *log << Log::Tab << "signal=" << reg->strSignal(sig);
         Log::Submit(log);
      }
   }

   priv_->exiting_ = true;

   //  Recreate the thread if it did not exit voluntarily and did not
   //  receive a final signal.
   //
   if(sig != SIGNIL)
      priv_->recreate_ = !reg->Attrs(sig).test(PosixSignal::Final);
   else
      priv_->recreate_ = false;

   if(priv_->recreate_)
   {
      ClearActiveThread(this);
      priv_->stackBase_ = nullptr;
      systhrd_.reset();
      Singleton< InitThread >::Instance()->Interrupt(InitThread::RecreateMask);
   }
   else
   {
      Destroy();
   }

   return sig;
}

//------------------------------------------------------------------------------

void Thread::ExitBlockingOperation(fn_name_arg func)
{
   Debug::ft(Thread_ExitBlockingOperation);

   auto thr = RunningThread();
   thr->blocked_ = NotBlocked;

   //  Check if the thread is being forced to sleep or exit.
   //
   switch(thr->priv_->action_)
   {
   case SleepThread:
      Pause(TIMEOUT_NEVER);
      return;
   case ExitThread:
      thr->ExitIfSafe(1);
   }

   thr->Ready();
   thr->Resume(func);
}

//------------------------------------------------------------------------------

fn_name Thread_ExitIfSafe = "Thread.ExitIfSafe";

void Thread::ExitIfSafe(debug32_t offset)
{
   //  Reset action_ to prevent this from being invoked recursively.
   //  If it isn't safe to exit the thread now, try again later.
   //
   priv_->action_ = RunThread;

   Debug::ft(Thread_ExitIfSafe);

   if(!trapped_ && SysThreadStack::TrapIsOk())
   {
      SetTrap(false);
      throw SignalException(priv_->signal_, offset);
   }

   priv_->action_ = ExitThread;
}

//------------------------------------------------------------------------------

fn_name Thread_ExitOnRestart = "Thread.ExitOnRestart";

bool Thread::ExitOnRestart(RestartLevel level) const
{
   Debug::ft(Thread_ExitOnRestart);

   //  RootThread and InitThread run during a restart.  A thread blocked on
   //  stream input, such as CinThread, cannot be forced to exit because C++
   //  has no mechanism for interrupting it.
   //
   if(faction_ >= SystemFaction) return false;
   if(blocked_ == BlockedOnConsole) return false;
   return true;
}

//------------------------------------------------------------------------------

fn_name Thread_ExitSwLog = "Thread.ExitSwLog";

void Thread::ExitSwLog(bool all)
{
   Debug::ft(Thread_ExitSwLog);

   auto thr = RunningThread(false);
   if(thr == nullptr) return;
   if(thr->priv_ == nullptr) return;
   if(thr->priv_->swlogs_ == 0) return;
   if(all)
      thr->priv_->swlogs_ = 0;
   else
      --thr->priv_->swlogs_;
}

//------------------------------------------------------------------------------

fn_name Thread_ExtendTime = "Thread.ExtendTime";

void Thread::ExtendTime(msecs_t msecs)
{
   Debug::ft(Thread_ExtendTime);

   //  Time cannot be extended for an orphaned thread: its Thread object has
   //  been deleted.  This is invoked during exception handling, so don't get
   //  upset if the thread can't be found.
   //
   auto thr = RunningThread(false);
   if(thr == nullptr) return;
   thr->priv_->currEnd_ += Clock::MsecsToTicks(msecs);
}

//------------------------------------------------------------------------------

fn_name Thread_FactionToPriority = "Thread.FactionToPriority";

SysThread::Priority Thread::FactionToPriority(Faction& faction)
{
   Debug::ft(Thread_FactionToPriority);

   if(faction < Faction_N) return FactionMap[faction];

   Debug::SwLog(Thread_FactionToPriority, "invalid faction", faction);
   faction = BackgroundFaction;
   return SysThread::DefaultPriority;
}

//------------------------------------------------------------------------------

fn_name Thread_ForceExit = "Thread.ForceExit";

main_t Thread::ForceExit(signal_t sig)
{
   Debug::ft(Thread_ForceExit);

   //  After logging the problem, destroy this Thread object.
   //
   auto reg = Singleton< PosixSignalRegistry >::Instance();
   auto log = Log::Create(ThreadLogGroup, ThreadForcedToExit);

   if(log != nullptr)
   {
      *log << Log::Tab << "thread=" << to_str() << CRLF;
      *log << Log::Tab << "signal=" << reg->strSignal(sig);
      Log::Submit(log);
   }

   Destroy();
   return sig;
}

//------------------------------------------------------------------------------

void Thread::FunctionInvoked(fn_name_arg func)
{
   Thread* thr = nullptr;

   //  This handles the following:
   //  (a) Adding FUNC to a trace.
   //  (b) Causing a trap after a thread is scheduled in.
   //  (c) Causing a trap before a thread overflows its stack.
   //
   if(Debug::FcFlags_.test(Debug::TracingActive))
   {
      if(TraceRunningThread(thr))
      {
         FunctionTrace::Capture(func);
      }
   }

   if(Debug::FcFlags_.test(Debug::TrapPending))
   {
      if(thr == nullptr) thr = RunningThread(false);
      if(thr == nullptr) return;
      thr->TrapCheck();
   }

   if(Debug::FcFlags_.test(Debug::StackChecking))
   {
      if(ThreadPriv::StackCheckCounter_ <= 1)
      {
         if(thr == nullptr) thr = RunningThread(false);
         if(thr == nullptr) return;
         thr->StackCheck();
      }
      else
      {
         --ThreadPriv::StackCheckCounter_;
      }
   }
}

//------------------------------------------------------------------------------

fn_name Thread_HandleSignal = "Thread.HandleSignal";

bool Thread::HandleSignal(signal_t sig, uint32_t code)
{
   Debug::ft(Thread_HandleSignal);  //@

   auto thr = RunningThread(false);

   if(thr != nullptr)
   {
      //  If the thread is supposed to exit, override SIG with the one
      //  already set for the thread.  This handles the case of a trap
      //  occurring before ExitIfSafe throws an exception.
      //
      if(thr->priv_->action_ == ExitThread)
      {
         sig = thr->priv_->signal_;
      }

      //  Turn the signal into a standard C++ exception so that ic can
      //  be caught and recovery action initiated.
      //
      throw SignalException(sig, code);
   }

   //  The running thread could not be identified.  A break signal (e.g.
   //  on ctrl-C) is sometimes delivered on an unregistered thread.  If
   //  the RTC timeout is not being enforced and the locked thread has
   //  run too long, trap it; otherwise, assume that the purpose of the
   //  ctrl-C is to trap the CLI thread so that it will abort its work.
   //
   auto reg = Singleton< PosixSignalRegistry >::Instance();

   if(reg->Attrs(sig).test(PosixSignal::Break))
   {
      if(!ThreadAdmin::TrapOnRtcTimeout())
      {
         thr = LockedThread();

         if((thr != nullptr) && (Clock::TicksUntil(thr->priv_->currEnd_) > 0))
         {
            thr = nullptr;
         }
      }

      if(thr == nullptr) thr = Singleton< CliThread >::Extant();
      if(thr == nullptr) return false;
      thr->Raise(sig);
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

bool Thread::HasExited() const
{
   return priv_->recreate_;
}

//------------------------------------------------------------------------------

fn_name Thread_InitialMsecs = "Thread.InitialMsecs";

msecs_t Thread::InitialMsecs() const
{
   Debug::ft(Thread_InitialMsecs);

   return ThreadAdmin::RtcTimeoutMsecs();
}

//------------------------------------------------------------------------------

fn_name Thread_Interrupt = "Thread.Interrupt";

bool Thread::Interrupt(const Flags& mask)
{
   Debug::ft(Thread_Interrupt);

   if(deleted_) return false;

   //  Update the thread's vector.  This always occurs because
   //  o A thread is only interrupted if it is sleeping (or running), not
   //    if it is waiting on a stream or socket.  Nonetheless, the thread
   //    may want to react to this interrupt at its next opportunity.
   //  o If SysThread.Interrupt fails, the thread can still react to the
   //    interrupt as soon as it checks its vector.
   //  o If SysThread.Interrupt succeeds, the thread may run immediately,
   //    before this function returns, in which case its vector must have
   //    already been updated.
   //
   auto reason = blocked_;
   auto bits = mask.to_ulong();
   priv_->vector_.fetch_or(bits);

   if((reason == NotBlocked) || (reason == BlockedOnClock))
   {
      if((systhrd_ != nullptr) && systhrd_->Interrupt())
      {
         ThreadAdmin::Incr(ThreadAdmin::Interrupts);
         stats_->interrupts_->Incr();
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Thread_IsCritical = "Thread.IsCritical";

bool Thread::IsCritical() const
{
   Debug::ft(Thread_IsCritical);

   return true;
}

//------------------------------------------------------------------------------

bool Thread::IsLocked() const
{
   return ((priv_ != nullptr) && (priv_->unpreempts_ > 0));
}

//------------------------------------------------------------------------------

bool Thread::IsReady() const
{
   return ((blocked_ == NotBlocked) && (faction_ < SystemFaction) && !deleted_);
}

//------------------------------------------------------------------------------

bool Thread::IsTraceable() const
{
   //  Do not trace a thread that has been explicitly excluded.  Trace
   //  RootThread and InitThread during system initialization and when
   //  explicitly included.  Trace other threads when included.
   //
   auto trace = CalcStatus(true);
   if(trace == TraceExcluded) return false;

   switch(faction_)
   {
   case WatchdogFaction:
   case SystemFaction:
      return ((Restart::GetStatus() != Running) || (status_ == TraceIncluded));
   }

   return (trace == TraceIncluded);
}

//------------------------------------------------------------------------------

Thread* Thread::LockedThread()
{
   auto thr = ActiveThread();
   if((thr != nullptr) && thr->IsLocked()) return thr;
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Thread_LogContextSwitch = "Thread.LogContextSwitch";

void Thread::LogContextSwitch() const
{
   Debug::ft(Thread_LogContextSwitch);

   ThreadAdmin::Incr(ThreadAdmin::Switches);

   auto tid = Tid();
   auto reg = Singleton< ThreadRegistry >::Instance();
   auto now = Clock::TicksNow();
   auto locked = IsLocked();

   if(reg->Threads().At(tid) == nullptr)
   {
      //  This thread is unknown, probably because it was deleted
      //  and is about to exit.  Create a partial entry for it.
      //
      auto rec = Singleton< ContextSwitches >::Instance()->AddSwitch();

      if(rec != nullptr)
      {
         rec->tid = 0;
         rec->nid = SysThread::RunningThreadId();
         rec->in = now;
         rec->out = now;
         rec->locked = locked;
         rec->overlap = false;
      }
   }
   else
   {
      if(stats_ != nullptr)
      {
         stats_->yields_->Incr();
         auto ticks = now - priv_->currStart_;
         auto usecs = Clock::TicksToUsecs(ticks);
         stats_->maxUsecs_->Update(usecs);
         stats_->totUsecs_->Add(usecs);
         priv_->currUsecs_ += usecs;
      }

      auto rec = Singleton< ContextSwitches >::Instance()->AddSwitch();

      if(rec != nullptr)
      {
         rec->tid = tid;
         rec->nid = SysThread::RunningThreadId();
         rec->in = priv_->currStart_;
         rec->out = now;
         rec->locked = locked;
         rec->overlap = false;
      }
   }
}

//------------------------------------------------------------------------------

TraceRc Thread::LogContextSwitches(bool on)
{
   return Singleton< ContextSwitches >::Instance()->LogSwitches(on);
}

//------------------------------------------------------------------------------

fn_name Thread_LogSignal = "Thread.LogSignal";

bool Thread::LogSignal(signal_t sig) const
{
   Debug::ft(Thread_LogSignal);

   //  Don't log
   //  o a subsequent SIGYIELD if traps on SIGYIELD are disabled;
   //  o an exit that is voluntary (SIGNIL);
   //  o a signal that is not associated with an error.
   //
   if((sig == SIGYIELD) && (priv_->warned_)) return false;
   if(sig == SIGNIL) return false;
   auto reg = Singleton< PosixSignalRegistry >::Instance();
   return (!reg->Attrs(sig).test(PosixSignal::NoLog));
}

//------------------------------------------------------------------------------

fn_name Thread_MakePreemptable = "Thread.MakePreemptable";

void Thread::MakePreemptable()
{
   Debug::ft(Thread_MakePreemptable);

   auto thr = RunningThread();

   //  Check for underflow.  If the thread has just become preemptable,
   //  schedule it out.
   //
   if(thr->priv_->unpreempts_ == 0)
   {
      Debug::SwLog(Thread_MakePreemptable, "underflow", thr->Tid());
      return;
   }

   if(--thr->priv_->unpreempts_ == 0) Pause();
}

//------------------------------------------------------------------------------

fn_name Thread_MakeUnpreemptable = "Thread.MakeUnpreemptable";

void Thread::MakeUnpreemptable()
{
   Debug::ft(Thread_MakeUnpreemptable);

   auto thr = RunningThread();

   //  Increment the unpreemptable count.  If the thread has just become
   //  unpreemptable, schedule it out before starting to run it locked.
   //
   if(thr->priv_->unpreempts_ >= 0x0f)
   {
      Debug::SwLog(Thread_MakeUnpreemptable, "overflow", thr->Tid());
      return;
   }

   if(++thr->priv_->unpreempts_ == 1) Pause();
}

//------------------------------------------------------------------------------

fn_name Thread_MemProtect = "Thread.MemProtect";

void Thread::MemProtect()
{
   Debug::ft(Thread_MemProtect);

   auto thr = RunningThread();

   //e Write-disable the protected memory segment.

   if(thr->priv_->unprotects_ == 0)
   {
      Debug::SwLog(Thread_MemProtect, "underflow", thr->Tid());
      return;
   }

   --thr->priv_->unprotects_;
}

//------------------------------------------------------------------------------

fn_name Thread_MemUnprotect = "Thread.MemUnprotect";

void Thread::MemUnprotect()
{
   Debug::ft(Thread_MemUnprotect);

   auto thr = RunningThread();

   //e Write-enable the protected memory segment.

   if(thr->priv_->unprotects_ >= 0x0f)
   {
      Debug::SwLog(Thread_MemUnprotect, "overflow", thr->Tid());
      return;
   }

   ++thr->priv_->unprotects_;
}

//------------------------------------------------------------------------------

fn_name Thread_MsecsSinceStart = "Thread.MsecsSinceStart";

msecs_t Thread::MsecsSinceStart() const
{
   Debug::ft(Thread_MsecsSinceStart);

   if(priv_->currStart_ == 0) return 0;
   auto runTime = Clock::TicksSince(priv_->currStart_);
   return Clock::TicksToMsecs(runTime);
}

//------------------------------------------------------------------------------

SysThreadId Thread::NativeThreadId() const
{
   if(deleted_) return NIL_ID;
   if(systhrd_ != nullptr) return systhrd_->Nid();
   return NIL_ID;
}

//------------------------------------------------------------------------------

fn_name Thread_new = "Thread.operator new";

void* Thread::operator new(size_t size)
{
   Debug::ft(Thread_new);

   return Singleton< ThreadPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void Thread::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Thread_Pause = "Thread.Pause";

DelayRc Thread::Pause(msecs_t msecs)
{
   Trace(nullptr, Thread_Pause, ThreadTrace::PauseEnter, msecs);

   auto drc = DelayCompleted;
   auto thr = RunningThread();

   //  See if the thread should be forced to sleep indefinitely.  This occurs
   //  o during the execution of Unblock(), which could be deleting some of
   //    the thread's resources;
   //  o when the thread decided to survive a restart instead of exiting.
   //
   if(thr->priv_->action_ == SleepThread)
   {
      msecs = TIMEOUT_NEVER;
   }

   EnterBlockingOperation(BlockedOnClock, Thread_Pause);
   {
      if(msecs != TIMEOUT_IMMED) drc = thr->systhrd_->Delay(msecs);
   }
   ExitBlockingOperation(Thread_Pause);

   Trace(thr, Thread_Pause, ThreadTrace::PauseExit, drc);
   return drc;
}

//------------------------------------------------------------------------------

fn_name Thread_PauseOver = "Thread.PauseOver";

void Thread::PauseOver(word limit)
{
   Debug::ft(Thread_PauseOver);

   if(RtcPercentUsed() >= limit) Pause();
}

//------------------------------------------------------------------------------

double Thread::PercentIdle()
{
   if(ThreadPriv::TimeIdle_ == 0) return 0.0;

   return 100 * (double(ThreadPriv::TimeIdle_) /
      (ThreadPriv::TimeIdle_ + ThreadPriv::TimeUsed_));
}

//------------------------------------------------------------------------------

fn_name Thread_Preempt = "Thread.Preempt";

void Thread::Preempt()
{
   Debug::ft(Thread_Preempt);

   //  Set the thread's ready time so that it will later be reselected,
   //  and lower its priority so that the platform won't schedule it in.
   //
   readyTime_ = Clock::TicksNow();
   systhrd_->SetPriority(SysThread::LowPriority);
   ThreadAdmin::Incr(ThreadAdmin::Preempts);
}

//------------------------------------------------------------------------------

fn_name Thread_Proceed = "Thread.Proceed";

void Thread::Proceed()
{
   Debug::ft(Thread_Proceed);

   //  Now that the thread has been scheduled, clear its ready time and
   //  ensure that its priority is such that the platform will schedule
   //  it in.  Signal it to resume.
   //
   readyTime_ = UINT64_MAX;
   systhrd_->SetPriority(SysThread::DefaultPriority);
   if(waiting_) systhrd_->Proceed();
}

//------------------------------------------------------------------------------

fn_name Thread_Raise = "Thread.Raise";

void Thread::Raise(signal_t sig)
{
   Debug::ft(Thread_Raise);

   //  Ensure that SIG is valid.
   //
   auto reg = Singleton< PosixSignalRegistry >::Instance();
   auto ps1 = reg->Find(sig);

   if(ps1 == nullptr)
   {
      Debug::SwLog(Thread_Raise, "unexpected signal", sig);
      return;
   }

   //  If the thread was not fully constructed, simply destroy it.
   //
   if(NativeThreadId() == NIL_ID)
   {
      Destroy();
      return;
   }

   //  If this is the running thread, throw the signal immediately.  If the
   //  running thread can't be found, don't assert: the signal handler can
   //  invoke this when a signal occurs on an unknown thread.
   //
   auto thr = RunningThread(false);

   if(thr == this)
   {
      throw SignalException(sig, 0);
   }

   //  This is not the running thread.  Verify that it is legal to raise
   //  the signal for another thread.
   //
   if(ps1->Severity() == 0)
   {
      Debug::SwLog(Thread_Raise, "invalid signal", sig);
      return;
   }

   //  If the target thread already has a signal pending (PS0), install
   //  the new one (PS1) only if it is more severe.  Even if the new one
   //  is not installed, apply its exit, trap, and interrupt attributes.
   //
   auto install = true;

   if(priv_->signal_ != SIGNIL)
   {
      auto ps0 = reg->Find(priv_->signal_);

      if(ps0 != nullptr)
         install = (ps1->Severity() > ps0->Severity());
      else
         Debug::SwLog(Thread_Raise, "signal not found", priv_->signal_);
   }

   //  If the signal will force the thread to exit, try to unblock it.
   //  Unblocking usually involves deallocating resources, so force the
   //  thread to sleep if it wakes up during Unblock().
   //
   if(ps1->Attrs().test(PosixSignal::Exit))
   {
      if(priv_->action_ == RunThread)
      {
         priv_->action_ = SleepThread;
         Unblock();
         priv_->action_ = ExitThread;
      }
   }

   //  Most signals are logged.
   //
   if(install && LogSignal(sig))
   {
      auto log = Log::Create(ThreadLogGroup, ThreadSignalRaised);

      if(log != nullptr)
      {
         if(thr != nullptr) *log << Log::Tab << "by " << thr->to_str() << CRLF;
         *log << Log::Tab << "for " << this->to_str() << CRLF;
         *log << Log::Tab << "signal=" << reg->strSignal(sig);
         Log::Submit(log);
      }
   }

   //  If a thread is being signalled for running unpreemptably too long,
   //  check that it is actually locked, and make do with the log if it
   //  is not to be trapped.
   //
   if(sig == SIGYIELD)
   {
      if(!IsLocked()) return;

      if(!ThreadAdmin::TrapOnRtcTimeout())
      {
         priv_->warned_ = true;
         return;
      }
   }

   if(install) SetSignal(sig);
   if(!ps1->Attrs().test(PosixSignal::Delayed)) SetTrap(true);
   if(ps1->Attrs().test(PosixSignal::Interrupt)) Interrupt();
}

//------------------------------------------------------------------------------

fn_name Thread_Ready = "Thread.Ready";

void Thread::Ready()
{
   Debug::ft(Thread_Ready);

   if(faction_ >= SystemFaction) return;

   //  Record the time when the thread became ready to run.  If no thread
   //  is currently active, wake InitThread to schedule this thread in,
   //  but have it wait to be signalled before it runs.
   //
   readyTime_ = Clock::TicksNow();
   waiting_ = true;

   if(ActiveThread() == nullptr)
   {
      Singleton< InitThread >::Instance()->Interrupt(InitThread::ScheduleMask);
   }

   systhrd_->Wait();
   waiting_ = false;
}

//------------------------------------------------------------------------------

fn_name Thread_Recover = "Thread.Recover";

Thread::RecoveryAction Thread::Recover()
{
   Debug::ft(Thread_Recover);

   return ReenterThread;
}

//------------------------------------------------------------------------------

fn_name Thread_Recreate = "Thread.Recreate";

bool Thread::Recreate()
{
   Debug::ft(Thread_Recreate);

   //  This function creates a new native thread, so one shouldn't exist.
   //  It is also intended only for critical threads.
   //
   if(systhrd_ != nullptr)
   {
      Debug::SwLog(Thread_Recreate,
         "SysThread already exists", pack2(faction_, Tid()));
      return true;
   }

   if(!IsCritical())
   {
      Debug::SwLog(Thread_Recreate,
         "thread is not critical", pack2(faction_, Tid()));
      return true;
   }

   //  Reset a subset of the thread's data.
   //
// systhrd_  = nullptr;
// tid_ = (preserved)
// faction_ = (preserved)
   blocked_ = NotBlocked;
// status_ = (preserved)
   traceMsg_ = false;
   deleted_ = false;
   trapped_ = false;
// msgq_ = (preserved)
// priv_ = (preserved)
// stats_ = (preserved)

   priv_->Recreate();
   SetTrap(false);

   //  Before creating the native thread, notify the thread that it is
   //  being recreated.
   //
   Recreated();

   //  Create the native thread and associate the thread's identifier with
   //  its new native thread identifier.
   //
   auto prio = FactionToPriority(faction_);
   systhrd_.reset(new SysThread(this, EnterThread, prio, 0));
   if(systhrd_ == nullptr) return false;

   Singleton< ThreadRegistry >::Instance()->AssociateIds(*this);

   if(systhrd_->status_.any())
   {
      Debug::SwLog(Thread_Recreate, systhrd_->status_.to_string(), Tid());
   }

   ThreadAdmin::Incr(ThreadAdmin::Recreations);
   return true;
}

//------------------------------------------------------------------------------

fn_name Thread_Recreated = "Thread.Recreated";

void Thread::Recreated()
{
   Debug::ft(Thread_Recreated);
}

//------------------------------------------------------------------------------

fn_name Thread_RegisterForSignals = "Thread.RegisterForSignals";

void Thread::RegisterForSignals()
{
   Debug::ft(Thread_RegisterForSignals);

   auto& signals = Singleton< PosixSignalRegistry >::Instance()->Signals();

   for(auto s = signals.First(); s != nullptr; signals.Next(s))
   {
      if(s->Attrs().test(PosixSignal::Native))
      {
         SysThread::RegisterForSignal(s->Value(), SignalHandler);
      }
   }
}

//------------------------------------------------------------------------------

fn_name Thread_ReleaseResources = "Thread.ReleaseResources";

void Thread::ReleaseResources(bool orphaned)
{
   Debug::ft(Thread_ReleaseResources);

   //  Setting deleted_ prevents most functions from accessing the thread
   //  while it is being deleted.
   //
   if(deleted_) return;
   deleted_ = true;

   //  Clear the thread's ready time so that it won't be scheduled.  It
   //  can no longer be the active thread.
   //
   readyTime_ = UINT64_MAX;
   ClearActiveThread(this);

   //  Remove the thread from the registry after voiding its message queue.
   //  The thread may have trapped because of a corrupt message queue, so
   //  let the object pool audit recover any messages queued against it.
   //
   msgq_.Init(Pooled::LinkDiff());
   Singleton< ThreadRegistry >::Instance()->UnbindThread(*this);

   //  If a restart is underway, release any object that the thread owns
   //  and whose heap will be deleted.
   //
   if(Restart::GetLevel() >= RestartCold)
   {
      stats_.release();
   }

   //  If the thread is not orphaned, it is about to exit, so delete its
   //  native thread; otherwise, add its native thread to set of orphans.
   //  Various functions invoke ExitNow to check for the existence of an
   //  orphaned native thread, which is immediately exited when found.
   //
   if(orphaned)
      Singleton< Orphans >::Instance()->Register(systhrd_.release());
   else
      systhrd_.reset();
}

//------------------------------------------------------------------------------

fn_name Thread_Reset = "Thread.Reset";

void Thread::Reset(FlagId fid)
{
   Debug::ft(Thread_Reset);

   uint32_t mask = (1 << fid);
   priv_->vector_.fetch_and(~mask);
}

//------------------------------------------------------------------------------

fn_name Thread_ResetFlag = "Thread.ResetFlag";

void Thread::ResetFlag(FlagId fid)
{
   Debug::ft(Thread_ResetFlag);

   RunningThread()->Reset(fid);
}

//------------------------------------------------------------------------------

fn_name Thread_ResetFlags = "Thread.ResetFlags";

void Thread::ResetFlags()
{
   Debug::ft(Thread_ResetFlags);

   RunningThread()->priv_->vector_.store(0);
}

//------------------------------------------------------------------------------

fn_name Thread_Restarting = "Thread.Restarting";

bool Thread::Restarting(RestartLevel level)
{
   Debug::ft(Thread_Restarting);

   //  If the thread is willing to exit, signal it.  ModuleRegistry.Shutdown
   //  will momentarily schedule it so that it can exit.
   //
   if(ExitOnRestart(level))
   {
      Raise(SIGCLOSE);
      return true;
   }

   //  Unless this is RootThread or InitThread, mark it as a survivor.  This
   //  causes various functions to force it to sleep until the restart ends.
   //
   if(faction_ < SystemFaction) priv_->action_ = SleepThread;
   return false;
}

//------------------------------------------------------------------------------

fn_name Thread_RestrictFactions = "Thread.RestrictFactions";

void Thread::RestrictFactions(bool enable)
{
   Debug::ft(Thread_RestrictFactions);

   FactionsRestricted_ = enable;
}

//------------------------------------------------------------------------------

fn_name Thread_Resume = "Thread.Resume";

void Thread::Resume(fn_name_arg func)
{
   Debug::ft(Thread_Resume);

   //  Set the time before which a locked thread should schedule itself out.
   //
   priv_->currStart_ = Clock::TicksNow();
   auto msecs = InitialMsecs() << ThreadAdmin::WarpFactor();
   if(!priv_->entered_) msecs <<= 2;
   priv_->currEnd_ = priv_->currStart_ + Clock::MsecsToTicks(msecs);
   priv_->warned_ = false;

   if(priv_->unpreempts_ > 0) ThreadAdmin::Incr(ThreadAdmin::Locks);
   ScheduledIn(func);
}

//------------------------------------------------------------------------------

fn_name Thread_RtcPercentUsed = "Thread.RtcPercentUsed";

word Thread::RtcPercentUsed()
{
   Debug::ft(Thread_RtcPercentUsed);

   //  This returns 0 unless the thread is running unpreemptably.
   //
   auto thr = RunningThread();
   if((thr == nullptr) || !thr->IsLocked()) return 0;

   auto used = Clock::TicksSince(thr->priv_->currStart_);
   auto full = thr->priv_->currEnd_ - thr->priv_->currStart_;

   if(used < full) return (100 * used) / full;
   return 100;
}

//------------------------------------------------------------------------------

fn_name Thread_RtcTimeout = "Thread.RtcTimeout";

void Thread::RtcTimeout()
{
   Debug::ft(Thread_RtcTimeout);

   if(stats_ != nullptr) stats_->exceeds_->Incr();

   if(priv_->rtcLbc_.HasReachedLimit())
   {
      Raise(SIGYIELD);
   }
}

//------------------------------------------------------------------------------

fn_name Thread_RunningThread = "Thread.RunningThread";

Thread* Thread::RunningThread(bool assert)
{
   Debug::ft(Thread_RunningThread);

   //  The running thread is usually the active thread.  If it isn't,
   //  search the thread registry.
   //
   auto nid = SysThread::RunningThreadId();
   Thread* thr = nullptr;
   auto active = ActiveThread();

   if((active != nullptr) && (active->NativeThreadId() == nid))
      thr = active;
   else
      thr = Singleton< ThreadRegistry >::Instance()->FindThread(nid);

   if((thr != nullptr) && !thr->deleted_) return thr;

   //  The thread could not be found.  This can occur for various reasons:
   //  o The system has just started to run, and not even RootThread has
   //    been created to wrap main().
   //  o The thread is undergoing deletion and has been removed from the
   //    thread registry.  It shouldn't be calling this itself, but trace
   //    tools will, which is why they use assert=false.
   //  o The thread is an orphan: its Thread object has been deleted.  This
   //    should not occur, but if it does, the thread must exit immediately.
   //
   ThreadAdmin::Incr(ThreadAdmin::Unknowns);

   if(assert)
   {
      if(Singleton< Orphans >::Instance()->ExitNow())
         throw SignalException(SIGDELETED, 0);
      else
         Debug::Assert(false);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Thread_Schedule = "Thread.Schedule";

void Thread::Schedule() const
{
   Debug::ft(Thread_Schedule);

   //  Scheduling only occurs among application threads.
   //
   if(faction_ >= SystemFaction) return;

   auto active = ActiveThread();

   if((active != this) && (active != nullptr))
   {
      //  We get here after a preemptable thread suspends or invokes
      //  MakeUnpreemptable.  If an unpreemptable thread is running,
      //  don't try to schedule another thread.
      //
      return;
   }

   //  No unpreemptable thread is running.  Wake InitThread to schedule
   //  the next thread.
   //
   ActiveThread_ = nullptr;
   Singleton< InitThread >::Instance()->Interrupt(InitThread::ScheduleMask);
}

//------------------------------------------------------------------------------

fn_name Thread_Select = "Thread.Select";

Thread* Thread::Select()
{
   Debug::ft(Thread_Select);

   //  Cycle through all threads, beginning with the one identified by
   //  start_, to find the next one that is ready to run.
   //
   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();
   auto first = threads.First(Start_);
   Thread* next = nullptr;

   for(auto t = first; t != nullptr; threads.Next(t))
   {
      if(t->IsReady() && FactionAllowed(t->faction_))
      {
         next = t;
         break;
      }
   }

   if(next == nullptr)
   {
      for(auto t = threads.First(); t != first; threads.Next(t))
      {
         if(t->IsReady() && FactionAllowed(t->faction_))
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
      Start_ = (t != nullptr ? t->Tid() : 1);
   }

   return next;
}

//------------------------------------------------------------------------------

fn_name Thread_SetSignal = "Thread.SetSignal";

void Thread::SetSignal(signal_t sig)
{
   Debug::ft(Thread_SetSignal);

   priv_->signal_ = sig;
}

//------------------------------------------------------------------------------

fn_name Thread_SetTrap = "Thread.SetTrap";

void Thread::SetTrap(bool on)
{
   Debug::ft(Thread_SetTrap);

   if(on)
   {
      //  Set the trap_ flag and the global TrapPending flag.
      //
      priv_->trap_ = true;
      Debug::FcFlags_.set(Debug::TrapPending);
      return;
   }

   if(priv_->trap_)
   {
      //  Clear the flag.  If no more trap requests are pending,
      //  clear the global TrapPending flag.
      //
      priv_->trap_ = false;

      auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

      for(auto t = threads.First(); t != nullptr; threads.Next(t))
      {
         if(t->priv_->trap_) return;
      }

      Debug::FcFlags_.reset(Debug::TrapPending);
   }
}

//------------------------------------------------------------------------------

fn_name Thread_Shutdown = "Thread.Shutdown";

void Thread::Shutdown(RestartLevel level)
{
   Debug::ft(Thread_Shutdown);

   if(level < RestartCold) return;

   //  On a cold restart or higher, the thread's messages and statistics
   //  will be deleted.  Clean up the messages in case they own objects
   //  that they need to free, and then reinitialize the message queue
   //  so that the destructor will not be invoked for each message.
   //
   for(auto m = msgq_.First(); m != nullptr; msgq_.Next(m))
   {
      m->Cleanup();
   }

   msgq_.Init(Pooled::LinkDiff());
   stats_.release();
}

//------------------------------------------------------------------------------

fn_name Thread_SignalHandler = "Thread.SignalHandler";

void Thread::SignalHandler(signal_t sig)
{
   //  Reenable Debug functions before tracing this function.
   //
   Debug::Reset();
   Debug::ft(Thread_SignalHandler);

   //  Re-register for signals before handling the signal.
   //
   RegisterForSignals();
   if(HandleSignal(sig, 0)) return;

   //  Either trap recovery is off or we received a signal that could not be
   //  associated with a thread.  Generate a log before restoring the default
   //  handler for the signal and reraising it (to enter the debugger, for
   //  example).
   //
   auto log = Log::Create(ThreadLogGroup, ThreadSignalReraised);

   if(log != nullptr)
   {
      auto reg = Singleton< PosixSignalRegistry >::Instance();
      *log << Log::Tab << "signal=" << reg->strSignal(sig);
      Log::Submit(log);
   }

   Pause(2 * TIMEOUT_1_SEC);
   signal(sig, SIG_DFL);
   raise(sig);
}

//------------------------------------------------------------------------------

void Thread::StackCheck()
{
   //  Return immediately if stackBase_ has not been initialized.
   //
   if((priv_ == nullptr) || (priv_->stackBase_ == nullptr)) return;

   ThreadPriv::StackCheckCounter_ = ThreadAdmin::StackCheckInterval();

   signal_t local = SIGNIL;
   ptrdiff_t stacksize = &local - priv_->stackBase_;
   if(stacksize < 0) stacksize = -stacksize;

   if(stacksize > ThreadAdmin::StackUsageLimit())
   {
      priv_->stackBase_ = nullptr;
      throw SignalException(SIGSTACK1, stacksize);
   }

   if(stats_ != nullptr) stats_->maxStack_->Update(stacksize);
}

//------------------------------------------------------------------------------

fn_name Thread_Start = "Thread.Start";

main_t Thread::Start()
{
   for(NO_OP; true; stats_->traps_->Incr())
   {
      try
      {
         Debug::ft(Thread_Start);

         //  If the thread is preemptable, we got here after handling a trap,
         //  not from Enter (which makes a thread unpreemptable).  Make the
         //  thread unpreemptable again.
         //
         if(priv_->unpreempts_ == 0) MakeUnpreemptable();

         //  Perform any environment-specific initialization (and recovery,
         //  if reentering the thread).  Exit the thread if this fails.
         //
         auto rc = systhrd_->Start();
         if(rc != 0) return Exit(rc);

         //  Save the approximate value of the thread's stack pointer.
         //
         priv_->stackBase_ = &rc;

         if(trapped_)
         {
            //  The thread just trapped.  In most cases, invoke Recover
            //  so that the thread can clean up work in progress.
            //
            auto action = DeleteThread;
            auto reg = Singleton< PosixSignalRegistry >::Instance();

            if(reg->Attrs(priv_->signal_).test(PosixSignal::NoRecover))
            {
               //  Recovery is not allowed after this signal.  If the
               //  thread is critical, recreate it, else delete it.
               //
               if(IsCritical()) action = RecreateThread;
            }
            else
            {
               //  Pause before error recovery.  Don't force the thread
               //  to exit if Recover traps, because the problem might
               //  be limited to the objects used during the last pass
               //  through the thread's work loop.
               //
               Pause();
               priv_->recovering_ = true;
               trapped_ = false;
               action = Recover();
               trapped_ = true;
               priv_->recovering_ = false;
            }

            switch(action)
            {
            case DeleteThread:
               //
               //  The thread is not critical or did not want to be reentered,
               //  so exit.
               //
               return Exit(SIGNIL);

            case ReenterThread:
               //
               //  Recover is still invoked when a thread reaches the trap
               //  limit, and the thread may ask to be reentered.  However,
               //  it is deleted and recreated instead (by the absence of a
               //  break statement here).  In all other cases, the thread is
               //  simply reentered by looping to the top of this function.
               //
               if(priv_->signal_ != SIGTRAPS)
               {
                  SetSignal(SIGNIL);
                  ThreadAdmin::Incr(ThreadAdmin::Recoveries);
                  break;
               }
               //  [[fallthrough]]
            case RecreateThread:
               //
               //  The thread must exit before it can be recreated.
               //
               return Exit(priv_->signal_);

            default:
               Restart::Initiate(DeathOfCriticalThread, Tid());
            }

            //  Pause after error recovery.
            //
            Pause();
            trapped_ = false;
         }

         //  Invoke the thread's entry function.  If this returns,
         //  the thread exited voluntarily.
         //
         priv_->entered_ = true;
         Enter();
         return Exit(SIGNIL);
      }

      catch(ElementException& nex)
      {
         auto reason = nex.Reason();
         auto code = nex.Errval();

         if((reason == ManualRestart) &&
            (code == RestartExit) && Element::RunningInLab())
         {
            //  This shuts the system down.  Wait for 10 seconds
            //  instead of letting the console suddenly vanish.
            //
            CoutThread::Spool(ClosingConsoleStr, true);
            Pause(10 * TIMEOUT_1_SEC);
            Suspend();
            exit(reason);
         }

         auto log = Log::Create(NodeLogGroup, NodeRestart);

         if(log != nullptr)
         {
            *log << Log::Tab << "in " << to_str() << CRLF;
            nex.Display(*log, Log::Tab + spaces(2));
            *log << Log::Tab << nex.Stack()->str();
            Log::Submit(log);
         }

         //  RootThread and InitThread handle their own flow of execution when
         //  initiating restarts, so just loop around and reinvoke their Enter
         //  functions.  Other threads must first notify InitThread.
         //
         if(faction_ < SystemFaction)
         {
            auto system = Singleton< InitThread >::Instance();
            system->InitiateRestart(reason, code);
         }

         continue;
      }

      //  For any other exception, attempt recovery.
      //
      catch(SignalException& sex)
      {
         switch(TrapHandler(&sex, &sex, sex.GetSignal(), sex.Stack()))
         {
         case Continue:
            continue;
         case Release:
            return Exit(sex.GetSignal());
         case Return:
            return sex.GetSignal();
         default:
            throw;
         }
      }

      catch(Exception& ex)
      {
         switch(TrapHandler(&ex, &ex, SIGNIL, ex.Stack()))
         {
         case Continue:
            continue;
         case Release:
            return Exit(SIGNIL);
         case Return:
            return SIGDELETED;
         default:
            throw;
         }
      }

      catch(std::exception& e)
      {
         switch(TrapHandler(nullptr, &e, SIGNIL, nullptr))
         {
         case Continue:
            continue;
         case Release:
            return Exit(SIGNIL);
         case Return:
            return SIGDELETED;
         default:
            throw;
         }
      }

      catch(...)
      {
         switch(TrapHandler(nullptr, nullptr, SIGNIL, nullptr))
         {
         case Continue:
            continue;
         case Release:
            return Exit(SIGNIL);
         case Return:
            return SIGDELETED;
         default:
            throw;
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name Thread_StartShortInterval = "Thread.StartShortInterval";

void Thread::StartShortInterval()
{
   Debug::ft(Thread_StartShortInterval);

   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

   ThreadPriv::TimeUsed_ = 0;

   for(auto t = threads.First(); t != nullptr; threads.Next(t))
   {
      ThreadPriv::TimeUsed_ += t->priv_->currUsecs_;
      t->priv_->prevUsecs_ = t->priv_->currUsecs_;
      t->priv_->currUsecs_ = 0;
   }

   ThreadPriv::PrevIntervalStart_ = ThreadPriv::CurrIntervalStart_;
   ThreadPriv::CurrIntervalStart_ = Clock::TicksNow();

   //  Until the first short interval ends, there is no "previous" short
   //  interval.
   //
   if(ThreadPriv::PrevIntervalStart_ > 0)
   {
      auto ticks =
         ThreadPriv::CurrIntervalStart_ - ThreadPriv::PrevIntervalStart_;
      auto total = Clock::TicksToUsecs(ticks);

      if(total > ThreadPriv::TimeUsed_)
         ThreadPriv::TimeIdle_ = total - ThreadPriv::TimeUsed_;
      else
         ThreadPriv::TimeIdle_ = 0;
   }
}

//------------------------------------------------------------------------------

TraceRc Thread::StartTracing(bool immediate, bool autostop)
{
   auto thr = RunningThread();
   auto rc = Singleton< TraceBuffer >::Instance()->StartTracing(immediate);

   if(rc == TraceOk)
   {
      thr->priv_->tracing_ = true;
      thr->priv_->autostop_ = autostop;
   }

   return rc;
}

//------------------------------------------------------------------------------

fn_name Thread_Startup = "Thread.Startup";

void Thread::Startup(RestartLevel level)
{
   Debug::ft(Thread_Startup);

   //  Recreate the thread's statistics if they were deleted.  If the
   //  thread slept during the restart, wake it up.
   //
   if(stats_ == nullptr) stats_.reset(new ThreadStats);

   auto wakeup = (priv_->action_ == SleepThread);
   priv_->action_ = RunThread;
   if(wakeup && (blocked_ == BlockedOnClock)) Interrupt();
}

//------------------------------------------------------------------------------

void Thread::StopTracing()
{
   auto thr = RunningThread();

   if(thr->priv_->tracing_)
   {
      Singleton< TraceBuffer >::Instance()->StopTracing();
      thr->priv_->tracing_ = false;
      thr->priv_->autostop_ = false;
   }
}

//------------------------------------------------------------------------------

fn_name Thread_Suspend = "Thread.Suspend";

void Thread::Suspend()
{
   Debug::ft(Thread_Suspend);

   if(priv_->autostop_) StopTracing();

   if(priv_->warned_)
   {
      auto log = Log::Create(ThreadLogGroup, ThreadYielded);

      if(log != nullptr)
      {
         *log << Log::Tab << "thread=" << to_str();
         auto ticks = Clock::TicksSince(priv_->currEnd_);
         auto msecs = Clock::TicksToMsecs(ticks);
         *log << " extra msecs=" << msecs;
         Log::Submit(log);
      }

      priv_->warned_ = false;
   }

   LogContextSwitch();
   priv_->currEnd_ = 0;
   Schedule();
}

//------------------------------------------------------------------------------

fn_name Thread_SwitchContext = "Thread.SwitchContext";

bool Thread::SwitchContext()
{
   Debug::ft(Thread_SwitchContext);

   auto curr = ActiveThread();

   if((curr != nullptr) && curr->IsLocked())
   {
      //  We were invoked to schedule a thread and started to do so, but
      //  before ActiveThread_ was set (below), we were invoked again.
      //
      ThreadAdmin::Incr(ThreadAdmin::Reentries);
      return true;
   }

   //  Select the next thread to run.  If one is found, preempt any running
   //  thread and signal the next one to resume.
   //
   auto next = Select();

   if(next != nullptr)
   {
      if(next == curr)
      {
         ThreadAdmin::Incr(ThreadAdmin::Reselects);
         return true;
      }

      if(curr != nullptr) curr->Preempt();
      ActiveThread_ = next;
      next->Proceed();
      return true;
   }

   return (curr != nullptr);
}

//------------------------------------------------------------------------------

fn_name Thread_Test = "Thread.Test";

bool Thread::Test(FlagId fid) const
{
   Debug::ft(Thread_Test);

   auto flags = priv_->vector_.load();
   return ((flags & (1 << fid)) != 0);
}

//------------------------------------------------------------------------------

fn_name Thread_TestFlag = "Thread.TestFlag";

bool Thread::TestFlag(FlagId fid)
{
   Debug::ft(Thread_TestFlag);

   return RunningThread()->Test(fid);
}

//------------------------------------------------------------------------------

fn_name Thread_TicksLeft = "Thread.TicksLeft";

ticks_t Thread::TicksLeft() const
{
   Debug::ft(Thread_TicksLeft);

   //  currEnd_ is zeroed just before yielding.  This prevents its previous
   //  value from being used during the brief interval in which the thread
   //  has again been scheduled to run unpreemptably but currEnd_ has not
   //  been recalculated.
   //
   if(priv_->currEnd_ == 0) return Clock::MsecsToTicks(InitialMsecs());
   return Clock::TicksUntil(priv_->currEnd_);
}

//------------------------------------------------------------------------------

string Thread::to_str() const
{
   std::ostringstream stream;

   stream << strClass(this);
   stream << " (tid=" << Tid();
   stream << ", nid=";
   if(systhrd_ != nullptr)
      stream << strHex(systhrd_->Nid(), 0);
   else
      stream << "none";
   stream << ')';

   return stream.str();
}

//------------------------------------------------------------------------------

void Thread::Trace(Thread* thr, fn_name_arg func, TraceRecordId rid, word info)
{
   if(thr == nullptr) thr = RunningThread(false);
   if(thr == nullptr) return;

   if(Debug::FcFlags_.test(Debug::TracingActive))
   {
      if(TraceRunningThread(thr))
      {
         ThreadTrace::CaptureEvent(func, rid, info);
      }
   }
}

//------------------------------------------------------------------------------

bool Thread::TraceRunningThread(Thread*& thr)
{
   //  Do not trace this thread if the trace buffer is locked or
   //  function tracing is not on.
   //
   auto buff = Singleton< TraceBuffer >::Instance();
   if(buff->IsLocked()) return false;
   if(!buff->ToolIsOn(FunctionTracer)) return false;

   //  If the running thread is unknown, find it while taking care not
   //  to create the thread registry prematurely during initialization.
   //  If the registry does not yet exist, we must be tracing the root
   //  thread during initialization.  Other cases in which the running
   //  thread cannot be found often involve error scenarios, so include
   //  them.
   //
   if(thr == nullptr)
   {
      auto reg = Singleton< ThreadRegistry >::Extant();
      if(reg == nullptr) return true;
      thr = RunningThread(false);
      if(thr == nullptr) return true;
   }

   return thr->IsTraceable();
}

//------------------------------------------------------------------------------

void Thread::TrapCheck()
{
   //  Wait to trap a thread if it has yet to be entered.
   //
   if((priv_ == nullptr) || !priv_->trap_ || !priv_->entered_) return;
   ExitIfSafe(3);
}

//------------------------------------------------------------------------------

fn_name Thread_TrapHandler = "Thread.TrapHandler";

Thread::TrapAction Thread::TrapHandler(const Exception* ex,
   const std::exception* e, signal_t sig, const std::ostringstream* stack)
{
   Debug::ft(Thread_TrapHandler);  //@

   //  Exit immediately if the Thread has already been deleted.
   //
   if((sig == SIGDELETED) || IsInvalid() ||
      Singleton< Orphans >::Instance()->ExitNow())
   {
      return Return;
   }

   //  Exit immediately if the thread trapped during recovery.
   //  In this case, the Thread must first be deleted.
   //
   if(trapped_)
   {
      return Release;
   }

   trapped_ = true;
   SetSignal(sig);

   if((sig == SIGSTACK1) && (systhrd_ != nullptr))
   {
      systhrd_->status_.set(SysThread::StackOverflowed);
   }

   ThreadAdmin::Incr(ThreadAdmin::Traps);

   auto reg = Singleton< PosixSignalRegistry >::Instance();

   if(!reg->Attrs(sig).test(PosixSignal::NoError))
   {
      auto log = Log::Create(ThreadLogGroup, ThreadException);

      if(log != nullptr)
      {
         auto trapcount = ThreadAdmin::TrapCount();
         *log << Log::Tab << "in " << to_str();
         *log << ": trap number " << trapcount << CRLF;

         if(e != nullptr)
         {
            *log << Log::Tab << "type=" << e->what() << CRLF;
            if(ex != nullptr) ex->Display(*log, spaces(4));
         }
         else
         {
            if(sig != SIGNIL)
            {
               *log << Log::Tab << "signal=" << reg->strSignal(sig) << CRLF;
            }
            else
            {
               *log << Log::Tab << UnknownExceptionStr << CRLF;
               if(Element::RunningInLab()) return Rethrow;
            }
         }

         //  A thread that traps during recovery or that traps too often
         //  will be exited and recreated.
         //
         if(priv_->recovering_)
         {
            SetSignal(SIGRETRAP);
            *log << Log::Tab << TrapDuringRecoveryStr << CRLF;
         }
         else if(priv_->trapLbc_.HasReachedLimit())
         {
            SetSignal(SIGTRAPS);
            *log << Log::Tab << TrapLimitReachedStr << CRLF;
         }

         if(stack != nullptr) *log << stack->str();

         //  Log a thread's data if it will be forced to exit.
         //
         if(reg->Attrs(priv_->signal_).test(PosixSignal::Exit))
         {
            *log << Log::Tab << ThreadDataStr << CRLF;
            Display(*log, Log::Tab + spaces(2), NoFlags);
         }

         Log::Submit(log);
      }
   }

   //  If this is a final signal, force the thread to exit.
   //
   if(reg->Attrs(sig).test(PosixSignal::Final))
   {
      if(!reg->Attrs(sig).test(PosixSignal::NoError))
      {
         ThreadAdmin::Incr(ThreadAdmin::Kills);
      }

      trapped_ = false;
      return Release;
   }

   //  Resume execution at the top of Start.
   //
   trapped_ = false;
   return Continue;
}

//------------------------------------------------------------------------------

fn_name Thread_Unblock = "Thread.Unblock";

void Thread::Unblock()
{
   Debug::ft(Thread_Unblock);
}

//------------------------------------------------------------------------------

fn_name Thread_Vector = "Thread.Vector";

std::atomic_uint32_t* Thread::Vector()
{
   Debug::ft(Thread_Vector);

   return &RunningThread()->priv_->vector_;
}
}
