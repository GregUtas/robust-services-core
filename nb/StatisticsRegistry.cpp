//==============================================================================
//
//  StatisticsRegistry.cpp
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
#include "StatisticsRegistry.h"
#include <cstddef>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Statistics.h"
#include "StatisticsGroup.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum number of statistics that can register.
//
constexpr size_t MaxStats = 1000;

//> The maximum number of groups that can register.
//
constexpr size_t MaxGroups = 100;

TimePoint StatisticsRegistry::StartTime_ = TimePoint();

//------------------------------------------------------------------------------

StatisticsRegistry::StatisticsRegistry()
{
   Debug::ft("StatisticsRegistry.ctor");

   stats_.Init(MaxStats, Statistic::CellDiff(), MemDynamic);
   groups_.Init(MaxGroups, StatisticsGroup::CellDiff(), MemDynamic);

   StartTime_ = TimePoint();
}

//------------------------------------------------------------------------------

fn_name StatisticsRegistry_dtor = "StatisticsRegistry.dtor";

StatisticsRegistry::~StatisticsRegistry()
{
   Debug::ftnt(StatisticsRegistry_dtor);

   Debug::SwLog(StatisticsRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool StatisticsRegistry::BindGroup(StatisticsGroup& group)
{
   Debug::ft("StatisticsRegistry.BindGroup");

   return groups_.Insert(group);
}

//------------------------------------------------------------------------------

bool StatisticsRegistry::BindStat(Statistic& stat)
{
   Debug::ft("StatisticsRegistry.BindStat");

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

void StatisticsRegistry::DisplayStats
   (ostream& stream, const Flags& options) const
{
   Debug::ft("StatisticsRegistry.DisplayStats");

   stream << "For reporting period beginning at ";
   stream << StartTime_.to_str() << CRLF;

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

void StatisticsRegistry::StartInterval(bool first)
{
   Debug::ft("StatisticsRegistry.StartInterval");

   for(auto s = stats_.First(); s != nullptr; stats_.Next(s))
   {
      s->StartInterval(first);
   }

   StartTime_ = TimePoint::Now();
}

//------------------------------------------------------------------------------

void StatisticsRegistry::Startup(RestartLevel level)
{
   Debug::ft("StatisticsRegistry.Startup");

   //  If StartTime_ is invalid, the registry has just been constructed or
   //  reconstructed.  Statistics can only start to be accumulated now, so
   //  set StartTicks_ to the current time.
   //
   if(!StartTime_.IsValid())
   {
      StartTime_ = TimePoint::Now();
   }
}

//------------------------------------------------------------------------------

void StatisticsRegistry::UnbindGroup(StatisticsGroup& group)
{
   Debug::ftnt("StatisticsRegistry.UnbindGroup");

   groups_.Erase(group);
}

//------------------------------------------------------------------------------

void StatisticsRegistry::UnbindStat(Statistic& stat)
{
   Debug::ftnt("StatisticsRegistry.UnbindStat");

   stats_.Erase(stat);
}
}
