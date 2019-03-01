//==============================================================================
//
//  ThreadAdmin.cpp
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
#include "ThreadAdmin.h"
#include "CfgBoolParm.h"
#include "Dynamic.h"
#include "StatisticsGroup.h"
#include <bitset>
#include <ostream>
#include <string>
#include "CfgFlagParm.h"
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "InitThread.h"
#include "Singleton.h"
#include "Statistics.h"
#include "SymbolRegistry.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Aggregate statistics for threads.
//
class ThreadsStats : public Dynamic
{
public:
   ThreadsStats();
   ~ThreadsStats();

   CounterPtr creations_;
   CounterPtr deletions_;
   CounterPtr switches_;
   CounterPtr locks_;
   CounterPtr interrupts_;
   CounterPtr traps_;
   CounterPtr recoveries_;
   CounterPtr recreations_;
   CounterPtr orphans_;
   CounterPtr kills_;
   CounterPtr unknowns_;
   CounterPtr unreleased_;
};

//  Statistics group for threads.
//
class ThreadsStatsGroup : public StatisticsGroup
{
public:
   ThreadsStatsGroup();
   ~ThreadsStatsGroup();
   void DisplayStats(ostream& stream, id_t id) const override;
};

//  Configuration parameter to allow breakpoint debugging.
//
class BreakEnabledCfg : public CfgBoolParm
{
public:
   explicit BreakEnabledCfg(bool* field);
   ~BreakEnabledCfg();
protected:
   void SetCurr() override;
};

//==============================================================================

fn_name ThreadsStats_ctor = "ThreadsStats.ctor";

ThreadsStats::ThreadsStats()
{
   Debug::ft(ThreadsStats_ctor);

   creations_.reset(new Counter("creations"));
   deletions_.reset(new Counter("deletions"));
   switches_.reset(new Counter("context switches"));
   locks_.reset(new Counter("RTC lock acquisitions"));
   interrupts_.reset(new Counter("interrupts"));
   traps_.reset(new Counter("traps"));
   recoveries_.reset(new Counter("trap recoveries"));
   recreations_.reset(new Counter("re-creations"));
   orphans_.reset(new Counter("orphan detections"));
   kills_.reset(new Counter("kills"));
   unknowns_.reset(new Counter("running thread not found"));
   unreleased_.reset(new Counter("locks recovered by kernel"));
}

//------------------------------------------------------------------------------

fn_name ThreadsStats_dtor = "ThreadsStats.dtor";

ThreadsStats::~ThreadsStats()
{
   Debug::ft(ThreadsStats_dtor);
}

//==============================================================================

fn_name ThreadsStatsGroup_ctor = "ThreadsStatsGroup.ctor";

ThreadsStatsGroup::ThreadsStatsGroup() : StatisticsGroup("Threads (all)")
{
   Debug::ft(ThreadsStatsGroup_ctor);
}

//------------------------------------------------------------------------------

fn_name ThreadsStatsGroup_dtor = "ThreadsStatsGroup.dtor";

ThreadsStatsGroup::~ThreadsStatsGroup()
{
   Debug::ft(ThreadsStatsGroup_dtor);
}

//------------------------------------------------------------------------------

fn_name ThreadsStatsGroup_DisplayStats = "ThreadsStatsGroup.DisplayStats";

void ThreadsStatsGroup::DisplayStats(ostream& stream, id_t id) const
{
   Debug::ft(ThreadsStatsGroup_DisplayStats);

   StatisticsGroup::DisplayStats(stream, id);

   Singleton< ThreadAdmin >::Instance()->DisplayStats(stream);
}

//==============================================================================

fn_name BreakEnabledCfg_ctor = "BreakEnabledCfg.ctor";

BreakEnabledCfg::BreakEnabledCfg(bool* field) :
   CfgBoolParm("BreakEnabled", "F", field, "set to use breakpoint debugging")
{
   Debug::ft(BreakEnabledCfg_ctor);
}

//------------------------------------------------------------------------------

fn_name BreakEnabledCfg_dtor = "BreakEnabledCfg.dtor";

BreakEnabledCfg::~BreakEnabledCfg()
{
   Debug::ft(BreakEnabledCfg_dtor);
}

//------------------------------------------------------------------------------

fn_name BreakEnabledCfg_SetCurr = "BreakEnabledCfg.SetCurr";

