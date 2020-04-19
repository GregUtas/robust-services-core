//==============================================================================
//
//  StatisticsRegistry.cpp
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
#include "StatisticsRegistry.h"
#include <sstream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "NbLogs.h"
#include "Restart.h"
#include "Statistics.h"
#include "StatisticsGroup.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t StatisticsRegistry::MaxStats = 1000;
const size_t StatisticsRegistry::MaxGroups = 100;
ticks_t StatisticsRegistry::StartTicks_ = 0;

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_ctor = "StatisticsRegistry.ctor";

StatisticsRegistry::StatisticsRegistry()
{
   Debug::ft(StatisticsRegistry_ctor);

   stats_.Init(MaxStats, Statistic::CellDiff(), MemDynamic);
   groups_.Init(MaxGroups, StatisticsGroup::CellDiff(), MemDynamic);

   StartTicks_ = Clock::TicksZero();
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_dtor = "StatisticsRegistry.dtor";

StatisticsRegistry::~StatisticsRegistry()
{
   Debug::ft(StatisticsRegistry_dtor);

   Debug::SwLog(StatisticsRegistry_dtor, UnexpectedInvocation, 0);
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

   auto lead = prefix + spaces(2);
   stream << prefix << "groups [id_t]" << CRLF;

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      stream << lead << strIndex(g->Gid()) << g->Expl() << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_DisplayStats = "StatisticsRegistry.DisplayStats";

void StatisticsRegistry::DisplayStats
   (ostream& stream, const Flags& options) const
{
   Debug::ft(StatisticsRegistry_DisplayStats);

   stream << "For reporting period beginning at ";
   stream << Clock::TicksToTime(StartTicks_) << CRLF;

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      stream << string(StatisticsGroup::ReportWidth, '-') << CRLF;
      g->DisplayStats(stream, 0, options);
   }

   stream << string(StatisticsGroup::ReportWidth, '-') << CRLF;
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

fn_name StatisticsRegistry_Shutdown = "StatisticsRegistry.Shutdown";

void StatisticsRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(StatisticsRegistry_Shutdown);

   //  Generate a statistics report if the registry will disappear
   //  during the restart.
   //
   if(Restart::ClearsMemory(MemType()))
   {
      auto log = Log::Create(StatsLogGroup, StatsReport);

      if(log != nullptr)
      {
         *log << Log::Tab;
         DisplayStats(*log, VerboseOpt);
         Log::Submit(log);
      }
   }
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
   if(Restart::ClearsMemory(MemType()))
   {
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
