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
#include "Persistent.h"
#include "StatisticsGroup.h"
#include <bitset>
#include <ostream>
#include <string>
#include "CfgFlagParm.h"
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "InitThread.h"
#include "Restart.h"
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
   CounterPtr interrupts_;
   CounterPtr switches_;
   CounterPtr locks_;
   CounterPtr preempts_;
   CounterPtr delays_;
   CounterPtr resignals_;
   CounterPtr reentries_;
   CounterPtr reselects_;
   CounterPtr retractions_;
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
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
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
   interrupts_.reset(new Counter("interrupts"));
   switches_.reset(new Counter("context switches"));
   locks_.reset(new Counter("scheduled to run locked"));
   preempts_.reset(new Counter("preemptions"));
   delays_.reset(new Counter("scheduled after timeout"));
   resignals_.reset(new Counter("resignaled to proceed"));
   reentries_.reset(new Counter("scheduling interrupt when thread locked"));
   reselects_.reset(new Counter("selected to run again"));
   retractions_.reset(new Counter("race condition between selected threads"));
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
   Debug::ftnt(ThreadsStats_dtor);

   Debug::SwLog(ThreadsStats_dtor, UnexpectedInvocation, 0);
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
   Debug::ftnt(ThreadsStatsGroup_dtor);
}

//------------------------------------------------------------------------------

fn_name ThreadsStatsGroup_DisplayStats = "ThreadsStatsGroup.DisplayStats";

void ThreadsStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft(ThreadsStatsGroup_DisplayStats);

   StatisticsGroup::DisplayStats(stream, id, options);

   Singleton< ThreadAdmin >::Instance()->DisplayStats(stream, options);
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
   Debug::ftnt(BreakEnabledCfg_dtor);
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

struct ThreadAdminValues : public Persistent
{
   //  Initializes configuration parameters to default values.
   //
   ThreadAdminValues() :
      initTimeoutMsecs_(2000),
      schedTimeoutMsecs_(100),
      reinitOnSchedTimeout_(true),
      rtcTimeoutMsecs_(20),
      trapOnRtcTimeout_(true),
      rtcLimit_(6),
      rtcInterval_(60),
      breakEnabled_(false),
      trapLimit_(4),
      trapInterval_(60),
      stackUsageLimit_(8000),
      stackCheckInterval_(1)
   {
   }

   //  See the eponymous public functions for a description of each value.
   //
   word initTimeoutMsecs_;
   word schedTimeoutMsecs_;
   bool reinitOnSchedTimeout_;
   word rtcTimeoutMsecs_;
   bool trapOnRtcTimeout_;
   word rtcLimit_;
   word rtcInterval_;
   bool breakEnabled_;
   word trapLimit_;
   word trapInterval_;
   word stackUsageLimit_;
   word stackCheckInterval_;
};

//  Created before entering main() to provide default values for configuration
//  parameters until they are set from the configuration file.
//
const ThreadAdminValues DefaultAdminValues;

//==============================================================================

fn_name ThreadAdmin_ctor = "ThreadAdmin.ctor";

