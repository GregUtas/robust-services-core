//==============================================================================
//
//  LogGroup.cpp
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
#include "LogGroup.h"
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "LogGroupRegistry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t LogGroup::MaxNameSize = 5;
const size_t LogGroup::MaxExplSize = 48;
const id_t LogGroup::MaxLogs = 250;

//------------------------------------------------------------------------------

fn_name LogGroup_ctor = "LogGroup.ctor";

LogGroup::LogGroup(fixed_string name, fixed_string expl) :
   name_(strUpper(name).c_str()),
   expl_(expl),
   suppressed_(false)
{
   Debug::ft(LogGroup_ctor);

   if(name_.size() > MaxNameSize)
   {
      Debug::SwLog(LogGroup_ctor, "name length", name_.size());
   }

   if(expl_.size() > MaxExplSize)
   {
      Debug::SwLog(LogGroup_ctor, "expl length", expl_.size());
   }

   logs_.Init(MaxLogs + 1, Log::CellDiff(), MemDyn);

   if(!Singleton< LogGroupRegistry >::Instance()->BindGroup(*this))
   {
      Debug::SwLog(LogGroup_ctor, name_.c_str(), 0);
   }
}

//------------------------------------------------------------------------------

fn_name LogGroup_dtor = "LogGroup.dtor";

LogGroup::~LogGroup()
{
   Debug::ft(LogGroup_dtor);

   Singleton< LogGroupRegistry >::Instance()->UnbindGroup(*this);
}

//------------------------------------------------------------------------------

fn_name LogGroup_BindLog = "LogGroup.BindLog";

bool LogGroup::BindLog(Log& log)
{
   Debug::ft(LogGroup_BindLog);

   //  Check that LOG's identifier isn't already in use and that logs
   //  are registered in ascending order within their group.
   //
   auto id = log.Id();

   if(FindLog(id) != nullptr)
   {
      Debug::SwLog(LogGroup_BindLog, "LogId in use", id);
   }

   auto last = logs_.Last();

   if((last != nullptr) && (last->Id() > id))
   {
      Debug::SwLog(LogGroup_BindLog, "LogId not sorted", id);
   }

   return logs_.Insert(log);
}

//------------------------------------------------------------------------------

ptrdiff_t LogGroup::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const LogGroup* >(&local);
   return ptrdiff(&fake->gid_, fake);
}

//------------------------------------------------------------------------------

void LogGroup::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << strIndex(gid_.GetId());
   stream << name_ << " group (" << expl_ << ')';
   if(suppressed_) stream << " [SUPPRESSED]";
   stream << CRLF;

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      l->Display(stream, prefix + spaces(2), NoFlags);
   }
}

//------------------------------------------------------------------------------

fn_name LogGroup_DisplayStats = "LogGroup.DisplayStats";

void LogGroup::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft(LogGroup_DisplayStats);

   stream << spaces(2) << name_.c_str() << " group ";
   stream << strIndex(Gid(), 0, false) << CRLF;

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      l->DisplayStats(stream, options);
   }
}

//------------------------------------------------------------------------------

fn_name LogGroup_FindLog = "LogGroup.FindLog";

Log* LogGroup::FindLog(LogId id) const
{
   Debug::ft(LogGroup_FindLog);

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      if(l->Id() == id) return l;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void LogGroup::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name LogGroup_Shutdown = "LogGroup.Shutdown";

void LogGroup::Shutdown(RestartLevel level)
{
   Debug::ft(LogGroup_Shutdown);

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      l->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

fn_name LogGroup_UnbindLog = "LogGroup.UnbindLog";

void LogGroup::UnbindLog(Log& log)
{
   Debug::ft(LogGroup_UnbindLog);

   logs_.Erase(log);
}
}
