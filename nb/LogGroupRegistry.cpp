//==============================================================================
//
//  LogGroupRegistry.cpp
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
#include "LogGroupRegistry.h"
#include "StatisticsGroup.h"
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

fn_name LogStatsGroup_ctor = "LogStatsGroup.ctor";

LogStatsGroup::LogStatsGroup() : StatisticsGroup("Logs [group id_t]")
{
   Debug::ft(LogStatsGroup_ctor);
}

//------------------------------------------------------------------------------

fn_name LogStatsGroup_dtor = "LogStatsGroup.dtor";

LogStatsGroup::~LogStatsGroup()
{
   Debug::ft(LogStatsGroup_dtor);
}

//------------------------------------------------------------------------------

fn_name LogStatsGroup_DisplayStats = "LogStatsGroup.DisplayStats";

void LogStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft(LogStatsGroup_DisplayStats);

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton< LogGroupRegistry >::Instance();

   if(id == 0)
   {
      auto& groups = reg->Groups();

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

const id_t LogGroupRegistry::MaxGroups = 250;

//------------------------------------------------------------------------------

fn_name LogGroupRegistry_ctor = "LogGroupRegistry.ctor";

LogGroupRegistry::LogGroupRegistry()
{
   Debug::ft(LogGroupRegistry_ctor);

   groups_.Init(MaxGroups, LogGroup::CellDiff(), MemImmutable);
   statsGroup_.reset(new LogStatsGroup);
}

//------------------------------------------------------------------------------

fn_name LogGroupRegistry_dtor = "LogGroupRegistry.dtor";

LogGroupRegistry::~LogGroupRegistry()
{
   Debug::ft(LogGroupRegistry_dtor);
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

   stream << prefix << "Groups [id_t] with logs:" << CRLF;

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      g->Display(stream, lead, NoFlags);
   }
}

//------------------------------------------------------------------------------

fn_name LogGroupRegistry_FindGroup = "LogGroupRegistry.FindGroup";

LogGroup* LogGroupRegistry::FindGroup(const std::string& name) const
{
   Debug::ft(LogGroupRegistry_FindGroup);

   auto key = strUpper(name);

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      if(g->Name() == key) return g;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name LogGroupRegistry_FindLog = "LogGroupRegistry.FindLog";

Log* LogGroupRegistry::FindLog(const std::string& name, LogId id) const
{
   Debug::ft(LogGroupRegistry_FindLog);

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

fn_name LogGroupRegistry_Shutdown = "LogGroupRegistry.Shutdown";

void LogGroupRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(LogGroupRegistry_Shutdown);

   for(auto g = groups_.First(); g != nullptr; groups_.Next(g))
   {
      g->Shutdown(level);
   }

   FunctionGuard guard(Guard_ImmUnprotect);
   Restart::Release(statsGroup_);
}

//------------------------------------------------------------------------------

fn_name LogGroupRegistry_Startup = "LogGroupRegistry.Startup";

void LogGroupRegistry::Startup(RestartLevel level)
{
   Debug::ft(LogGroupRegistry_Startup);

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

fn_name LogGroupRegistry_UnbindGroup = "LogGroupRegistry.UnbindGroup";

void LogGroupRegistry::UnbindGroup(LogGroup& group)
{
   Debug::ft(LogGroupRegistry_UnbindGroup);

   groups_.Erase(group);
}
}