ThreadAdmin::ThreadAdmin()
{
   Debug::ft(ThreadAdmin_ctor);

   config_.reset(new ThreadAdminValues);
   stats_.reset(new ThreadsStats);
   statsGroup_.reset(new ThreadsStatsGroup);

   auto creg = Singleton< CfgParmRegistry >::Instance();

   initTimeoutMsecs_.reset(new CfgIntParm("InitTimeoutMsecs",
      "10000", &config_->initTimeoutMsecs_, 5000, 180000,
      "restart timeout (msecs)"));
   creg->BindParm(*initTimeoutMsecs_);

   schedTimeoutMsecs_.reset(new CfgIntParm("SchedTimeoutMsecs",
      "50", &config_->schedTimeoutMsecs_, 5, 200,
      "scheduling timeout (msecs)"));
   creg->BindParm(*schedTimeoutMsecs_);

   reinitOnSchedTimeout_.reset(new CfgBoolParm("ReinitOnSchedTimeout",
      "T", &config_->reinitOnSchedTimeout_,
      "set to cause a restart on scheduling timeout"));
   creg->BindParm(*reinitOnSchedTimeout_);

   rtcTimeoutMsecs_.reset(new CfgIntParm("RtcTimeoutMsecs",
      "20", &config_->rtcTimeoutMsecs_, 5, 100,
      "run-to-completion timeout (msecs)"));
   creg->BindParm(*rtcTimeoutMsecs_);

   trapOnRtcTimeout_.reset(new CfgBoolParm("TrapOnRtcTimeout",
      "T", &config_->trapOnRtcTimeout_,
      "set to trap when a thread exceeds the RTC timeout"));
   creg->BindParm(*trapOnRtcTimeout_);

   rtcLimit_.reset(new CfgIntParm("RtcLimit",
      "6", &config_->rtcLimit_, 1, 10,
      "RTC timeouts that cause thread to be trapped"));
   creg->BindParm(*rtcLimit_);

   rtcInterval_.reset(new CfgIntParm("RtcInterval",
      "60", &config_->rtcInterval_, 5, 60,
      "interval (secs) in which to reach RtcLimit"));
   creg->BindParm(*rtcInterval_);

   breakEnabled_.reset(new BreakEnabledCfg(&config_->breakEnabled_));
   creg->BindParm(*breakEnabled_);

   trapLimit_.reset(new CfgIntParm("TrapLimit",
      "4", &config_->trapLimit_, 2, 10,
      "trap count that kills/recreates thread"));
   creg->BindParm(*trapLimit_);

   trapInterval_.reset(new CfgIntParm("TrapInterval",
      "60", &config_->trapInterval_, 5, 300,
      "interval (secs) in which to reach TrapLimit"));
   creg->BindParm(*trapInterval_);

   checkStack_.reset(new CfgFlagParm("CheckStack",
      "F", &Debug::FcFlags_, Debug::StackChecking,
      "set to check stack sizes"));
   creg->BindParm(*checkStack_);

   stackUsageLimit_.reset(new CfgIntParm("StackUsageLimit",
      "8000", &config_->stackUsageLimit_, 4000, 20000,
      "stack usage that traps thread (words)"));
   creg->BindParm(*stackUsageLimit_);

   stackCheckInterval_.reset(new CfgIntParm("StackCheckInterval",
      "10", &config_->stackCheckInterval_, 1, 20,
      "check stack size every nth function call"));
   creg->BindParm(*stackCheckInterval_);
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_dtor = "ThreadAdmin.dtor";

ThreadAdmin::~ThreadAdmin()
{
   Debug::ftnt(ThreadAdmin_dtor);

   Debug::SwLog(ThreadAdmin_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

const ThreadAdminValues* ThreadAdmin::AccessConfig()
{
   if((Restart::GetStage() == ShuttingDown) &&
      (Restart::GetLevel() == RestartReload))
   {
      return nullptr;
   }

   auto admin = Singleton< ThreadAdmin >::Extant();
   if(admin == nullptr) return nullptr;
   return admin->config_.get();
}

//------------------------------------------------------------------------------

bool ThreadAdmin::BreakEnabled()
{
   if(!Element::RunningInLab()) return false;

   auto config = AccessConfig();
   if(config != nullptr) return config->breakEnabled_;
   return DefaultAdminValues.breakEnabled_;
}

//------------------------------------------------------------------------------

void ThreadAdmin::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "InitTimeoutMsecs     : ";
   stream << config_->initTimeoutMsecs_ << CRLF;
   stream << prefix << "SchedTimeoutMsecs    : ";
   stream << config_->schedTimeoutMsecs_ << CRLF;
   stream << prefix << "ReinitOnSchedTimeout : ";
   stream << config_->reinitOnSchedTimeout_ << CRLF;
   stream << prefix << "RtcTimeoutMsecs      : ";
   stream << config_->rtcTimeoutMsecs_ << CRLF;
   stream << prefix << "TrapOnRtcTimeout     : ";
   stream << config_->trapOnRtcTimeout_ << CRLF;
   stream << prefix << "RtcLimit             : ";
   stream << config_->rtcLimit_ << CRLF;
   stream << prefix << "RtcInterval          : ";
   stream << config_->rtcInterval_ << CRLF;
   stream << prefix << "BreakEnabled         : ";
   stream << config_->breakEnabled_ << CRLF;
   stream << prefix << "TrapLimit            : ";
   stream << config_->trapLimit_ << CRLF;
   stream << prefix << "TrapInterval         : ";
   stream << config_->trapInterval_ << CRLF;
   stream << prefix << "StackUsageLimit      : ";
   stream << config_->stackUsageLimit_ << CRLF;
   stream << prefix << "StackCheckInterval   : ";
   stream << config_->stackCheckInterval_ << CRLF;

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

void ThreadAdmin::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft(ThreadAdmin_DisplayStats);

   if(stats_ != nullptr)
   {
      stats_->creations_->DisplayStat(stream, options);
      stats_->deletions_->DisplayStat(stream, options);
      stats_->interrupts_->DisplayStat(stream, options);
      stats_->switches_->DisplayStat(stream, options);
      stats_->locks_->DisplayStat(stream, options);
      stats_->preempts_->DisplayStat(stream, options);
      stats_->delays_->DisplayStat(stream, options);
      stats_->resignals_->DisplayStat(stream, options);
      stats_->reentries_->DisplayStat(stream, options);
      stats_->reselects_->DisplayStat(stream, options);
      stats_->retractions_->DisplayStat(stream, options);
      stats_->traps_->DisplayStat(stream, options);
      stats_->recoveries_->DisplayStat(stream, options);
      stats_->recreations_->DisplayStat(stream, options);
      stats_->orphans_->DisplayStat(stream, options);
      stats_->kills_->DisplayStat(stream, options);
      stats_->unknowns_->DisplayStat(stream, options);
      stats_->unreleased_->DisplayStat(stream, options);
   }
}

//------------------------------------------------------------------------------

void ThreadAdmin::Incr(Register r)
{
   auto admin = Singleton< ThreadAdmin >::Extant();

   if(admin == nullptr) return;
   if(admin->stats_ == nullptr) return;
   if(Restart::GetStage() != Running) return;

   switch(r)
   {
   case Interrupts:
      admin->stats_->interrupts_->Incr();
      break;
   case Switches:
      admin->stats_->switches_->Incr();
      break;
   case Locks:
      admin->stats_->locks_->Incr();
      break;
   case Preempts:
      admin->stats_->preempts_->Incr();
      break;
   case Delays:
      admin->stats_->delays_->Incr();
      break;
   case Resignals:
      admin->stats_->resignals_->Incr();
      break;
   case Reentries:
      admin->stats_->reentries_->Incr();
      break;
   case Reselects:
      admin->stats_->reselects_->Incr();
      break;
   case Retractions:
      admin->stats_->retractions_->Incr();
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

fn_name ThreadAdmin_InitTimeout = "ThreadAdmin.InitTimeout";

Duration ThreadAdmin::InitTimeout()
{
   Debug::ft(ThreadAdmin_InitTimeout);

   auto config = AccessConfig();
   auto msecs = (config != nullptr ?
      config->initTimeoutMsecs_ : DefaultAdminValues.initTimeoutMsecs_);
   return Duration(msecs, mSECS) << WarpFactor();
}

//------------------------------------------------------------------------------

void ThreadAdmin::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool ThreadAdmin::ReinitOnSchedTimeout()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->reinitOnSchedTimeout_;
   return DefaultAdminValues.reinitOnSchedTimeout_;
}

//------------------------------------------------------------------------------

word ThreadAdmin::RtcInterval()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->rtcInterval_;
   return DefaultAdminValues.rtcInterval_;
}

//------------------------------------------------------------------------------

word ThreadAdmin::RtcLimit()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->rtcLimit_;
   return DefaultAdminValues.rtcLimit_;
}

//------------------------------------------------------------------------------

Duration ThreadAdmin::RtcTimeout()
{
   auto config = AccessConfig();
   auto msecs = (config != nullptr ?
      config->rtcTimeoutMsecs_ : DefaultAdminValues.rtcTimeoutMsecs_);
   return Duration(msecs, mSECS);
}

//------------------------------------------------------------------------------

Duration ThreadAdmin::SchedTimeout()
{
   auto config = AccessConfig();
   auto msecs = (config != nullptr ?
      config->schedTimeoutMsecs_ : DefaultAdminValues.schedTimeoutMsecs_);
   return Duration(msecs, mSECS);
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_Shutdown = "ThreadAdmin.Shutdown";

void ThreadAdmin::Shutdown(RestartLevel level)
{
   Debug::ft(ThreadAdmin_Shutdown);

   FunctionGuard guard(Guard_MemUnprotect);
   Restart::Release(stats_);
   Restart::Release(statsGroup_);
}

//------------------------------------------------------------------------------

word ThreadAdmin::StackCheckInterval()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->stackCheckInterval_;
   return DefaultAdminValues.stackCheckInterval_;
}

//------------------------------------------------------------------------------

word ThreadAdmin::StackUsageLimit()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->stackUsageLimit_;
   return DefaultAdminValues.stackUsageLimit_;
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_Startup = "ThreadAdmin.Startup";

void ThreadAdmin::Startup(RestartLevel level)
{
   Debug::ft(ThreadAdmin_Startup);

   FunctionGuard guard(Guard_MemUnprotect);
   if(stats_ == nullptr) stats_.reset(new ThreadsStats);
   if(statsGroup_ == nullptr) statsGroup_.reset(new ThreadsStatsGroup);
   guard.Release();

   //  Define symbols related to threads.
   //
   auto reg = Singleton< SymbolRegistry >::Instance();
   if(!Restart::ClearsMemory(reg->MemType())) return;

   reg->BindSymbol("faction.audit", AuditFaction);
   reg->BindSymbol("faction.bkgd", BackgroundFaction);
   reg->BindSymbol("faction.oper", OperationsFaction);
   reg->BindSymbol("faction.mtce", MaintenanceFaction);
   reg->BindSymbol("faction.payload", PayloadFaction);
   reg->BindSymbol("faction.loadtest", LoadTestFaction);
   reg->BindSymbol("faction.system", SystemFaction);
   reg->BindSymbol("faction.watchdog", WatchdogFaction);
}

//------------------------------------------------------------------------------

word ThreadAdmin::TrapCount()
{
   auto admin = Singleton< ThreadAdmin >::Extant();

   if(admin == nullptr) return 0;
   if(admin->stats_ == nullptr) return 0;
   if(Restart::GetStage() != Running) return 0;
   return admin->stats_->traps_->Overall();
}

//------------------------------------------------------------------------------

word ThreadAdmin::TrapInterval()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->trapInterval_;
   return DefaultAdminValues.trapInterval_;
}

//------------------------------------------------------------------------------

word ThreadAdmin::TrapLimit()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->trapLimit_;
   return DefaultAdminValues.trapLimit_;
}

//------------------------------------------------------------------------------

bool ThreadAdmin::TrapOnRtcTimeout()
{
   auto config = AccessConfig();
   if(config != nullptr) return config->trapOnRtcTimeout_;
   return DefaultAdminValues.trapOnRtcTimeout_;
}

//------------------------------------------------------------------------------

fn_name ThreadAdmin_WarpFactor = "ThreadAdmin.WarpFactor";

int ThreadAdmin::WarpFactor()
{
   Debug::ft(ThreadAdmin_WarpFactor);

   auto warp = 0;

   //  Calculate the time warp factor as follows (to be updated):
   //  o 2x if this is a lab load.
   //  o 4x if the function tracer is on.
   //  o 2x if other tracers are on.
   //  o 2x if immediate tracing is on.
   //
   if(Element::RunningInLab()) warp += 1;

   if(Debug::TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      if(buff->ToolIsOn(FunctionTracer)) warp += 2;
      if(buff->GetTools().count() > 1) warp += 1;
      if(buff->ImmediateTraceOn()) warp += 1;
   }

   return warp;
}
}
