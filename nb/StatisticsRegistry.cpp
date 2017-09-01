//==============================================================================
//
//  StatisticsRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "StatisticsRegistry.h"
#include <memory>
#include <ostream>
#include "CfgParmRegistry.h"
#include "CfgStrParm.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "Singleton.h"
#include "Statistics.h"
#include "StatisticsGroup.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t StatisticsRegistry::MaxStats = 1000;
const size_t StatisticsRegistry::MaxGroups = 100;
string StatisticsRegistry::StatsFileName_ = "stats";
ticks_t StatisticsRegistry::StartTicks_ = 0;

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_ctor = "StatisticsRegistry.ctor";

StatisticsRegistry::StatisticsRegistry()
{
   Debug::ft(StatisticsRegistry_ctor);

   stats_.Init(MaxStats + 1, Statistic::CellDiff(), MemDyn);
   groups_.Init(MaxGroups + 1, StatisticsGroup::CellDiff(), MemDyn);

   StartTicks_ = Clock::TicksZero();

   auto reg = Singleton< CfgParmRegistry >::Instance();

   statsFileName_.reset
      (static_cast< CfgFileTimeParm* >(reg->FindParm("StatsFileName")));

   if(statsFileName_ == nullptr)
   {
      statsFileName_.reset(new CfgFileTimeParm("StatsFileName",
         "stats", &StatsFileName_, "name for statistics files"));
      reg->BindParm(*statsFileName_);
   }
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_dtor = "StatisticsRegistry.dtor";

StatisticsRegistry::~StatisticsRegistry()
{
   Debug::ft(StatisticsRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_BindGroup = "StatisticsRegistry.BindGroup";

bool StatisticsRegistry::BindGroup(StatisticsGroup& group)
{
   Debug::ft(StatisticsRegistry_BindGroup);

   return groups_.Insert(group);
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_BindStat = "StatisticsRegistry.BindStat";

bool StatisticsRegistry::BindStat(Statistic& stat)
{
   Debug::ft(StatisticsRegistry_BindStat);

   return stats_.Insert(stat);
}

//------------------------------------------------------------------------------

void StatisticsRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix
      << "StatsFileName : " << StatsFileName_ << CRLF;
   stream << prefix
      << "statsFileName : " << strObj(statsFileName_.get()) << CRLF;

   auto lead = prefix + spaces(2);
   stream << prefix << "groups [StatisticsGroup::Id]" << CRLF;

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      stream << lead << strIndex(g->Gid()) << g->Expl() << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_DisplayStats = "StatisticsRegistry.DisplayStats";

void StatisticsRegistry::DisplayStats(ostream& stream) const
{
   Debug::ft(StatisticsRegistry_DisplayStats);

   stream << "STATISTICS REPORT: " << Element::strTimePlace() << CRLF;
   stream << "for interval beginning at ";
   stream << Clock::TicksToTime(StartTicks_) << CRLF;

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      stream << string(StatisticsGroup::ReportWidth, '-') << CRLF;
      g->DisplayStats(stream, 0);
   }

   stream << string(StatisticsGroup::ReportWidth, '=') << CRLF;
}

//------------------------------------------------------------------------------

StatisticsGroup* StatisticsRegistry::GetGroup(id_t gid) const
{
   return groups_.At(gid);
}

//------------------------------------------------------------------------------

void StatisticsRegistry::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_StartInterval = "StatisticsRegistry.StartInterval";

void StatisticsRegistry::StartInterval(bool first)
{
   Debug::ft(StatisticsRegistry_StartInterval);

   for(auto s = stats_.First(); s != nullptr; stats_.Next(s))
   {
      s->StartInterval(first);
   }

   StartTicks_ = Clock::TicksNow();
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_Startup = "StatisticsRegistry.Startup";

void StatisticsRegistry::Startup(RestartLevel level)
{
   Debug::ft(StatisticsRegistry_Startup);

   //  The registry is reconstructed after a cold or reload restart and sets
   //  StartTicks_ to the system's original boot time.  It needs to be reset
   //  to the current time, given that all statistics have been cleared.
   //
   switch(level)
   {
   case RestartReload:
   case RestartCold:
      StartTicks_ = Clock::TicksNow();
   }
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_UnbindGroup = "StatisticsRegistry.UnbindGroup";

void StatisticsRegistry::UnbindGroup(StatisticsGroup& group)
{
   Debug::ft(StatisticsRegistry_UnbindGroup);

   groups_.Erase(group);
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_UnbindStat = "StatisticsRegistry.UnbindStat";

void StatisticsRegistry::UnbindStat(Statistic& stat)
{
   Debug::ft(StatisticsRegistry_UnbindStat);

   stats_.Erase(stat);
}
}