void BreakEnabledCfg::SetCurr()
{
   Debug::ft(BreakEnabledCfg_SetCurr);

   auto curr = GetCurrValue();
   auto next = GetNextValue();

   CfgBoolParm::SetCurr();

   //  If breakpoint debugging was previously enabled and has now been
   //  disabled, InitThread is sleeping forever.  Awaken it so that it
   //  can handle the scheduling and RTC timeouts again.
   //
   if(curr && !next)
   {
      Singleton< InitThread >::Instance()->Interrupt();
   }
}

//==============================================================================

word ThreadAdmin::InitTimeoutMsecs_     = 2000;
word ThreadAdmin::SchedTimeoutMsecs_    = 100;
bool ThreadAdmin::ReinitOnSchedTimeout_ = true;
word ThreadAdmin::RtcTimeoutMsecs_      = 20;
bool ThreadAdmin::TrapOnRtcTimeout_     = true;
word ThreadAdmin::RtcLimit_             = 6;
word ThreadAdmin::RtcInterval_          = 60;
bool ThreadAdmin::BreakEnabled_         = false;
word ThreadAdmin::TrapLimit_            = 4;
word ThreadAdmin::TrapInterval_         = 60;
word ThreadAdmin::StackUsageLimit_      = 6000;
word ThreadAdmin::StackCheckInterval_   = 1;

//------------------------------------------------------------------------------

fn_name ThreadAdmin_ctor = "ThreadAdmin.ctor";

