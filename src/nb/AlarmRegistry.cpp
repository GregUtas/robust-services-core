//==============================================================================
//
//  AlarmRegistry.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "AlarmRegistry.h"
#include <cstddef>
#include <iomanip>
#include <ios>
#include <ostream>
#include "Alarm.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysTypes.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum number of alarms.
//
constexpr id_t MaxAlarms = 250;

//------------------------------------------------------------------------------

AlarmRegistry::AlarmRegistry()
{
   Debug::ft("AlarmRegistry.ctor");

   alarms_.Init(MaxAlarms, Alarm::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name AlarmRegistry_dtor = "AlarmRegistry.dtor";

AlarmRegistry::~AlarmRegistry()
{
   Debug::ftnt(AlarmRegistry_dtor);

   Debug::SwLog(AlarmRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name AlarmRegistry_BindAlarm = "AlarmRegistry.BindAlarm";

bool AlarmRegistry::BindAlarm(Alarm& alarm)
{
   Debug::ft(AlarmRegistry_BindAlarm);

   if(Find(alarm.Name()) != nullptr)
   {
      Debug::SwLog(AlarmRegistry_BindAlarm, alarm.Name(), 0);
      return false;
   }

   return alarms_.Insert(alarm);
}

//------------------------------------------------------------------------------

void AlarmRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead = prefix + spaces(2);
   size_t active = 0;

   stream << prefix << "Alarms:" << CRLF;

   for(auto a = alarms_.First(); a != nullptr; alarms_.Next(a))
   {
      a->Display(stream, lead, NoFlags);
      if(a->Status() != NoAlarm) ++active;
   }

   stream << prefix;
   if(active == 0)
      stream << "No";
   else
      stream << active;
   stream << " alarm(s) active." << CRLF;
}

//------------------------------------------------------------------------------

Alarm* AlarmRegistry::Find(const std::string& name) const
{
   Debug::ftnt("AlarmRegistry.Find");

   auto key = strUpper(name);

   for(auto a = alarms_.First(); a != nullptr; alarms_.Next(a))
   {
      if(strUpper(a->Name()) == key) return a;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void AlarmRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void AlarmRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("AlarmRegistry.Shutdown");

   for(auto a = alarms_.First(); a != nullptr; alarms_.Next(a))
   {
      a->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

void AlarmRegistry::Startup(RestartLevel level)
{
   Debug::ft("AlarmRegistry.Startup");

   for(auto a = alarms_.First(); a != nullptr; alarms_.Next(a))
   {
      a->Startup(level);
   }
}

//------------------------------------------------------------------------------

fixed_string AlarmHeader = " Id  Level  Name       Explanation";
//                         |  3.     6..10        .<expl>

size_t AlarmRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << AlarmHeader << CRLF;

   for(auto a = alarms_.First(); a != nullptr; alarms_.Next(a))
   {
      stream << setw(3) << a->Aid();
      stream << SPACE << setw(6) << AlarmStatusSymbol(a->Status());
      stream << spaces(2) << std::left << setw(10) << a->Name();
      stream << SPACE << std::right << a->Expl() << CRLF;
   }

   return alarms_.Size();
}

//------------------------------------------------------------------------------

void AlarmRegistry::UnbindAlarm(Alarm& alarm)
{
   Debug::ftnt("AlarmRegistry.UnbindAlarm");

   alarms_.Erase(alarm);
}
}
