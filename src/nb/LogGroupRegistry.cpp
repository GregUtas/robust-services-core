//==============================================================================
//
//  LogGroupRegistry.cpp
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
#include "LogGroupRegistry.h"
#include "StatisticsGroup.h"
#include <bitset>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "LogGroup.h"
#include "NbCliParms.h"
#include "Restart.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
class LogStatsGroup : public StatisticsGroup
{
public:
   LogStatsGroup();
   ~LogStatsGroup();
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
};

//------------------------------------------------------------------------------

LogStatsGroup::LogStatsGroup() : StatisticsGroup("Logs [group id_t]")
{
   Debug::ft("LogStatsGroup.ctor");
}

//------------------------------------------------------------------------------

LogStatsGroup::~LogStatsGroup()
{
   Debug::ftnt("LogStatsGroup.dtor");
}

//------------------------------------------------------------------------------

void LogStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft("LogStatsGroup.DisplayStats");

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton<LogGroupRegistry>::Instance();

   if(id == 0)
   {
      const auto& groups = reg->Groups();

      for(auto g = groups.First(); g != nullptr; groups.Next(g))
      {
         g->DisplayStats(stream, options);
      }
   }
   else
   {
      auto p = reg->Group(id);

      if(p == nullptr)
      {
         stream << spaces(2) << NoLogGroupExpl << CRLF;
         return;
      }

      p->DisplayStats(stream, options);
   }
}

//==============================================================================
//
//> The maximum number of log groups.
//
constexpr id_t MaxGroups = 250;

//------------------------------------------------------------------------------

LogGroupRegistry::LogGroupRegistry()
{
   Debug::ft("LogGroupRegistry.ctor");

   groups_.Init(MaxGroups, LogGroup::CellDiff(), MemImmutable);
   statsGroup_.reset(new LogStatsGroup);
}

//------------------------------------------------------------------------------

fn_name LogGroupRegistry_dtor = "LogGroupRegistry.dtor";

LogGroupRegistry::~LogGroupRegistry()
{
   Debug::ftnt(LogGroupRegistry_dtor);

   Debug::SwLog(LogGroupRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name LogGroupRegistry_BindGroup = "LogGroupRegistry.BindGroup";

bool LogGroupRegistry::BindGroup(LogGroup& group)
{
   Debug::ft(LogGroupRegistry_BindGroup);

   auto curr = FindGroup(group.Name());

   if(curr != nullptr)
   {
      Debug::SwLog(LogGroupRegistry_BindGroup, group.Name(), 0);
      return false;
   }

   return groups_.Insert(group);
}

//------------------------------------------------------------------------------

void LogGroupRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead = prefix + spaces(2);

   stream << prefix << "Groups [id_t]";
   if(options.test(DispVerbose)) stream << " with logs";
   stream << ":" << CRLF;

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      g->Display(stream, lead, options);
   }
}

//------------------------------------------------------------------------------

LogGroup* LogGroupRegistry::FindGroup(const std::string& name) const
{
   Debug::ftnt("LogGroupRegistry.FindGroup");

   auto key = strUpper(name);

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      if(g->Name() == key) return g;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Log* LogGroupRegistry::FindLog(const std::string& name, LogId id) const
{
   Debug::ftnt("LogGroupRegistry.FindLog");

   auto group = FindGroup(name);
   if(group != nullptr) return group->FindLog(id);
   return nullptr;
}

//------------------------------------------------------------------------------

LogGroup* LogGroupRegistry::Group(id_t gid) const
{
   return groups_.At(gid);
}

//------------------------------------------------------------------------------

void LogGroupRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void LogGroupRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("LogGroupRegistry.Shutdown");

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      g->Shutdown(level);
   }

   FunctionGuard guard(Guard_ImmUnprotect);
   Restart::Release(statsGroup_);
}

//------------------------------------------------------------------------------

void LogGroupRegistry::Startup(RestartLevel level)
{
   Debug::ft("LogGroupRegistry.Startup");

   if(statsGroup_ == nullptr)
   {
      FunctionGuard guard(Guard_ImmUnprotect);
      statsGroup_.reset(new LogStatsGroup);
   }

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      g->Startup(level);
   }
}

//------------------------------------------------------------------------------

void LogGroupRegistry::UnbindGroup(LogGroup& group)
{
   Debug::ftnt("LogGroupRegistry.UnbindGroup");

   groups_.Erase(group);
}
}