ThreadAdmin::ThreadAdmin()
{
   Debug::ft(ThreadAdmin_ctor);

   stats_.reset(new ThreadsStats);
   statsGroup_.reset(new ThreadsStatsGroup);

   auto creg = Singleton< CfgParmRegistry >::Instance();

   initTimeoutMsecs_.reset(new CfgIntParm("InitTimeoutMsecs", "10000",
      &InitTimeoutMsecs_, 5000, 180000, "restart timeout (msecs)"));
   creg->BindParm(*initTimeoutMsecs_);

   schedTimeoutMsecs_.reset(new CfgIntParm("SchedTimeoutMsecs", "250",
      &SchedTimeoutMsecs_, 200, 1000, "scheduling timeout (msecs)"));
   creg->BindParm(*schedTimeoutMsecs_);

   reinitOnSchedTimeout_.reset(new CfgBoolParm("ReinitOnSchedTimeout", "T",
      &ReinitOnSchedTimeout_, "set to cause a restart on scheduling timeout"));
   creg->BindParm(*reinitOnSchedTimeout_);

   rtcTimeoutMsecs_.reset(new CfgIntParm("RtcTimeoutMsecs", "20",
      &RtcTimeoutMsecs_, 5, 100, "run-to-completion timeout (msecs)"));
   creg->BindParm(*rtcTimeoutMsecs_);

   trapOnRtcTimeout_.reset(new CfgBoolParm("TrapOnRtcTimeout", "T",
      &TrapOnRtcTimeout_, "set to trap when a thread exceeds the RTC timeout"));
   creg->BindParm(*trapOnRtcTimeout_);

   rtcLimit_.reset(new CfgIntParm("RtcLimit", "6",
      &RtcLimit_, 1, 10, "RTC timeouts that cause thread to be trapped"));
   creg->BindParm(*rtcLimit_);

   rtcInterval_.reset(new CfgIntParm("RtcInterval", "60",
      &RtcInterval_, 5, 60, "interval in which to reach RtcLimit (secs)"));
   creg->BindParm(*rtcInterval_);

   breakEnabled_.reset(new BreakEnabledCfg(&BreakEnabled_));
   creg->BindParm(*breakEnabled_);

   trapLimit_.reset(new CfgIntParm("TrapLimit", "4",
      &TrapLimit_, 2, 10, "trap count that kills/recreates thread"));
   creg->BindParm(*trapLimit_);

   trapInterval_.reset(new CfgIntParm("TrapInterval", "60",
      &TrapInterval_, 5, 300, "interval in which to reach TrapLimit (secs)"));
   creg->BindParm(*trapInterval_);

   checkStack_.reset(new CfgFlagParm("CheckStack", "F",
      &Debug::FcFlags_, Debug::StackChecking, "set to enable stack checking"));
   creg->BindParm(*checkStack_);

   stackUsageLimit_.reset(new CfgIntParm("StackUsageLimit", "6000",
      &StackUsageLimit_, 4000, 20000, "stack usage that traps thread (words)"));
   creg->BindParm(*stackUsageLimit_);

   stackCheckInterval_.reset(new CfgIntParm("StackCheckInterval", "10",
      &StackCheckInterval_, 1, 20, "check stack on every nth function call"));
   creg->BindParm(*stackCheckInterval_);
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_dtor = "ThreadAdmin.dtor";

ThreadAdmin::~ThreadAdmin()
{
   Debug::ft(ThreadAdmin_dtor);
}

//------------------------------------------------------------------------------

bool ThreadAdmin::BreakEnabled()
{
   if(Element::RunningInLab()) return BreakEnabled_;
   return false;
}

//------------------------------------------------------------------------------

void ThreadAdmin::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "InitTimeoutMsecs     : ";
   stream << InitTimeoutMsecs_ << CRLF;
   stream << prefix << "SchedTimeoutMsecs    : ";
   stream << SchedTimeoutMsecs_ << CRLF;
   stream << prefix << "ReinitOnSchedTimeout : ";
   stream << ReinitOnSchedTimeout_ << CRLF;
   stream << prefix << "RtcTimeoutMsecs      : ";
   stream << RtcTimeoutMsecs_ << CRLF;
   stream << prefix << "TrapOnRtcTimeout     : ";
   stream << TrapOnRtcTimeout_ << CRLF;
   stream << prefix << "RtcLimit             : ";
   stream << RtcLimit_ << CRLF;
   stream << prefix << "RtcInterval          : ";
   stream << RtcInterval_ << CRLF;
   stream << prefix << "BreakEnabled         : ";
   stream << BreakEnabled_ << CRLF;
   stream << prefix << "TrapLimit            : ";
   stream << TrapLimit_ << CRLF;
   stream << prefix << "TrapInterval         : ";
   stream << TrapInterval_ << CRLF;
   stream << prefix << "StackUsageLimit      : ";
   stream << StackUsageLimit_ << CRLF;
   stream << prefix << "StackCheckInterval   : ";
   stream << StackCheckInterval_ << CRLF;

   stream << prefix << "initTimeoutMsecs     : ";
   stream << strObj(initTimeoutMsecs_.get()) << CRLF;
   stream << prefix << "schedTimeoutMsecs    : ";
   stream << strObj(schedTimeoutMsecs_.get()) << CRLF;
   stream << prefix << "reinitOnSchedTimeout : ";
   stream << strObj(reinitOnSchedTimeout_.get()) << CRLF;
   stream << prefix << "rtcTimeoutMsecs      : ";
   stream << strObj(rtcTimeoutMsecs_.get()) << CRLF;
   stream << prefix << "trapOnRtcTimeout     : ";
   stream << strObj(trapOnRtcTimeout_.get()) << CRLF;
   stream << prefix << "rtcLimit             : ";
   stream << strObj(rtcLimit_.get()) << CRLF;
   stream << prefix << "rtcInterval          : ";
   stream << strObj(rtcInterval_.get()) << CRLF;
   stream << prefix << "breakEnabled         : ";
   stream << strObj(breakEnabled_.get()) << CRLF;
   stream << prefix << "trapLimit            : ";
   stream << strObj(trapLimit_.get()) << CRLF;
   stream << prefix << "trapInterval         : ";
   stream << strObj(trapInterval_.get()) << CRLF;
   stream << prefix << "checkStack           : ";
   stream << strObj(checkStack_.get()) << CRLF;
   stream << prefix << "stackUsageLimit      : ";
   stream << strObj(stackUsageLimit_.get()) << CRLF;
   stream << prefix << "stackCheckInterval   : ";
   stream << strObj(stackCheckInterval_.get()) << CRLF;

   stream << prefix << "statsGroup           : ";
   stream << strObj(statsGroup_.get()) << CRLF;
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_DisplayStats = "ThreadAdmin.DisplayStats";

void ThreadAdmin::DisplayStats(ostream& stream) const
{
   Debug::ft(ThreadAdmin_DisplayStats);

   if(stats_ != nullptr)
   {
      stats_->creations_->DisplayStat(stream);
      stats_->deletions_->DisplayStat(stream);
      stats_->switches_->DisplayStat(stream);
      stats_->locks_->DisplayStat(stream);
      stats_->interrupts_->DisplayStat(stream);
      stats_->traps_->DisplayStat(stream);
      stats_->recoveries_->DisplayStat(stream);
      stats_->recreations_->DisplayStat(stream);
      stats_->orphans_->DisplayStat(stream);
      stats_->kills_->DisplayStat(stream);
      stats_->unknowns_->DisplayStat(stream);
      stats_->unreleased_->DisplayStat(stream);
   }
}

//------------------------------------------------------------------------------

void ThreadAdmin::Incr(Register r)
{
   auto admin = Singleton< ThreadAdmin >::Extant();

   if(admin == nullptr) return;

   if(admin->stats_ == nullptr) return;

   switch(r)
   {
   case Switches:
      admin->stats_->switches_->Incr();
      break;
   case Interrupts:
      admin->stats_->interrupts_->Incr();
      break;
   case Locks:
      admin->stats_->locks_->Incr();
      break;
   case Creations:
      admin->stats_->creations_->Incr();
      break;
   case Deletions:
      admin->stats_->deletions_->Incr();
      break;
   case Traps:
      admin->stats_->traps_->Incr();
      break;
   case Recoveries:
      admin->stats_->recoveries_->Incr();
      break;
   case Recreations:
      admin->stats_->recreations_->Incr();
      break;
   case Orphans:
      admin->stats_->orphans_->Incr();
      break;
   case Kills:
      admin->stats_->kills_->Incr();
      break;
   case Unknowns:
      admin->stats_->unknowns_->Incr();
      break;
   case Unreleased:
      admin->stats_->unreleased_->Incr();
      break;
   }
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_InitTimeoutMsecs = "ThreadAdmin.InitTimeoutMsecs";

msecs_t ThreadAdmin::InitTimeoutMsecs()
{
   Debug::ft(ThreadAdmin_InitTimeoutMsecs);

   return InitTimeoutMsecs_ << WarpFactor();
}

//------------------------------------------------------------------------------

void ThreadAdmin::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_Shutdown = "ThreadAdmin.Shutdown";

void ThreadAdmin::Shutdown(RestartLevel level)
{
   Debug::ft(ThreadAdmin_Shutdown);

   if(level < RestartCold) return;

   stats_.release();
   statsGroup_.release();
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_Startup = "ThreadAdmin.Startup";

void ThreadAdmin::Startup(RestartLevel level)
{
   Debug::ft(ThreadAdmin_Startup);

   //  Define symbols related to threads.
   //
   if(level < RestartCold) return;

   if(stats_ == nullptr) stats_.reset(new ThreadsStats);
   if(statsGroup_ == nullptr) statsGroup_.reset(new ThreadsStatsGroup);

   auto reg = Singleton< SymbolRegistry >::Instance();
   reg->BindSymbol("faction.audit", AuditFaction);
   reg->BindSymbol("faction.bkgd", BackgroundFaction);
   reg->BindSymbol("faction.oper", OperationsFaction);
   reg->BindSymbol("faction.mtce", MaintenanceFaction);
   reg->BindSymbol("faction.payload", PayloadFaction);
   reg->BindSymbol("faction.system", SystemFaction);
   reg->BindSymbol("faction.watchdog", WatchdogFaction);
}

//------------------------------------------------------------------------------

word ThreadAdmin::TrapCount()
{
   auto admin = Singleton< ThreadAdmin >::Extant();

   if(admin == nullptr) return 0;
   if(admin->stats_ == nullptr) return 0;
   return admin->stats_->traps_->Overall();
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_WarpFactor = "ThreadAdmin.WarpFactor";

int ThreadAdmin::WarpFactor()
{
   Debug::ft(ThreadAdmin_WarpFactor);

   auto warp = 0;

   //e Calculate the time warp factor as follows (to be updated):
   //  o 2x if this is a lab load.
   //  o 32x if the function tracer is on.
   //  o 2x if other tracers is on.
   //  o 2x if immediate tracing is on.
   //
   if(Element::RunningInLab()) warp += 1;

   if(Debug::TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Instance();

      if(buff->ToolIsOn(FunctionTracer))
         warp += 5;
      else
         warp += 1;

      if(buff->ImmediateTraceOn()) warp += 1;
   }

   return warp;
}
}
