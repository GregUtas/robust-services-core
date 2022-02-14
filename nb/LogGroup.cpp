//==============================================================================
//
//  LogGroup.cpp
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
#include "LogGroup.h"
#include <bitset>
#include <cstdint>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Log.h"
#include "LogGroupRegistry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum length of the string that explains a log group.
//
constexpr size_t MaxExplSize = 48;

//> The maximum number of logs in a group.
//
constexpr id_t MaxLogs = 250;

const size_t LogGroup::MaxNameSize = 5;

//------------------------------------------------------------------------------

fn_name LogGroup_ctor = "LogGroup.ctor";

LogGroup::LogGroup(c_string name, c_string expl) :
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

   logs_.Init(MaxLogs, Log::CellDiff(), MemImmutable);

   Singleton< LogGroupRegistry >::Instance()->BindGroup(*this);
}

//------------------------------------------------------------------------------

fn_name LogGroup_dtor = "LogGroup.dtor";

LogGroup::~LogGroup()
{
   Debug::ftnt(LogGroup_dtor);

   Debug::SwLog(LogGroup_dtor, UnexpectedInvocation, 0);
   Singleton< LogGroupRegistry >::Extant()->UnbindGroup(*this);
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
      return false;
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
   uintptr_t local;
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
   if(!options.test(DispVerbose)) return;

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      l->Display(stream, prefix + spaces(2), NoFlags);
   }
}

//------------------------------------------------------------------------------

void LogGroup::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft("LogGroup.DisplayStats");

   stream << spaces(2) << name_.c_str() << " group ";
   stream << strIndex(Gid(), 0, false) << CRLF;

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      l->DisplayStats(stream, options);
   }
}

//------------------------------------------------------------------------------

Log* LogGroup::FindLog(LogId id) const
{
   Debug::ftnt("LogGroup.FindLog");

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      if(l->Id() == id) return l;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void LogGroup::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void LogGroup::SetSuppressed(bool suppressed)
{
   FunctionGuard guard(Guard_ImmUnprotect);
   suppressed_ = suppressed;
}

//------------------------------------------------------------------------------

void LogGroup::Shutdown(RestartLevel level)
{
   Debug::ft("LogGroup.Shutdown");

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      l->Shutdown(level);
   }

   SetSuppressed(false);
}

//------------------------------------------------------------------------------

void LogGroup::Startup(RestartLevel level)
{
   Debug::ft("LogGroup.Startup");

   for(auto l = logs_.First(); l != nullptr; logs_.Next(l))
   {
      l->Startup(level);
   }
}

//------------------------------------------------------------------------------

void LogGroup::UnbindLog(Log& log)
{
   Debug::ftnt("LogGroup.UnbindLog");

   logs_.Erase(log);
}
}
