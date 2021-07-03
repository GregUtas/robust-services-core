//==============================================================================
//
//  Thread.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "Duration.h"
#include "Thread.h"
#include "Dynamic.h"
#include "FunctionTrace.h"
#include <cctype>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <ios>
#include <map>
#include <sstream>
#include <utility>
#include <vector>
#include "AllocationException.h"
#include "CliThread.h"
#include "CoutThread.h"
#include "Daemon.h"
#include "Debug.h"
#include "Element.h"
#include "ElementException.h"
#include "Formatters.h"
#include "InitThread.h"
#include "LeakyBucketCounter.h"
#include "Log.h"
#include "Memory.h"
#include "MsgBuffer.h"
#include "MutexGuard.h"
#include "MutexRegistry.h"
#include "NbAppIds.h"
#include "NbLogs.h"
#include "NbPools.h"
#include "NbSignals.h"
#include "NbTracer.h"
#include "PosixSignal.h"
#include "PosixSignalRegistry.h"
#include "Registry.h"
#include "Restart.h"
#include "RootThread.h"
#include "SignalException.h"
#include "Singleton.h"
#include "Statistics.h"
#include "StatisticsRegistry.h"
#include "SysMutex.h"
#include "SysThreadStack.h"
#include "SysTickTimer.h"
#include "ThreadAdmin.h"
#include "ThreadRegistry.h"
#include "TimePoint.h"
#include "Tool.h"
#include "TraceBuffer.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  FtLocks_ provides a per-thread lock to prevent nested calls to functions
//  that are invoked from Debug::ft and that, in turn, invoke functions that
//  also invoke Debug::Ft.  Nested calls to these functions must be blocked
//  to prevent a stack overflow.
//
//  NOTE ON INITIALIZATION ORDER:
//  ============================
//  Debug::ft is invoked fairly early during initialization, well before entry
//  to main().  The SysMutex and Duration items defined at file scope in this
//  file end up invoking Debug::ft during their initialization.  If FtLocks_
//  has not been initialized at that point, FtLocks_.find() will trap.  This
//  could occur if an item initialized in another file also causes Debug::ft
//  to be invoked.
//
//  SysTickTimer provides the time at which a function was invoked, so it is
//  created after FtLocks_.  FtLocks_ must be created first because functions
//  invoked to create SysTickTimer invoke Debug::ft, which requires FtLocks_
//  to have been constructed.
//
std::map< SysThreadId, std::atomic_flag > FtLocks_;

//  Returns the Debug::ft lock for the running thread.
//
std::atomic_flag& AccessFtLock()
{
   Debug::noft();

   auto nid = SysThread::RunningThreadId();
   auto iter = FtLocks_.find(nid);

   if(iter != FtLocks_.cend())
   {
      return iter->second;
   }

   //  The lock must be created and initialized.
   //
   auto& lock = FtLocks_[nid];
   lock.clear();
   return lock;
}

const SysTickTimer* TickTimer = SysTickTimer::Instance();

//------------------------------------------------------------------------------
//
//  Records invocations of Pause.
//
class ThreadTrace : public FunctionTrace
{
public:
   static const Id PauseEnter = 1;  // entering Pause
   static const Id PauseExit = 2;   // returning from Pause

   //  Creates a trace record for the event identified by RID, which
   //  occurred in function FUNC.  INFO is any debugging information.
   //
   static void CaptureEvent(fn_name_arg func, Id rid, int32_t info = 0);

   //  Overridden to allocate the trace record from the default heap,
   //  because the buffer for FunctionTrace records does not support
   //  subclasses with additional memory requirements.
   //
   static void* operator new(size_t size);

   //  Overridden to return the record to the default heap.
   //
   static void operator delete(void* addr);

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, const string& opts) override;
private:
   //  Private to restrict creation to CaptureEvent.
   //
   ThreadTrace(fn_name_arg func, fn_depth depth, Id rid, int32_t info);

   //  Additional debug information.
   //
   const int32_t info_;
};

//------------------------------------------------------------------------------

ThreadTrace::ThreadTrace(fn_name_arg func,
   fn_depth depth, Id rid, int32_t info) :
   FunctionTrace(func, depth),
   info_(info)
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

fn_name ThreadTrace_CaptureEvent = "ThreadTrace.CaptureEvent";

void ThreadTrace::CaptureEvent(fn_name_arg func, Id rid, int32_t info)
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
   switch(rid)
   {
   case PauseExit:
   case PauseEnter:
   {
      auto depth = SysThreadStack::FuncDepth();
      auto buff = Singleton< TraceBuffer >::Instance();
      auto rec = new ThreadTrace(func, depth - 2, rid, info);
      buff->Insert(rec);
      break;
   }
   default:
      Debug::SwLog(ThreadTrace_CaptureEvent, "unexpected event", rid);
   }
}

//------------------------------------------------------------------------------

