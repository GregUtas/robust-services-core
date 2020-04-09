//==============================================================================
//
//  ThreadAdmin.h
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
#ifndef THREADADMIN_H_INCLUDED
#define THREADADMIN_H_INCLUDED

#include "Persistent.h"
#include <iosfwd>
#include "Clock.h"
#include "NbTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   class ThreadsStats;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Defines statistics and configuration parameters used by the Thread class.
//  Logically, these are members of Thread but would clutter its interface.
//
class ThreadAdmin : public Persistent
{
   friend class Singleton< ThreadAdmin >;
public:
   //  Returns the time allowed for the system to initialize.
   //
   static msecs_t InitTimeoutMsecs();

   //  Returns how long InitThread should sleep before interrupting
   //  RootThread to indicate that scheduling is still occurring.
   //
   static msecs_t SchedTimeoutMsecs() { return SchedTimeoutMsecs_; }

   //  Returns true if RootThread should cause a restart if InitThread
   //  fails to indicate that scheduling is still occurring.
   //
   static bool ReinitOnSchedTimeout() { return ReinitOnSchedTimeout_; }

   //  Returns how long a thread can run unpreemptably before yielding.
   //
   static msecs_t RtcTimeoutMsecs() { return RtcTimeoutMsecs_; }

   //  Returns true if InitThread should trap the running thread if
   //  it runs unpreemptably too long.
   //
   static bool TrapOnRtcTimeout() { return TrapOnRtcTimeout_; }

   //  Returns the maximum number of run-to-completion timeouts that
   //  are allowed before a thread is trapped.
   //
   static word RtcLimit() { return RtcLimit_; }

   //  Returns the interval (in seconds) during which a thread must
   //  reach its RTC timeout limit to be trapped.
   //
   static word RtcInterval() { return RtcInterval_; }

   //  Returns true if breakpoint debugging is enabled.  This stops
   //  RootThread and InitThread from timing out and generating logs
   //  or taking more drastic action.
   //
   static bool BreakEnabled();

   //  Returns a shift factor (for use in a << N expression) that
   //  is used to adjust the above timeouts based on overheads such
   //  as running a debug build or enabling trace tools.
   //
   static int WarpFactor();

   //  Returns the maximum number of traps allowed before a thread
   //  is killed and recreated.
   //
   static word TrapLimit() { return TrapLimit_; }

   //  Returns the interval (in seconds) during which a thread must
   //  reach its trap limit for it to be killed and recreated.
   //
   static word TrapInterval() { return TrapInterval_; }

   //  Returns a thread's maximum allowed stack size.
   //
   static word StackUsageLimit() { return StackUsageLimit_; }

   //  Returns the frequency (every nth function call) at which a
   //  stack size check is performed.
   //
   static word StackCheckInterval() { return StackCheckInterval_; }

   //  Identifiers for Counters associated with threads.
   //
   enum Register
   {
      Creations,    // threads created
      Deletions,    // threads deleted
      Switches,     // context switches
      Locks,        // thread started to run unpreemptably
      Preempts,     // thread preempted
      Delays,       // InitThread timed out but found a thread to schedule
      Resignals,    // selected thread had to be resignalled to run
      Reentries,    // asked to schedule but locked thread exists
      Reselects,    // active thread selected to run again
      Retractions,  // another thread became active before the selected thread
      Interrupts,   // thread interrupts
      Traps,        // traps (signals and exceptions)
      Recoveries,   // trap recoveries
      Recreations,  // threads recreated
      Orphans,      // orphans detected
      Kills,        // threads killed
      Unknowns,     // RunningThread returned nullptr
      Unreleased    // exiting thread failed to release a mutex
   };

   //  Increments the Counter specified by R.
   //
   static void Incr(Register r);

   //  Displays statistics.
   //
   void DisplayStats(std::ostream& stream, const Flags& options) const;

   //  Returns the number of traps.
   //
   static word TrapCount();

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ThreadAdmin();

   //  Private because this singleton is not subclassed.
   //
   ~ThreadAdmin();

   //  Aggregate statistics for threads.
   //
   std::unique_ptr< ThreadsStats > stats_;

   //  The statistics group for threads.
   //
   StatisticsGroupPtr statsGroup_;

   //  Thread configuration parameters.
   //
   CfgIntParmPtr  initTimeoutMsecs_;
   CfgIntParmPtr  schedTimeoutMsecs_;
   CfgBoolParmPtr reinitOnSchedTimeout_;
   CfgIntParmPtr  rtcTimeoutMsecs_;
   CfgBoolParmPtr trapOnRtcTimeout_;
   CfgIntParmPtr  rtcLimit_;
   CfgIntParmPtr  rtcInterval_;
   CfgBoolParmPtr breakEnabled_;
   CfgIntParmPtr  trapLimit_;
   CfgIntParmPtr  trapInterval_;
   CfgFlagParmPtr checkStack_;
   CfgIntParmPtr  stackUsageLimit_;
   CfgIntParmPtr  stackCheckInterval_;

   //  See public functions for documentation.
   //
   static word InitTimeoutMsecs_;
   static word SchedTimeoutMsecs_;
   static bool ReinitOnSchedTimeout_;
   static word RtcTimeoutMsecs_;
   static bool TrapOnRtcTimeout_;
   static word RtcLimit_;
   static word RtcInterval_;
   static bool BreakEnabled_;
   static word TrapLimit_;
   static word TrapInterval_;
   static word StackUsageLimit_;
   static word StackCheckInterval_;
};
}
#endif
