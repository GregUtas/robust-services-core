//==============================================================================
//
//  Alarm.cpp
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
#include "Alarm.h"
#include <sstream>
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "Debug.h"
#include "Log.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t Alarm::MaxNameSize = 12;
const size_t Alarm::MaxExplSize = 48;

//------------------------------------------------------------------------------

fn_name Alarm_ctor = "Alarm.ctor";

Alarm::Alarm(const string& name, const string& expl, secs_t delay) :
   name_(name.c_str()),
   expl_(expl.c_str()),
   delay_(delay * Clock::TicksPerSec()),
   status_(NoAlarm),
   nextStatus_(NoAlarm),
   currStatusTime_(0)
{
   Debug::ft(Alarm_ctor);

   if(name_.size() > MaxNameSize)
   {
      Debug::SwLog(Alarm_ctor, "name length", name_.size());
   }

   if(expl_.size() > MaxExplSize)
   {
      Debug::SwLog(Alarm_ctor, "expl length", expl_.size());
   }

   if(!Singleton< AlarmRegistry >::Instance()->BindAlarm(*this))
   {
      Debug::SwLog(Alarm_ctor, expl_.c_str(), 0);
   }
}

//------------------------------------------------------------------------------

fn_name Alarm_dtor = "Alarm.dtor";

Alarm::~Alarm()
{
   Debug::ft(Alarm_dtor);

   Singleton< AlarmRegistry >::Instance()->UnbindAlarm(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Alarm::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Alarm* >(&local);
   return ptrdiff(&fake->aid_, fake);
}

//------------------------------------------------------------------------------

fn_name Alarm_Create = "Alarm.Create";

ostringstreamPtr Alarm::Create
   (fixed_string groupName, LogId id, AlarmStatus status)
{
   Debug::ft(Alarm_Create);

   auto now = Clock::TicksNow();
   ostringstreamPtr log(nullptr);

   if(status > status_)
   {
      //  Increase the alarm's level immediately.
      //
      SetStatus(status);
      log = Log::Create(groupName, id, name_.c_str(), status_);
   }
   else if(status == status_)
   {
      //  The alarm is still at its current level.
      //
      nextStatus_ = NoAlarm;
      currStatusTime_ = now;
   }
   else
   {
      //  After the delay has passed, decrease the alarm to the
      //  highest level recorded during the delay period.
      //
      if(status > nextStatus_) nextStatus_ = status;

      if(now - currStatusTime_ >= delay_)
      {
         SetStatus(nextStatus_);
         log = Log::Create(groupName, id, name_.c_str(), status_);
      }
   }

   return log;
}

//------------------------------------------------------------------------------

void Alarm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << AlarmStatusSymbol(status_);
   stream << name_ << SPACE << '(' << expl_ << ')' << CRLF;
}

//------------------------------------------------------------------------------

void Alarm::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Alarm_SetStatus = "Alarm.SetStatus";

void Alarm::SetStatus(AlarmStatus status)
{
   Debug::ft(Alarm_SetStatus);

   status_ = status;
   nextStatus_ = NoAlarm;
   currStatusTime_ = Clock::TicksNow();
}

//------------------------------------------------------------------------------

fn_name Alarm_Shutdown = "Alarm.Shutdown";

void Alarm::Shutdown(RestartLevel level)
{
   Debug::ft(Alarm_Shutdown);

   //  An alarm survives a warm restart but should be cleared.
   //
   status_ = NoAlarm;
}
}