bool ThreadTrace::Display(ostream& stream, const string& opts)
{
   if(!FunctionTrace::Display(stream, opts)) return false;

   switch(rid_)
   {
   case PauseEnter:
      if(info_ == -1)
         stream << " (forever)";
      else
         stream << " (msecs=" << Duration(info_, TICKS).To(mSECS) << ')';
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

//------------------------------------------------------------------------------

void ThreadTrace::operator delete(void* addr)
{
   ::operator delete(addr);
}

//------------------------------------------------------------------------------

void* ThreadTrace::operator new(size_t size)
{
   auto addr = ::operator new(size, std::nothrow);
   if(addr != nullptr) return addr;
   throw AllocationException(MemPermanent, size);
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
   HighWatermarkPtr maxTime_;
   AccumulatorPtr   totTime_;
};

//------------------------------------------------------------------------------

ThreadStats::ThreadStats()
{
   Debug::ft("ThreadStats.ctor");

   traps_.reset(new Counter("traps"));
   exceeds_.reset(new Counter("running unpreemptable too long"));
   yields_.reset(new Counter("yields"));
   interrupts_.reset(new Counter("interrupts"));
   maxMsgs_.reset(new HighWatermark("longest length of message queue"));
   maxStack_.reset(new HighWatermark("highest stack usage (words)"));
   maxTime_.reset
      (new HighWatermark("longest time scheduled in (usecs)", TICKS_PER_uSEC));
   totTime_.reset
      (new Accumulator("total execution time (msecs)", TICKS_PER_mSEC));
}

//------------------------------------------------------------------------------

ThreadStats::~ThreadStats()
{
   Debug::ftnt("ThreadStats.dtor");
}

//==============================================================================
//
//  Information about a context switch.
//
struct ContextSwitch
{
   //  Initializes the record.
   //
   ContextSwitch();

   //  When the thread started to run.
   //
   TimePoint in;

   //  When the context switch occurred.
   //
   TimePoint out;

   //  The native identifier for the thread being scheduled out.
   //
   SysThreadId nid;

   //  The thread being scheduled out.
   //
   ThreadId tid;

   //  Set if unpreemptable when scheduled out.
   //
   bool locked;
};

//------------------------------------------------------------------------------

ContextSwitch::ContextSwitch() :
   in(0),
   out(0),
   nid(0),
   tid(0),
   locked(false)
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

   //  Deleted to prohibit copy assignment.
   //
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

   //  Characters used when displaying context switches.
   //
   static const char IdleChar = '.';      // thread not running
   static const char UnlockedChar = '|';  // thread running preemptably
   static const char LockedChar = '#';    // thread running unpreemptably
   static const char EndChar = 'V';       // thread scheduled out

   //  Displays context switches in STREAM.
   //
   void DisplaySwitches(ostream& stream) const;
private:
   //  Private because this is a singleton.
   //
   ContextSwitches();

   //  Private because this is a singleton.
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

ContextSwitches::ContextSwitches() :
   capacity_(0),
   next_(0),
   switches_(nullptr),
   full_(false),
   log_(false)
{
   Debug::ft("ContextSwitches.ctor");

   switches_.reset(new ContextSwitch[4096]);
   capacity_ = 4096;
}

//------------------------------------------------------------------------------

fn_name ContextSwitches_dtor = "ContextSwitches.dtor";

ContextSwitches::~ContextSwitches()
{
   Debug::ftnt(ContextSwitches_dtor);

   Debug::SwLog(ContextSwitches_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

ContextSwitch* ContextSwitches::AddSwitch()
{
   static Duration timeout(10, mSECS);

   if(!log_) return nullptr;

   ContextSwitch* cs;

   if(ContextSwitchesLock_.Acquire(timeout) == SysMutex::Acquired)
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
//
//  Thread activity at a time point associated with a context switch.
//
struct SchedSnapshot
{
   //  MAX is the maximum ThreadId seen while recording context switches.
   //
   explicit SchedSnapshot(ThreadId max) :
      activity(nullptr),
      nid(0)
   {
      activity.reset(new char[max + 1]);
      for(size_t i = 0; i <= max; ++i) activity[i] = ContextSwitches::IdleChar;
   }

   //  An array of characters, one per thread, indicating what each thread was
   //  doing at this time point.
   //
   std::unique_ptr< char[] > activity;

   //  If a thread was scheduled out at this time point, how long it had run.
   //
   Duration duration;

   //  Set if an unknown thread was associated with this entry.
   //
   SysThreadId nid;
};

//  Each SchedSnapshot is managed by a unique_ptr.
//
typedef std::unique_ptr< SchedSnapshot > SchedSnapshotPtr;

//  Associates a time point with what each thread was doing at that time.
//
typedef std::pair< TimePoint, SchedSnapshotPtr> SchedEntry;

//  Maps each time point associated with a context switch to what each thread
//  was doing at that time.
//
typedef std::map< TimePoint, SchedSnapshotPtr> SchedEntries;

//  The header for displaying context switches.  ThreadIds starting at 1 are
//  output dynamically following the 0.  Each thread's activity is then shown
//  in its column.
//
fixed_string SwitchHeader1 = "             Ran for  -";
fixed_string SwitchHeader2 = "Timestamp    (usecs)  0";

//  The footer (legend) for displaying context switches.
//
fixed_string SwitchFooter1 =
   "Symbols: . idle   # unpreemptable   | preemptable   V scheduled out";
fixed_string SwitchFooter2 =
   "         * multiple threads running unpreemptably (rightmost column)";

//------------------------------------------------------------------------------

fn_name ContextSwitches_DisplaySwitches = "ContextSwitches.DisplaySwitches";

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
   auto elems = next_;

   if(full_)
   {
      first = next_;
      elems = capacity_;
   }

   //  Find the maximum ThreadId recorded during the context switches.
   //
   ThreadId max = 0;

   for(size_t i = first, count = elems; count > 0; --count)
   {
      if(switches_[i].tid > max) max = switches_[i].tid;
      i = (i == capacity_ - 1 ? 0 : i + 1);
   }

   //  For each context switch, create an entry for its time in and time out.
   //
   SchedEntries timeline;

   for(size_t i = first, count = elems; count > 0; --count)
   {
      auto entry = &switches_[i];

      auto curr = timeline.find(entry->in);

      if(curr == timeline.cend())
      {
         timeline.insert
            (SchedEntry(entry->in, SchedSnapshotPtr(new SchedSnapshot(max))));
      }

      curr = timeline.find(entry->out);

      if(curr == timeline.cend())
      {
         timeline.insert
            (SchedEntry(entry->out, SchedSnapshotPtr(new SchedSnapshot(max))));
      }
      else
      {
         //  An unknown thread always ends up here because its entry->in and
         //  entry->out are the same.
         //
         if(entry->tid == NIL_ID)
         {
            curr->second->nid = entry->nid;
         }
      }

      i = (i == capacity_ - 1 ? 0 : i + 1);
   }

   //  For each context switch, record whether the thread was running locked
   //  or unlocked between the time it was scheduled and the time it was
   //  scheduled out.
   //
   for(size_t i = first, count = elems; count > 0; --count)
   {
      auto entry = &switches_[i];
      auto begin = timeline.find(entry->in);

      if(begin == timeline.cend())
      {
         Debug::SwLog(ContextSwitches_DisplaySwitches, "begin not found", i);
         return;
      }

      auto end = timeline.find(entry->out);

      if(end == timeline.cend())
      {
         Debug::SwLog(ContextSwitches_DisplaySwitches, "end not found", i);
         return;
      }

      auto symbol = (entry->locked ? LockedChar : UnlockedChar);

      for(NO_OP; begin != end; ++begin)
      {
         begin->second->activity[entry->tid] = symbol;
      }

      end->second->activity[entry->tid] = EndChar;
      end->second->duration = entry->out - entry->in;

      i = (i == capacity_ - 1 ? 0 : i + 1);
   }

   //  Output the context switch timeline.  The thread identifiers appear
   //  across the top, after SwitchHeader.  For each entry, output
   //  o the time, for correlation with any function trace (mm::ss.msecs)
   //  o when a thread is scheduled out, how long it had run (in usecs)
   //  o the activity for each thread (running locked, running unlocked,
   //    or being scheduled out)
   //
   auto multilocked = false;

   stream << CRLF;
   stream << "Context switches: " << elems << CRLF;

   stream << SwitchHeader1;

   auto front = ((3 * max) - strlen("Threads")) / 2;
   auto back = ((3 * max) + 1 - strlen("Threads")) / 2;
   stream << string(front, '-') << "Threads" << string(back, '-') << CRLF;

   stream << SwitchHeader2;

   for(ThreadId t = 1; t <= max; ++t)
   {
      stream << setw(3) << t;
   }

   stream << CRLF;

   for(auto entry = timeline.cbegin(); entry != timeline.cend(); ++entry)
   {
      stream << entry->first.to_str(MinsField);

      if(entry->second->duration > ZERO_SECS)
         stream << setw(11) << entry->second->duration.To(uSECS);
      else if(entry->second->nid != 0)
         stream << strHex(entry->second->nid, 11, true);
      else
         stream << spaces(11);

      size_t locked = 0;

      for(ThreadId t = 0; t <= max; ++t)
      {
         auto c = entry->second->activity[t];
         if(c == LockedChar) ++locked;
         stream << spaces(2) << entry->second->activity[t];
      }

      if(locked > 1)
      {
         stream << "  *";
         multilocked = true;
      }

      stream << CRLF;
   }

   stream << SwitchFooter1 << CRLF;

   if(multilocked)
   {
      Debug::SwLog(ContextSwitches_DisplaySwitches, "simultaneously locked", 0);
      stream << SwitchFooter2 << CRLF;
      stream << "UNPREEMPTABLE THREADS RAN SIMULTANEOUSLY" << CRLF;
   }
}

//------------------------------------------------------------------------------

TraceRc ContextSwitches::LogSwitches(bool on)
{
   Debug::ft("ContextSwitches.LogSwitches");

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
//  What to do with a thread on the next scheduling operation.
//
enum SchedulingAction : uint8_t
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
//  o to survive deletion of this object
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

   //  Overridden to display member variables.
   //
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   //  Calls to MakeUnpreemptable minus calls to MakePreemptable.
   //
   uint8_t unpreempts_;

   //  Calls to ImmUnprotect minus calls to ImmProtect.
   //
   uint8_t immUnprots_;

   //  Calls to MemUnprotect minus calls to MemProtect.
   //
   uint8_t memUnprots_;

   //  The number of mutexes currently held by the thread.
   //
   uint8_t mutexes_;

   //  The depth of nested software logs.
   //
   uint8_t swlogs_;

   //  Set if the thread has been entered.
   //
   bool entered_;

   //  Set when ready to run but waiting to be signalled.
   //
   bool waiting_;

   //  Set when running unpreemptably.
   //
   bool locked_;

   //  Set if the thread's current message is being traced.
   //
   bool traceMsg_;

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

   //  Incremented when a trap occurs, and reset when Recover or Enter
   //  is invoked.  Upon entering TrapHandler, a non-zero value means
   //  that another trap occurred during recovery, in which case the
   //  thread is forced to exit.
   //
   uint8_t traps_;

   //  Set if thread is undergoing recovery after a trap.
   //
   bool recovering_;

   //  Set if the thread's data has been saved in a trap log.
   //
   bool logged_;

   //  Set if the thread is exiting.
   //
   bool exiting_;

   //  Determines what happens to the thread on a scheduling operation.
   //
   SchedulingAction action_;

   //  The reason why the thread is blocked.
   //
   BlockingReason blocked_;

   //  Whether the thread is being traced.
   //
   TraceStatus status_;

   //  The signal to be raised or that is being handled.
   //
   signal_t signal_;

   //  The thread's stack pointer after entering Thread::Start.
   //
   const signal_t* stackBase_;

   //  The mutex on which the thread is currently blocked.
   //
   SysMutex* acquiring_;

   //  Determines whether the thread has failed to yield too often.
   //
   LeakyBucketCounter rtcLbc_;

   //  Determines whether the thread has trapped too often.
   //
   LeakyBucketCounter trapLbc_;

   //  Flags set when Interrupt was invoked on the thread.
   //
   std::atomic_uint32_t vector_;

   //  How long the thread ran during the previous short interval for
   //  thread statistics.  This provides a view of how thread behavior
   //  has recently changed.
   //
   Duration prevTime_;

   //  How long the thread has run during the current short interval for
   //  thread statistics.
   //
   Duration currTime_;

   //  The time at which the thread became ready to run.
   //
   TimePoint readyTime_;

   //  The last time at which the thread started to run unpreemptably.
   //  If the thread is preemptable, the last time it exited a blocking
   //  operation, although it may have been scheduled out and back in
   //  several times since then.
   //
   TimePoint currStart_;

   //  The time at which the thread will be trapped if it has not yielded.
   //  Reset to 0 when the thread yields, and set when it resumes running
   //  unpreemptably.
   //
   TimePoint currEnd_;
};

//------------------------------------------------------------------------------

ThreadPriv::ThreadPriv() :
   unpreempts_(1),
   immUnprots_(0),
   memUnprots_(0),
   mutexes_(0),
   swlogs_(0),
   entered_(false),
   waiting_(false),
   locked_(false),
   traceMsg_(false),
   tracing_(false),
   autostop_(false),
   warned_(false),
   trap_(false),
   traps_(0),
   recovering_(false),
   logged_(false),
   exiting_(false),
   action_(RunThread),
   blocked_(NotBlocked),
   status_(TraceDefault),
   signal_(SIGNIL),
   stackBase_(nullptr),
   acquiring_(nullptr),
   vector_(0)
{
   Debug::ft("ThreadPriv.ctor");

   rtcLbc_.Initialize(ThreadAdmin::RtcLimit(), ThreadAdmin::RtcInterval());
   trapLbc_.Initialize(ThreadAdmin::TrapLimit(), ThreadAdmin::TrapInterval());
}

//------------------------------------------------------------------------------

ThreadPriv::~ThreadPriv()
{
   Debug::ftnt("ThreadPriv.dtor");
}

//------------------------------------------------------------------------------

void ThreadPriv::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "unpreempts : " << int(unpreempts_) << CRLF;
   stream << prefix << "immUnprots : " << int(immUnprots_) << CRLF;
   stream << prefix << "memUnprots : " << int(memUnprots_) << CRLF;
   stream << prefix << "mutexes    : " << int(mutexes_) << CRLF;
   stream << prefix << "swlogs     : " << int(swlogs_) << CRLF;
   stream << prefix << "entered    : " << entered_ << CRLF;
   stream << prefix << "waiting    : " << waiting_ << CRLF;
   stream << prefix << "locked     : " << locked_ << CRLF;
   stream << prefix << "traceMsg   : " << traceMsg_ << CRLF;
   stream << prefix << "tracing    : " << tracing_ << CRLF;
   stream << prefix << "autostop   : " << autostop_ << CRLF;
   stream << prefix << "warned     : " << warned_ << CRLF;
   stream << prefix << "trap       : " << trap_ << CRLF;
   stream << prefix << "traps      : " << int(traps_) << CRLF;
   stream << prefix << "recovering : " << recovering_ << CRLF;
   stream << prefix << "logged     : " << logged_ << CRLF;
   stream << prefix << "exiting    : " << exiting_ << CRLF;
   stream << prefix << "action     : " << action_ << CRLF;
   stream << prefix << "blocked    : " << blocked_ << CRLF;
   stream << prefix << "status     : " << status_ << CRLF;
   stream << prefix << "signal     : " << signal_ << CRLF;
   stream << prefix << "stackBase  : " << stackBase_ << CRLF;
   stream << prefix << "acquiring  : ";
   if(acquiring_ == nullptr)
      stream << acquiring_ << CRLF;
   else
      stream << acquiring_->Name() << CRLF;
   stream << prefix << "rtcLbc     : " << CRLF;
   rtcLbc_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "trapLbc    : " << CRLF;
   trapLbc_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "vector     : "
      << std::hex << vector_ << std::dec << CRLF;
   stream << prefix << "prevTime   : " << prevTime_.Ticks() << CRLF;
   stream << prefix << "currTime   : " << currTime_.Ticks() << CRLF;
   stream << prefix << "readyTime  : " << readyTime_.Ticks() << CRLF;
   stream << prefix << "currStart  : " << currStart_.Ticks() << CRLF;
   stream << prefix << "currEnd    : " << currEnd_.Ticks() << CRLF;
}

//==============================================================================

fixed_string UnknownExceptionStr = "unknown exception";
fixed_string ThreadDataStr = "Thread Data:";
fixed_string TrapDuringRecoveryStr = "TRAP DURING RECOVERY.";
fixed_string TrapLimitReachedStr = "TRAP LIMIT EXCEEDED.";
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
//  watchdog > system > loadtest/payload/maintenance/operations/background/audit
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
std::atomic< Thread* > ActiveThread_ = nullptr;

//  The factions that may currently be scheduled.
//
FactionFlags FactionsEnabled_ = FactionFlags();

//  Causes a stack check each time it counts down to one.
//
size_t StackCheckCounter_ = 1;

//  The time when the previous short interval for thread statistics began.
//
TimePoint PrevIntervalStart_ = TimePoint();

//  The time when the current short interval for thread statistics began.
//
TimePoint CurrIntervalStart_ = TimePoint();

//  The amount of idle time during the most recent short interval.
//
Duration TimeIdle_ = Duration();

//  The time spent in threads during the most recent short interval.
//
Duration TimeUsed_ = Duration();

//------------------------------------------------------------------------------

const ThreadId Thread::MaxId = 99;

//------------------------------------------------------------------------------

Thread::Thread(Faction faction, Daemon* daemon) :
   daemon_(daemon),
   tid_(NIL_ID),
   faction_(faction),
   deleting_(false)
{
   Debug::ft("Thread.ctor");

   priv_.reset(new ThreadPriv);
   stats_.reset(new ThreadStats);
   msgq_.Init(Pooled::LinkDiff());

   auto reg = Singleton< ThreadRegistry >::Instance();

   if(reg->Threads().empty())
   {
      //  There are no threads, so we must be wrapping the root thread.
      //
      CurrIntervalStart_ = TimePoint::Now();
      Singleton< ContextSwitches >::Instance();

      systhrd_.reset(new SysThread);
      priv_->currStart_ = TimePoint::TimeZero();
      priv_->entered_ = true;
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

   reg->Created(systhrd_.get(), this);
   ThreadAdmin::Incr(ThreadAdmin::Creations);
   if(daemon_ != nullptr) daemon_->ThreadCreated(this);
}

//------------------------------------------------------------------------------

Thread::~Thread()
{
   Debug::ftnt("Thread.dtor");

   auto threads = Singleton< ThreadRegistry >::Extant();
   threads->Destroying(Deleting, systhrd_.get());

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

   //  This thread was constructed and has not invoked Thread::Exit.
   //  This is a serious error, so output a log now.
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
   Debug::noft();

   auto thr = ActiveThread_.load();
   if(thr == nullptr) return nullptr;
   if(thr->deleting_) return nullptr;
   return thr;
}

//------------------------------------------------------------------------------

SysMutex* Thread::BlockingMutex() const
{
   return priv_->acquiring_;
}

//------------------------------------------------------------------------------

TraceStatus Thread::CalcStatus(bool dynamic) const
{
   if(dynamic && priv_->traceMsg_) return TraceIncluded;
   if(priv_->status_ != TraceDefault) return priv_->status_;

   auto nbt = Singleton< NbTracer >::Extant();
   if(nbt == nullptr) return TraceExcluded;
   auto status = nbt->FactionStatus(faction_);
   if(status != TraceDefault) return status;

   auto buff = Singleton< TraceBuffer >::Extant();
   if(buff == nullptr) return TraceExcluded;
   if(buff->FilterIsOn(TraceAll)) return TraceIncluded;
   return TraceExcluded;
}

//------------------------------------------------------------------------------

bool Thread::CanBeScheduled() const
{
   return (!deleting_ && (priv_->blocked_ == NotBlocked) &&
      FactionsEnabled_.test(faction_));
}

//------------------------------------------------------------------------------

void Thread::CauseTrap()
{
   Debug::ft("Thread.CauseTrap");

   auto p = reinterpret_cast< char* >(BAD_POINTER);
   if(*p == 0) ++p;
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

void Thread::ClaimBlocks()
{
   Debug::ft("Thread.ClaimBlocks");

   //  Claim messages on the queue.  Sometimes there are hundreds of these,
   //  so trying to add them all to GetSubtended's array isn't possible.
   //
   for(auto m = msgq_.First(); m != nullptr; msgq_.Next(m))
   {
      m->ClaimBlocks();
   }
}

//------------------------------------------------------------------------------

bool Thread::ClearActiveThread(Thread* active)
{
   return ActiveThread_.compare_exchange_strong(active, nullptr);
}

//------------------------------------------------------------------------------

Duration Thread::CurrTimeRunning() const
{
   Debug::ft("Thread.CurrTimeRunning");

   if(!priv_->currStart_.IsValid()) return ZERO_SECS;
   return (TimePoint::Now() - priv_->currStart_);
}

//------------------------------------------------------------------------------

MsgBuffer* Thread::DeqMsg(const Duration& timeout)
{
   Debug::ft("Thread.DeqMsg");

   auto buff = msgq_.Deq();

   if(buff == nullptr)
   {
      if(timeout == TIMEOUT_IMMED) return nullptr;

      switch(Pause(timeout))
      {
      case DelayError:
         Restart::Initiate(RestartWarm, ThreadPauseFailed, Tid());
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

   priv_->traceMsg_ = (buff->GetStatus() == TraceIncluded);
   return buff;
}

//------------------------------------------------------------------------------

void Thread::Destroy()
{
   Debug::ft("Thread.Destroy");

   delete this;
}

//------------------------------------------------------------------------------

void Thread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   auto lead = prefix + spaces(2);
   stream << prefix << "systhrd  : " << systhrd_.get() << CRLF;
   if(systhrd_ != nullptr) systhrd_->Display(stream, lead, options);
   stream << prefix << "daemon   : " << strObj(daemon_) << CRLF;
   stream << prefix << "tid      : " << tid_ << CRLF;
   stream << prefix << "faction  : " << int(faction_) << CRLF;
   stream << prefix << "deleting : " << deleting_ << CRLF;
   stream << prefix << "msgq     : " << CRLF;
   msgq_.Display(stream, lead, options);
   stream << prefix << "priv     : " << CRLF;
   priv_->Display(stream, lead, options);
}

//------------------------------------------------------------------------------

void Thread::DisplayContextSwitches(ostream& stream)
{
   Singleton< ContextSwitches >::Instance()->DisplaySwitches(stream);
}

//------------------------------------------------------------------------------

void Thread::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft("Thread.DisplayStats");

   stream << spaces(2) << AbbrName()
      << SPACE << strIndex(Tid(), 0, false) << CRLF;

   stats_->traps_->DisplayStat(stream, options);
   stats_->exceeds_->DisplayStat(stream, options);
   stats_->yields_->DisplayStat(stream, options);
   stats_->interrupts_->DisplayStat(stream, options);
   stats_->maxMsgs_->DisplayStat(stream, options);
   stats_->maxStack_->DisplayStat(stream, options);
   stats_->maxTime_->DisplayStat(stream, options);
   stats_->totTime_->DisplayStat(stream, options);
}

//------------------------------------------------------------------------------

fixed_string SchedHeader =
"      THREADS          |   SINCE START OF CURRENT 15-MINUTE INTERVAL  | LAST\n"
"                       |            rtc  max   max     max  total     |5 SEC\n"
"id    name     host f b| ex yields  t/o msgs stack   usecs  msecs %cpu| %cpu";
//        1         2         3         4         5         6         7
//234567890123456789012345678901234567890123456789012345678901234567890123456
fixed_string SchedLine =
"----------------------------------------------------------------------------";

void Thread::DisplaySummaries(ostream& stream)
{
   Duration time0;  // duration of current interval
   Duration idle0;  // idle time during current interval
   Duration used0;  // time in all threads during current interval

   auto threads = Singleton< ThreadRegistry >::Instance()->GetThreads();

   for(auto t = threads.cbegin(); t != threads.cend(); ++t)
   {
      used0 += Duration((*t)->stats_->totTime_->Curr(), TICKS);
   }

   time0 = TimePoint::Now() - StatisticsRegistry::StartTime();
   idle0 = (time0 > used0 ? time0 - used0 : ZERO_SECS);

   stream << std::setprecision(1) << std::fixed;

   stream << "SCHEDULER REPORT: " << Element::strTimePlace() << CRLF;
   stream << "for interval beginning at ";
   stream << StatisticsRegistry::StartTime().to_str() << CRLF;

   stream << SchedLine << CRLF;
   stream << SchedHeader << CRLF;
   stream << SchedLine << CRLF;

   stream << setw(10) << "idle";
   stream << setw(55) << (idle0 + Duration(500, uSECS)).To(mSECS);
   stream << setw(5) << 100 * double(idle0.Ticks()) / time0.Ticks();

   //  Set TIME1 to the length of the previous short interval.
   //
   auto time1 = TimeIdle_ + TimeUsed_;

   if(time1 > ZERO_SECS)
   {
      stream << setw(6) << 100 * double(TimeIdle_.Ticks()) / time1.Ticks();
   }

   stream << CRLF;

   for(auto t = threads.cbegin(); t != threads.cend(); ++t)
   {
      (*t)->DisplaySummary(stream, time0, time1);
   }

   stream << SchedLine << CRLF;

   if(Singleton< ContextSwitches >::Instance()->LoggingOn())
   {
      stream << "Context switch logging is ON." << CRLF;
   }
}

//------------------------------------------------------------------------------

void Thread::DisplaySummary
   (ostream& stream, const Duration& time0, const Duration& time1) const
{
   Duration currTime(stats_->totTime_->Curr(), TICKS);

   stream << setw(2) << Tid();
   stream << setw(8) << AbbrName() << SPACE;
   stream << setw(8) << std::hex << NativeThreadId() << std::dec;

   auto f = FactionChar(faction_);
   if(priv_->unpreempts_ == 0) f = tolower(f);
   stream << setw(2) << f;

   auto r = BlockingReasonChar(priv_->blocked_);
   r = (priv_->blocked_ == NotBlocked ? SPACE : toupper(r));
   stream << setw(2) << r;

   stream << setw(4) << stats_->traps_->Curr();
   stream << setw(7) << stats_->yields_->Curr();
   stream << setw(5) << stats_->exceeds_->Curr();
   stream << setw(5) << stats_->maxMsgs_->Curr();
   stream << setw(6) << stats_->maxStack_->Curr();

   auto usecs = Duration(stats_->maxTime_->Curr(), TICKS).To(uSECS);

   if(usecs <= 9999999)
      stream << setw(8) << usecs;
   else
      stream << " 10+ sec";

   auto pct = 100 * double(currTime.Ticks()) / time0.Ticks();
   stream << setw(7) << (currTime + Duration(500, uSECS)).To(mSECS);
   stream << setw(5) << pct;

   if(time1 > ZERO_SECS)
   {
      pct = 100 * double(priv_->prevTime_.Ticks()) / time1.Ticks();
      stream << setw(6) << pct;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Thread::EnableFactions(const FactionFlags& enabled)
{
   Debug::ft("Thread.EnableFactions");

   FactionsEnabled_ = enabled;
}

//------------------------------------------------------------------------------

bool Thread::EnqMsg(MsgBuffer& msg)
{
   Debug::ft("Thread.EnqMsg");

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

   if(thr->priv_->mutexes_ > 0)
   {
      Debug::SwLog(Thread_EnterBlockingOperation,
         "mutex holder", thr->priv_->mutexes_);
   }

   thr->priv_->blocked_ = why;
   thr->Suspend();
   return true;
}

//------------------------------------------------------------------------------

bool Thread::EnterSwLog()
{
   Debug::ftnt("Thread.EnterSwLog");

   //  If the thread is already generating nested software logs, prevent
   //  further nesting.
   //
   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return true;
   if(thr->priv_ == nullptr) return true;
   if(++thr->priv_->swlogs_ <= 2) return true;
   --thr->priv_->swlogs_;
   return false;
}

//------------------------------------------------------------------------------

main_t Thread::EnterThread(void* arg)
{
   Debug::ft("Thread.EnterThread");

   //  Our argument is a pointer to a Thread.
   //
   auto self = static_cast< Thread* >(arg);
   return self->Start();
}

//------------------------------------------------------------------------------

main_t Thread::Exit(signal_t sig)
{
   Debug::ft("Thread.Exit");

   //  If the thread is holding any mutexes, release them.
   //  Then log the exit.
   //
   Singleton< MutexRegistry >::Instance()->Abandon();

   ostringstreamPtr log = nullptr;

   if(priv_->traps_ > 0)
   {
      log = Log::Create(ThreadLogGroup, ThreadForcedToExit);
   }
   else
   {
      if(LogSignal(sig) || Element::RunningInLab())
      {
         log = Log::Create(ThreadLogGroup, ThreadExited);
      }
   }

   if(log != nullptr)
   {
      auto reg = Singleton< PosixSignalRegistry >::Instance();
      *log << Log::Tab << "thread=" << to_str() << CRLF;
      *log << Log::Tab << "signal=" << reg->strSignal(sig);
      Log::Submit(log);
   }

   priv_->exiting_ = true;
   Destroy();
   return sig;
}

//------------------------------------------------------------------------------

void Thread::ExitBlockingOperation(fn_name_arg func)
{
   Debug::ft("Thread.ExitBlockingOperation");

   auto thr = RunningThread();
   thr->priv_->currStart_ = TimePoint::Now();

   if(thr->priv_->blocked_ != NotBlocked)
      thr->priv_->blocked_ = NotBlocked;
   else
      Debug::SwLog(Thread_EnterBlockingOperation, "not blocked", 0);

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

void Thread::ExitIfSafe(debug64_t offset)
{
   Debug::noft();

   //  If the thread is blocked, it just invoked ExitBlockingOperation.  It
   //  can be trapped before it can even record the time when it started to
   //  run, so record it now.  This prevents ContextSwitches.DisplaySwitches
   //  from generating a "simultaneous unpreemptable threads" log.
   //
   if(priv_->blocked_ != NotBlocked)
   {
      priv_->currStart_ = TimePoint::Now();
   }

   //  Reset action_ to prevent this from being invoked again.  If it isn't
   //  safe to exit the thread now, try again later.
   //
   priv_->action_ = RunThread;

   //  This function can be invoked from Debug::ft and TrapCheck, and the
   //  following functions also invoke Debug::Ft.  Reinvocations of this
   //  function are therefore blocked to prevent a stack overflow.
   //
   auto& lock = AccessFtLock();
   if(lock.test_and_set()) return;

   Debug::ft("Thread.ExitIfSafe");

   if((priv_->traps_ == 0) && SysThreadStack::TrapIsOk())
   {
      SetTrap(false);
      lock.clear();
      throw SignalException(priv_->signal_, offset);
   }

   lock.clear();
   priv_->action_ = ExitThread;
}

//------------------------------------------------------------------------------

bool Thread::ExitOnRestart(RestartLevel level) const
{
   Debug::ft("Thread.ExitOnRestart");

   //  RootThread and InitThread run during a restart.  A thread blocked on
   //  stream input, such as CinThread, cannot be forced to exit because C++
   //  has no mechanism for interrupting it.
   //
   if(faction_ >= SystemFaction) return false;
   if(priv_->blocked_ == BlockedOnStream) return false;
   return true;
}

//------------------------------------------------------------------------------

void Thread::ExitSwLog(bool all)
{
   Debug::ftnt("Thread.ExitSwLog");

   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;
   if(thr->priv_ == nullptr) return;
   if(thr->priv_->swlogs_ == 0) return;
   if(all)
      thr->priv_->swlogs_ = 0;
   else
      --thr->priv_->swlogs_;
}

//------------------------------------------------------------------------------

void Thread::ExtendTime(const Duration& time)
{
   Debug::ft("Thread.ExtendTime");

   //  Time cannot be extended for an orphaned thread: its Thread object has
   //  been deleted.  This is invoked during exception handling, so don't get
   //  upset if the thread can't be found.
   //
   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;
   thr->priv_->currEnd_ += time;
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

Thread* Thread::FindRunningThread()
{
   Debug::noft();

   //  The running thread is usually the active thread.  If it isn't,
   //  search the thread registry.
   //
   auto nid = SysThread::RunningThreadId();
   Thread* thr = nullptr;
   auto active = ActiveThread();

   if((active != nullptr) && (active->NativeThreadId() == nid))
   {
      thr = active;
   }
   else
   {
      auto reg = Singleton< ThreadRegistry >::Extant();
      if(reg != nullptr) thr = reg->FindThread(nid);
   }

   return thr;
}

//------------------------------------------------------------------------------

void Thread::FunctionInvoked(fn_name_arg func)
{
   Debug::noft();

   Thread* thr = nullptr;

   //  This handles the following:
   //  (a) Adding FUNC to a trace.
   //  (b) Causing a trap after a thread is scheduled in.
   //  (c) Causing a trap before a thread overflows its stack.
   //
   if(Debug::FcFlags_.test(Debug::TracingActive))
   {
      auto& lock = AccessFtLock();
      if(!lock.test_and_set())
      {
         if(TraceRunningThread(thr))
            FunctionTrace::Capture(func);
         lock.clear();
      }
   }

   if(Debug::FcFlags_.test(Debug::TrapPending))
   {
      if(thr == nullptr) thr = RunningThread(std::nothrow);
      if(thr == nullptr) return;
      thr->TrapCheck();
   }

   if(Debug::FcFlags_.test(Debug::StackChecking))
   {
      if(StackCheckCounter_ <= 1)
      {
         if(thr == nullptr) thr = RunningThread(std::nothrow);
         if(thr == nullptr) return;
         thr->StackCheck();
      }
      else
      {
         --StackCheckCounter_;
      }
   }
}

//------------------------------------------------------------------------------

void Thread::FunctionInvoked(fn_name_arg func, const std::nothrow_t&)
{
   Debug::noft();

   Thread* thr = nullptr;

   if(Debug::FcFlags_.test(Debug::TracingActive))
   {
      auto& lock = AccessFtLock();
      if(!lock.test_and_set())
      {
         if(TraceRunningThread(thr, std::nothrow))
            FunctionTrace::Capture(func);
         lock.clear();
      }
   }
}

//------------------------------------------------------------------------------

BlockingReason Thread::GetBlockingReason() const
{
   return priv_->blocked_;
}

//------------------------------------------------------------------------------

signal_t Thread::GetSignal() const
{
   return priv_->signal_;
}

//------------------------------------------------------------------------------

TraceStatus Thread::GetStatus() const
{
   return priv_->status_;
}

//------------------------------------------------------------------------------

bool Thread::HandleSignal(signal_t sig, uint32_t code)
{
   Debug::ft("Thread.HandleSignal");  //@

   auto thr = RunningThread(std::nothrow);

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

      //  Turn the signal into a standard C++ exception so that it can
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

         if((thr != nullptr) && (TimePoint::Now() < thr->priv_->currEnd_))
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

fn_name Thread_ImmProtect = "Thread.ImmProtect";

void Thread::ImmProtect()
{
   Debug::ft(Thread_ImmProtect);

   if(Restart::GetLevel() >= RestartReboot) return;

   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;

   //  Write-protect the immutable memory segment.  This is used after
   //  ImmUnprotect, so it is an error if underflow would occur.
   //
   if(thr->priv_->immUnprots_ == 0)
   {
      Debug::SwLog(Thread_ImmProtect, "underflow", thr->Tid());
      return;
   }

   if(--thr->priv_->immUnprots_ == 0)
   {
      Memory::Protect(MemImmutable);
   }
}

//------------------------------------------------------------------------------

const uint8_t MaxUnprotectCount = 15;

fn_name Thread_ImmUnprotect = "Thread.ImmUnprotect";

void Thread::ImmUnprotect()
{
   Debug::ft(Thread_ImmUnprotect);

   if(Restart::GetLevel() >= RestartReboot) return;

   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;

   //  Write-enable the immutable memory segment.
   //
   if(thr->priv_->immUnprots_ >= MaxUnprotectCount)
   {
      Debug::SwLog(Thread_ImmUnprotect, "overflow", thr->Tid());
      return;
   }

   if(++thr->priv_->immUnprots_ == 1)
   {
      Memory::Unprotect(MemImmutable);
   }
}

//------------------------------------------------------------------------------

Duration Thread::InitialTime() const
{
   Debug::ft("Thread.InitialTime");

   return ThreadAdmin::RtcTimeout();
}

//------------------------------------------------------------------------------

bool Thread::Interrupt(const Flags& mask)
{
   Debug::ft("Thread.Interrupt");

   if(deleting_) return false;

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
   auto bits = mask.to_ulong();
   priv_->vector_.fetch_or(bits);

   if((priv_->blocked_ == NotBlocked) || (priv_->blocked_ == BlockedOnClock))
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

bool Thread::IsLocked() const
{
   return ((priv_ != nullptr) && (priv_->unpreempts_ > 0));
}

//------------------------------------------------------------------------------

bool Thread::IsScheduled() const
{
   return priv_->waiting_;
}

//------------------------------------------------------------------------------

bool Thread::IsTraceable() const
{
   //  Don't trace a thread that has been explicitly excluded.
   //
   auto trace = CalcStatus(true);
   if(trace == TraceExcluded) return false;

   //  Don't trace a preemptable thread if only counting function invocations.
   //  That capability uses std::map, which isn't thread safe, and we don't
   //  want the overhead of acquiring a lock.
   //
   if(priv_->unpreempts_ == 0)
   {
      if(FunctionTrace::GetScope() == FunctionTrace::CountsOnly) return false;
   }

   switch(faction_)
   {
   case WatchdogFaction:
   case SystemFaction:
      //
      //  Always trace RootThread and InitThread during system initalization
      //  and restarts.
      //
      if(Restart::GetStage() != Running) return true;
   }

   return (trace == TraceIncluded);
}

//------------------------------------------------------------------------------

fixed_string KillRootThread = "The root thread cannot be killed.";
fixed_string KillDeletingThread = "The thread is already being deleted.";

fixed_string Thread::Kill()
{
   Debug::ft("Thread.Kill");

   if(Singleton< RootThread >::Extant() == this) return KillRootThread;
   if(deleting_) return KillDeletingThread;

   //  If the thread is holding or blocked on a mutex, delete it outright.
   //  Otherwise, sending it the signal SIGPURGE will cause it to exit as
   //  soon as it resumes execution and invokes Debug::ft.
   //
   if((priv_->mutexes_ > 0) || (priv_->acquiring_ != nullptr))
      Destroy();
   else
      Raise(SIGPURGE);

   return nullptr;
}

//------------------------------------------------------------------------------

Thread* Thread::LockedThread()
{
   auto thr = ActiveThread();
   if((thr != nullptr) && thr->IsLocked()) return thr;
   return nullptr;
}

//------------------------------------------------------------------------------

void Thread::LogContextSwitch() const
{
   Debug::ft("Thread.LogContextSwitch");

   ThreadAdmin::Incr(ThreadAdmin::Switches);

   auto now = TimePoint::Now();

   if(Singleton< ThreadRegistry >::Extant()->IsDeleted())
   {
      //  This thread has been deleted.  Create a partial entry for it.
      //
      auto rec = Singleton< ContextSwitches >::Instance()->AddSwitch();

      if(rec != nullptr)
      {
         rec->tid = 0;
         rec->nid = SysThread::RunningThreadId();
         rec->in = now;
         rec->out = now;
         rec->locked = false;
      }
   }
   else
   {
      if(stats_ != nullptr)
      {
         stats_->yields_->Incr();
         auto elapsed = now - priv_->currStart_;
         stats_->maxTime_->Update(elapsed.Ticks());
         stats_->totTime_->Add(elapsed.Ticks());
         priv_->currTime_ += elapsed;
      }

      auto rec = Singleton< ContextSwitches >::Instance()->AddSwitch();

      if(rec != nullptr)
      {
         rec->tid = Tid();
         rec->nid = SysThread::RunningThreadId();
         rec->in = priv_->currStart_;
         rec->out = now;
         rec->locked = priv_->locked_;
      }

      priv_->locked_ = false;
   }
}

//------------------------------------------------------------------------------

TraceRc Thread::LogContextSwitches(bool on)
{
   return Singleton< ContextSwitches >::Instance()->LogSwitches(on);
}

//------------------------------------------------------------------------------

bool Thread::LogSignal(signal_t sig) const
{
   Debug::ft("Thread.LogSignal");

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

bool Thread::LogTrap(const Exception* ex,
   const std::exception* e, signal_t sig, const std::ostringstream* stack)
{
   Debug::ft("Thread.LogTrap");

   auto reg = Singleton< PosixSignalRegistry >::Instance();
   if(reg->Attrs(sig).test(PosixSignal::NoError)) return false;

   auto log = Log::Create(ThreadLogGroup, ThreadException);
   if(log == nullptr) return false;

   auto exceeded = false;
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
      }
   }

   if(priv_->recovering_)
   {
      *log << Log::Tab << TrapDuringRecoveryStr << CRLF;
   }

   if(priv_->trapLbc_.HasReachedLimit())
   {
      exceeded = true;
      *log << Log::Tab << TrapLimitReachedStr << CRLF;
   }

   if(stack != nullptr) *log << stack->str();

   //  Log the thread's data if it will be forced to exit.
   //
   if(!priv_->logged_ && reg->Attrs(priv_->signal_).test(PosixSignal::Final))
   {
      priv_->logged_ = true;
      *log << Log::Tab << ThreadDataStr << CRLF;
      Display(*log, Log::Tab + spaces(2), NoFlags);
   }

   Log::Submit(log);
   return exceeded;
}

//------------------------------------------------------------------------------

void Thread::MakePreemptable()
{
   Debug::ftnt("Thread.MakePreemptable");

   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;

   //  If the thread is already preemptable, nothing needs to be done.
   //  If it just become preemptable, schedule it out.
   //
   if(thr->priv_->unpreempts_ == 0) return;
   if(--thr->priv_->unpreempts_ == 0) Pause();
}

//------------------------------------------------------------------------------

const uint8_t MaxUnpreemptCount = 15;

fn_name Thread_MakeUnpreemptable = "Thread.MakeUnpreemptable";

void Thread::MakeUnpreemptable()
{
   Debug::ftnt(Thread_MakeUnpreemptable);

   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;

   //  Increment the unpreemptable count.  If the thread has just become
   //  unpreemptable, schedule it out before starting to run it locked.
   //
   if(thr->priv_->unpreempts_ >= MaxUnpreemptCount)
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
   Debug::ftnt(Thread_MemProtect);

   if(Restart::GetLevel() >= RestartReload) return;

   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;

   //  Write-protect the protected memory segment.  This is used after
   //  MemUnprotect, so it is an error if underflow would occur.
   //
   if(thr->priv_->memUnprots_ == 0)
   {
      Debug::SwLog(Thread_MemProtect, "underflow", thr->Tid());
      return;
   }

   if(--thr->priv_->memUnprots_ == 0)
   {
      Memory::Protect(MemProtected);
   }
}

//------------------------------------------------------------------------------

fn_name Thread_MemUnprotect = "Thread.MemUnprotect";

void Thread::MemUnprotect()
{
   Debug::ftnt(Thread_MemUnprotect);

   if(Restart::GetLevel() >= RestartReload) return;

   auto thr = RunningThread(std::nothrow);
   if(thr == nullptr) return;

   //  Write-enable the protected memory segment.
   //
   if(thr->priv_->memUnprots_ >= MaxUnprotectCount)
   {
      Debug::SwLog(Thread_MemUnprotect, "overflow", thr->Tid());
      return;
   }

   if(++thr->priv_->memUnprots_ == 1)
   {
      Memory::Unprotect(MemProtected);
   }
}

//------------------------------------------------------------------------------

uint8_t Thread::MutexCount() const
{
   return priv_->mutexes_;
}

//------------------------------------------------------------------------------

SysThreadId Thread::NativeThreadId() const
{
   Debug::noft();

   if(systhrd_ != nullptr) return systhrd_->Nid();
   return NIL_ID;
}

//------------------------------------------------------------------------------

void Thread::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Thread_Pause = "Thread.Pause";

DelayRc Thread::Pause(Duration time)
{
   Trace(nullptr, Thread_Pause, ThreadTrace::PauseEnter, time.Ticks());

   auto drc = DelayCompleted;
   auto thr = RunningThread();

   //  See if the thread should be forced to sleep indefinitely.  This occurs
   //  o during the execution of Unblock(), which could be deleting some of
   //    the thread's resources;
   //  o when the thread decided to survive a restart instead of exiting.
   //
   if(thr->priv_->action_ == SleepThread)
   {
      time = TIMEOUT_NEVER;
   }

   if(EnterBlockingOperation(BlockedOnClock, Thread_Pause))
   {
      if(time != TIMEOUT_IMMED) drc = thr->systhrd_->Delay(time);
      ExitBlockingOperation(Thread_Pause);
   }
   else
   {
      if(time != TIMEOUT_IMMED) drc = DelayInterrupted;
   }

   Trace(thr, Thread_Pause, ThreadTrace::PauseExit, drc);
   return drc;
}

//------------------------------------------------------------------------------

void Thread::PauseOver(word limit)
{
   Debug::ft("Thread.PauseOver");

   if(RtcPercentUsed() >= limit) Pause();
}

//------------------------------------------------------------------------------

double Thread::PercentIdle()
{
   if(TimeIdle_ == ZERO_SECS) return 0.0;
   auto total = TimeIdle_ + TimeUsed_;
   return 100 * (double(TimeIdle_.Ticks()) / total.Ticks());
}

//------------------------------------------------------------------------------

void Thread::Preempt()
{
   Debug::ft("Thread.Preempt");

   //  Set the thread's ready time so that it will later be reselected,
   //  and lower its priority so that the platform won't schedule it in.
   //
   priv_->readyTime_ = TimePoint::Now();
   systhrd_->SetPriority(SysThread::LowPriority);
   ThreadAdmin::Incr(ThreadAdmin::Preempts);
}

//------------------------------------------------------------------------------

void Thread::Proceed()
{
   Debug::ft("Thread.Proceed");

   //  Unless a restart runs with unprotected memory, update memory protection
   //  to what the thread expects.  Ensure that its priority is such that the
   //  platform will schedule it in, and signal it to resume.
   //
   auto level = Restart::GetLevel();

   if(level < RestartReload)
   {
      if(priv_->memUnprots_ == 0)
         Memory::Protect(MemProtected);
      else
         Memory::Unprotect(MemProtected);
   }

   if(level < RestartReboot)
   {
      if(priv_->immUnprots_ == 0)
         Memory::Protect(MemImmutable);
      else
         Memory::Unprotect(MemImmutable);
   }

   systhrd_->SetPriority(SysThread::DefaultPriority);
   if(priv_->waiting_) systhrd_->Proceed();
}

//------------------------------------------------------------------------------

fn_name Thread_Raise = "Thread.Raise";

void Thread::Raise(signal_t sig)
{
   Debug::ft(Thread_Raise);

   //  Ensure that SIG is valid.
   //
   auto reg = Singleton< PosixSignalRegistry >::Extant();
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
   auto thr = RunningThread(std::nothrow);

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
   if(ps1->Attrs().test(PosixSignal::Final))
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

void Thread::Ready()
{
   priv_->currStart_ = TimePoint::Now();

   Debug::ft("Thread.Ready");

   if(faction_ >= SystemFaction) return;

   //  Record the time when the thread became ready to run.  If no thread
   //  is currently active, wake InitThread to schedule this thread in,
   //  but have it wait to be signalled before it runs.
   //
   priv_->readyTime_ = TimePoint::Now();
   priv_->waiting_ = true;

   if(ActiveThread() == nullptr)
   {
      Singleton< InitThread >::Instance()->Interrupt(InitThread::ScheduleMask);
   }

   systhrd_->Wait();
   priv_->waiting_ = false;
   priv_->currStart_ = TimePoint::Now();
   priv_->locked_ = (priv_->unpreempts_ > 0);
}

//------------------------------------------------------------------------------

bool Thread::Recover()
{
   Debug::ft("Thread.Recover");

   return true;
}

//------------------------------------------------------------------------------

void Thread::RegisterForSignals()
{
   Debug::ft("Thread.RegisterForSignals");

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

void Thread::ReleaseResources(bool orphaned)
{
   Debug::ft("Thread.ReleaseResources");

   //  Setting deleting_ prevents any attempt to come through here twice and
   //  prevents the thread from being accessed remotely while being deleted.
   //
   if(deleting_) return;
   deleting_ = true;

   //  Void the thread's message queue.  It may have trapped because of
   //  a corrupt message queue, so let the object pool audit recover any
   //  messages queued against it.
   //
   msgq_.Init(Pooled::LinkDiff());

   //  If a restart is underway, release any object whose heap will be
   //  deleted.  This saves time in our destructor, which deletes this
   //  object before returning if we haven't released it.
   //
   Restart::Release(stats_);

   //  If the thread is about to exit, remove it from the registry, else
   //  register its native thread as an orphan.  Various functions invoke
   //  GetState to check for the existence of an orphaned native thread,
   //  which is immediately exited when found.
   //
   auto threads = Singleton< ThreadRegistry >::Extant();

   if(orphaned)
   {
      threads->Destroying(Deleted, systhrd_.release());
   }
   else
   {
      auto nid = (systhrd_ != nullptr ? systhrd_->Nid() : NIL_ID);
      systhrd_.reset();
      if(nid != NIL_ID) threads->Erase(nid);
   }

   //  If the thread has a daemon, notify it of the deletion so that it can
   //  recreate the thread.
   //
   if(daemon_ != nullptr) daemon_->ThreadDeleted(this);

   //  This can no longer be the active thread.
   //
   ClearActiveThread(this);
}

//------------------------------------------------------------------------------

void Thread::Reset(FlagId fid)
{
   Debug::ft("Thread.Reset");

   uint32_t mask = (1 << fid);
   priv_->vector_.fetch_and(~mask);
}

//------------------------------------------------------------------------------

void Thread::ResetDebugFlags()
{
   AccessFtLock().clear();
   ExitSwLog(true);
}

//------------------------------------------------------------------------------

void Thread::ResetFlag(FlagId fid)
{
   Debug::ft("Thread.ResetFlag");

   RunningThread()->Reset(fid);
}

//------------------------------------------------------------------------------

void Thread::ResetFlags()
{
   Debug::ft("Thread.ResetFlags");

   RunningThread()->priv_->vector_.store(0);
}

//------------------------------------------------------------------------------

bool Thread::Restarting(RestartLevel level)
{
   Debug::ft("Thread.Restarting");

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

void Thread::Resume(fn_name_arg func)
{
   Debug::ft("Thread.Resume");

   //  Set the time before which a locked thread should schedule itself out.
   //
   auto time = InitialTime() << ThreadAdmin::WarpFactor();
   if(!priv_->entered_) time <<= 2;
   priv_->currEnd_ = priv_->currStart_ + time;
   priv_->warned_ = false;

   if(priv_->unpreempts_ > 0) ThreadAdmin::Incr(ThreadAdmin::Locks);
   ScheduledIn(func);
}

//------------------------------------------------------------------------------

word Thread::RtcPercentUsed()
{
   Debug::ft("Thread.RtcPercentUsed");

   //  This returns 0 unless the thread is running unpreemptably.
   //
   auto thr = RunningThread();
   if(!thr->IsLocked()) return 0;

   auto used = TimePoint::Now() - thr->priv_->currStart_;
   auto full = thr->priv_->currEnd_ - thr->priv_->currStart_;

   if(used < full) return ((100 * used) / full);
   return 100;
}

//------------------------------------------------------------------------------

void Thread::RtcTimeout()
{
   Debug::ft("Thread.RtcTimeout");

   if(stats_ != nullptr) stats_->exceeds_->Incr();

   if(priv_->rtcLbc_.HasReachedLimit())
   {
      Raise(SIGYIELD);
   }
}

//------------------------------------------------------------------------------

Thread* Thread::RunningThread()
{
   Debug::noft();

   auto thr = FindRunningThread();
   if(thr != nullptr) return thr;

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

   if(Singleton< ThreadRegistry >::Instance()->GetState() == Deleted)
      throw SignalException(SIGDELETED, 0);
   else
      Debug::Assert(false);

   return nullptr;
}

//------------------------------------------------------------------------------

Thread* Thread::RunningThread(const std::nothrow_t&)
{
   Debug::noft();

   auto thr = FindRunningThread();
   if(thr == nullptr) ThreadAdmin::Incr(ThreadAdmin::Unknowns);
   return thr;
}

//------------------------------------------------------------------------------

void Thread::Schedule()
{
   Debug::ft("Thread.Schedule");

   //  Scheduling only occurs among application threads.
   //
   if(faction_ >= SystemFaction) return;

   auto active = this;

   if(!ActiveThread_.compare_exchange_strong(active, nullptr))
   {
      //  This occurs when a preemptable thread suspends or invokes
      //  MakeUnpreemptable.  The active thread is an unpreemptable
      //  thread, so don't try to schedule another one.
      //
      return;
   }

   //  No unpreemptable thread is running.  Wake InitThread to schedule
   //  the next thread.
   //
   Singleton< InitThread >::Instance()->Interrupt(InitThread::ScheduleMask);
}

//------------------------------------------------------------------------------

void Thread::SetInitialized()
{
   Debug::ft("Thread.SetInitialized");

   Singleton< ThreadRegistry >::Instance()->Initialized(systhrd_->Nid());
}

//------------------------------------------------------------------------------

void Thread::SetSignal(signal_t sig)
{
   Debug::ft("Thread.SetSignal");

   priv_->signal_ = sig;
}

//------------------------------------------------------------------------------

void Thread::SetStatus(TraceStatus status)
{
   priv_->status_ = status;
}

//------------------------------------------------------------------------------

void Thread::SetTrap(bool on)
{
   Debug::ft("Thread.SetTrap");

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

      for(auto t = threads.cbegin(); t != threads.cend(); ++t)
      {
         auto thr = t->second.thread_;
         if((thr != nullptr) && thr->priv_->trap_) return;
      }

      Debug::FcFlags_.reset(Debug::TrapPending);
   }
}

//------------------------------------------------------------------------------

void Thread::Shutdown(RestartLevel level)
{
   Debug::ft("Thread.Shutdown");

   Restart::Release(stats_);

   auto pool = Singleton< MsgBufferPool >::Instance();
   if(!Restart::ClearsMemory(pool->BlockType())) return;

   //  The thread's messages will be deleted during this restart.  Clean
   //  up the messages in case they own objects that they need to free,
   //  and then reinitialize the message queue so that the destructor
   //  will not be invoked for each message.
   //
   for(auto m = msgq_.First(); m != nullptr; msgq_.Next(m))
   {
      m->Cleanup();
   }

   msgq_.Init(Pooled::LinkDiff());
}

//------------------------------------------------------------------------------

void Thread::SignalHandler(signal_t sig)
{
   //  Reenable Debug functions before tracing this function.
   //
   ResetDebugFlags();
   Debug::ft("Thread.SignalHandler");

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

   Pause(Duration(2, SECS));
   signal(sig, SIG_DFL);
   raise(sig);
}

//------------------------------------------------------------------------------

void Thread::StackCheck()
{
   Debug::noft();

   //  Return immediately if stackBase_ has not been initialized.
   //
   if((priv_ == nullptr) || (priv_->stackBase_ == nullptr)) return;

   StackCheckCounter_ = ThreadAdmin::StackCheckInterval();

   signal_t local = SIGNIL;
   ptrdiff_t stacksize = &local - priv_->stackBase_;
   if(stacksize < 0) stacksize = -stacksize;

   if(stacksize > ThreadAdmin::StackUsageLimit())
   {
      //  This function can be invoked from Debug::ft, and SignalException's
      //  constructor also invokes Debug::Ft.  Reinvocations of this function
      //  are therefore blocked to prevent a stack overflow.
      //
      auto& lock = AccessFtLock();
      if(lock.test_and_set()) return;

      priv_->stackBase_ = nullptr;
      throw SignalException(SIGSTACK1, stacksize);
   }

   if(stats_ != nullptr) stats_->maxStack_->Update(stacksize);
}

//------------------------------------------------------------------------------

fn_name Thread_Start = "Thread.Start";

main_t Thread::Start()
{
   auto started = false;

   while(true)
   {
      try
      {
         if(!started)
         {
            //  Immediately register to catch POSIX signals.
            //
            RegisterForSignals();

            //  A thread may start to run before its Thread object is fully
            //  constructed.  This causes a trap, so the thread must wait
            //  until it is constructed.  If its constructor traps, it gets
            //  registered as an orphan, so immediately exit it by returning
            //  SIGDELETED.
            //
            auto reg = Singleton< ThreadRegistry >::Instance();

            while(true)
            {
               auto state = reg->GetState();
               if(state == Constructed) break;
               if(state == Deleted) return SIGDELETED;
            }

            //  Indicate that we're ready to run.  This blocks until we're
            //  scheduled in.  At that point, resume execution.
            //
            Ready();
            Resume(Thread_Start);
            started = true;
         }

         Debug::ft(Thread_Start);

         //  If the thread is preemptable, we got here after handling a trap,
         //  because we make each new thread unpreemptable.  Make the thread
         //  unpreemptable again.
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

         //  See if we got here after a "continue" in a catch clause below.
         //
         switch(priv_->traps_)
         {
         case 0:
            break;

         case 1:
         {
            //  The thread just trapped.  Invoke Recover so the thread can
            //  clean up work in progress and be reentered.  Reset traps_
            //  so that we won't force the thread to exit if Recover traps,
            //  because the problem might be limited to objects used during
            //  the last pass through the thread's work loop.
            //    If recovering_ is *already* set, Recover trapped when it
            //  was invoked, so don't invoke it again.
            //
            auto reenter = true;

            if(!priv_->recovering_)
            {
               priv_->recovering_ = true;
               priv_->traps_ = 0;
               reenter = Recover();
               priv_->traps_ = 1;
            }

            priv_->recovering_ = false;

            if(reenter)
            {
               //  After pausing, reenter the thread by invoking Enter (below).
               //
               SetSignal(SIGNIL);
               ThreadAdmin::Incr(ThreadAdmin::Recoveries);
               Pause();
               priv_->traps_ = 0;
            }
            else
            {
               //  Exit the thread.
               //
               priv_->traps_ = 0;
               return Exit(priv_->signal_);
            }

            break;
         }

         default:
            //
            //  TrapHandler should have prevented us from getting here.
            //
            Debug::SwLog(Thread_Start, "retrapped", priv_->traps_);
            return Exit(priv_->signal_);
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
            Pause(Duration(10, SECS));
            exit(reason);
         }

         auto log = Log::Create(NodeLogGroup, NodeRestart);

         if(log != nullptr)
         {
            *log << Log::Tab << "in " << to_str() << CRLF;
            nex.Display(*log, Log::Tab + spaces(2));
            *log << nex.Stack()->str();
            Log::Submit(log);
         }

         //  RootThread and InitThread handle their own flow of execution when
         //  initiating restarts, so just loop around and reinvoke their Enter
         //  functions.  Other threads must first notify InitThread.
         //
         if(faction_ < SystemFaction)
         {
            Singleton< InitThread >::Instance()->InitiateRestart(nex.Level());
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
         default:
            return sex.GetSignal();
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
         default:
            return SIGDELETED;
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
         default:
            return SIGDELETED;
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
         default:
            return SIGDELETED;
         }
      }
   }
}

//------------------------------------------------------------------------------

void Thread::StartShortInterval()
{
   Debug::ft("Thread.StartShortInterval");

   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

   TimeUsed_ = ZERO_SECS;

   for(auto t = threads.cbegin(); t != threads.cend(); ++t)
   {
      auto thr = t->second.thread_;
      if(thr == nullptr) continue;
      TimeUsed_ += thr->priv_->currTime_;
      thr->priv_->prevTime_ = thr->priv_->currTime_;
      thr->priv_->currTime_ = ZERO_SECS;
   }

   PrevIntervalStart_ = CurrIntervalStart_;
   CurrIntervalStart_ = TimePoint::Now();

   //  Until the first short interval ends, there is no "previous" short
   //  interval.
   //
   if(PrevIntervalStart_.IsValid())
   {
      auto elapsed = CurrIntervalStart_ - PrevIntervalStart_;

      if(elapsed > TimeUsed_)
         TimeIdle_ = elapsed - TimeUsed_;
      else
         TimeIdle_ = ZERO_SECS;
   }
}

//------------------------------------------------------------------------------

TraceRc Thread::StartTracing(const string& opts)
{
   auto rc = Singleton< TraceBuffer >::Instance()->StartTracing(opts);

   if(rc == TraceOk)
   {
      auto thr = RunningThread();
      thr->priv_->autostop_ = (opts.find(TraceAutostop) != string::npos);
      thr->priv_->tracing_ = true;
   }

   return rc;
}

//------------------------------------------------------------------------------

void Thread::Startup(RestartLevel level)
{
   Debug::ft("Thread.Startup");

   //  Recreate the thread's statistics if they were deleted.  If the
   //  thread slept during the restart, wake it up.
   //
   if(stats_ == nullptr) stats_.reset(new ThreadStats);

   auto wakeup = (priv_->action_ == SleepThread);
   priv_->action_ = RunThread;
   if(wakeup && (priv_->blocked_ == BlockedOnClock)) Interrupt();
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

void Thread::Suspend()
{
   Debug::ft("Thread.Suspend");

   if(priv_->autostop_) StopTracing();

   if(priv_->warned_)
   {
      auto log = Log::Create(ThreadLogGroup, ThreadYielded);

      if(log != nullptr)
      {
         *log << Log::Tab << "thread=" << to_str();
         auto elapsed = TimePoint::Now() - priv_->currEnd_;
         *log << " overrun=" << elapsed.to_str(mSECS);
         Log::Submit(log);
      }

      priv_->warned_ = false;
   }

   LogContextSwitch();
   priv_->currEnd_ = TimePoint();
   Schedule();
}

//------------------------------------------------------------------------------

Thread* Thread::SwitchContext()
{
   Debug::ft("Thread.SwitchContext");

   auto curr = ActiveThread();

   if((curr != nullptr) && curr->IsLocked())
   {
      //  This is similar to code in InitThread, where the scheduled thread
      //  occasionally misses its Proceed() and needs to be resignalled.
      //
      if(curr->IsScheduled())
      {
         curr->Proceed();
         ThreadAdmin::Incr(ThreadAdmin::Resignals);
      }
      else
      {
         ThreadAdmin::Incr(ThreadAdmin::Reentries);
      }

      return curr;
   }

   //  Select the next thread to run.  If one is found, preempt any running
   //  thread (which cannot be locked) and signal the next one to resume.
   //
   auto next = Singleton< ThreadRegistry >::Instance()->Select();

   if(next != nullptr)
   {
      if(next == curr)
      {
         ThreadAdmin::Incr(ThreadAdmin::Reselects);
         return curr;
      }

      if(!ActiveThread_.compare_exchange_strong(curr, next))
      {
         //  CURR is no longer the active thread, even though it was when
         //  this function was entered.
         //
         ThreadAdmin::Incr(ThreadAdmin::Retractions);
         return curr;
      }

      if(curr != nullptr) curr->Preempt();
      next->Proceed();
      return next;
   }

   return curr;
}

//------------------------------------------------------------------------------

bool Thread::Test(FlagId fid) const
{
   Debug::ft("Thread.Test");

   auto flags = priv_->vector_.load();
   return ((flags & (1 << fid)) != 0);
}

//------------------------------------------------------------------------------

bool Thread::TestFlag(FlagId fid)
{
   Debug::ft("Thread.TestFlag");

   return RunningThread()->Test(fid);
}

//------------------------------------------------------------------------------

Duration Thread::TimeLeft() const
{
   Debug::ft("Thread.TimeLeft");

   //  currEnd_ is zeroed just before yielding.  This prevents its previous
   //  value from being used during the brief interval in which the thread
   //  has again been scheduled to run unpreemptably but currEnd_ has not
   //  been recalculated.
   //
   if(!priv_->currEnd_.IsValid()) return InitialTime();
   auto time = priv_->currEnd_ - TimePoint::Now();
   if(time.Ticks() <= 0) return ZERO_SECS;
   return time;
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

void Thread::Trace(Thread* thr,
   fn_name_arg func, TraceRecordId rid, int32_t info)
{
   if(thr == nullptr) thr = RunningThread(std::nothrow);
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
      thr = RunningThread(std::nothrow);
      if(thr == nullptr) return true;
   }

   return thr->IsTraceable();
}

//------------------------------------------------------------------------------

bool Thread::TraceRunningThread(Thread*& thr, const std::nothrow_t&)
{
   //  Do not trace this thread if the trace buffer is locked or
   //  function tracing is not on.
   //
   auto buff = Singleton< TraceBuffer >::Extant();
   if(buff == nullptr) return false;
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
      thr = RunningThread(std::nothrow);
      if(thr == nullptr) return true;
   }

   return thr->IsTraceable();
}

//------------------------------------------------------------------------------

void Thread::TrapCheck()
{
   Debug::noft();

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
   try
   {
      Debug::ft(Thread_TrapHandler);  //@

      //  Reprotect any unprotected memory.
      //
      auto level = Restart::GetLevel();

      if(level < RestartReboot)
      {
         Memory::Protect(MemImmutable);
      }

      if(level < RestartReload)
      {
         Memory::Protect(MemProtected);
      }

      //  If the thread is holding any mutexes, release them.
      //
      Singleton< MutexRegistry >::Instance()->Abandon();

      //  Exit immediately if the Thread has already been deleted.
      //
      if(sig == SIGDELETED) return Return;

      if(Singleton< ThreadRegistry >::Instance()->GetState() != Constructed)
      {
         return Return;
      }

      //  The thread is no longer running with unprotected memory.
      //
      priv_->immUnprots_ = 0;
      priv_->memUnprots_ = 0;

      //  The first time in, save the signal.  After that, we're dealing
      //  with a trap during trap recovery:
      //  o On the second trap, log it and force the thread to exit.
      //  o On the third trap, force the thread to exit.
      //  o On the fourth trap, exit without even deleting the thread.
      //    This will leak its memory, which is better than what seems
      //    to be an infinite loop.
      //
      auto retrapped = false;
      if(Restart::GetStage() == Running) stats_->traps_->Incr();

      switch(++priv_->traps_)
      {
      case 1:
         priv_->logged_ = false;
         SetSignal(sig);
         break;
      case 2:
         retrapped = true;
         break;
      case 3:
         return Release;
      default:
         return Return;
      }

      if((sig == SIGSTACK1) && (systhrd_ != nullptr))
      {
         systhrd_->status_.set(SysThread::StackOverflowed);
      }

      ThreadAdmin::Incr(ThreadAdmin::Traps);

      auto exceeded = LogTrap(ex, e, sig, stack);

      if(Debug::SwFlagOn(ThreadRetrapFlag))
      {
         CauseTrap();
      }

      //  Force the thread to exit if
      //  o it has trapped too many times
      //  o it trapped during trap recovery
      //  o this is a final signal
      //  In the first two cases, leave trapped_ on so that Exit will log a
      //  forced exit.
      //
      auto sigAttrs = Singleton< PosixSignalRegistry >::Instance()->Attrs(sig);

      if(exceeded || retrapped || sigAttrs.test(PosixSignal::Final))
      {
         if(!sigAttrs.test(PosixSignal::NoError))
         {
            ThreadAdmin::Incr(ThreadAdmin::Kills);
         }

         if(!retrapped && !exceeded) priv_->traps_ = 0;
         return Release;
      }

      //  Resume execution at the top of Start.
      //
      return Continue;
   }

   //  The following catch an exception during trap recovery and
   //  invoke this function recursively to handle it.
   //
   catch(SignalException& sex)
   {
      switch(TrapHandler(&sex, &sex, sex.GetSignal(), sex.Stack()))
      {
      case Continue:
         Debug::SwLog(Thread_TrapHandler, "continue", 0);
         //  [[fallthrough]]
      case Release:
         return Release;
      case Return:
      default:
         return Return;
      }
   }

   catch(Exception& ex)
   {
      switch(TrapHandler(&ex, &ex, SIGNIL, ex.Stack()))
      {
      case Continue:
         Debug::SwLog(Thread_TrapHandler, "continue", 1);
         //  [[fallthrough]]
      case Release:
         return Release;
      case Return:
      default:
         return Return;
      }
   }

   catch(std::exception& e)
   {
      switch(TrapHandler(nullptr, &e, SIGNIL, nullptr))
      {
      case Continue:
         Debug::SwLog(Thread_TrapHandler, "continue", 2);
         //  [[fallthrough]]
      case Release:
         return Release;
      case Return:
      default:
         return Return;
      }
   }

   catch(...)
   {
      switch(TrapHandler(nullptr, nullptr, SIGNIL, nullptr))
      {
      case Continue:
         Debug::SwLog(Thread_TrapHandler, "continue", 2);
         //  [[fallthrough]]
      case Release:
         return Release;
      case Return:
      default:
         return Return;
      }
   }
}

//------------------------------------------------------------------------------

void Thread::Unblock()
{
   Debug::ft("Thread.Unblock");
}

//------------------------------------------------------------------------------

void Thread::UpdateMutex(SysMutex* mutex)
{
   priv_->acquiring_ = mutex;
}

//------------------------------------------------------------------------------

void Thread::UpdateMutexCount(bool acquired)
{
   if(deleting_) return;

   if(acquired)
      ++priv_->mutexes_;
   else
      --priv_->mutexes_;
}

//------------------------------------------------------------------------------

std::atomic_uint32_t* Thread::Vector()
{
   Debug::ft("Thread.Vector");

   return &RunningThread()->priv_->vector_;
}

//------------------------------------------------------------------------------

const Duration ZERO_SECS = Duration::Immed();
const Duration ONE_uSEC = Duration(1, uSECS);
const Duration ONE_mSEC = Duration(1, mSECS);
const Duration ONE_SEC = Duration(1, SECS);
const Duration TIMEOUT_IMMED = Duration::Immed();
const Duration TIMEOUT_NEVER = Duration::Never();
const int64_t TICKS_PER_uSEC = Duration(1, uSECS).Ticks();
const int64_t TICKS_PER_mSEC = Duration(1, mSECS).Ticks();
const int64_t TICKS_PER_SEC = Duration(1, SECS).Ticks();
}
