//==============================================================================
//
//  Alarm.cpp
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
#include "Alarm.h"
#include "Permanent.h"
#include <chrono>
#include <ratio>
#include <sstream>
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "Debug.h"
#include "Log.h"
#include "Singleton.h"
#include "SteadyTime.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Data that changes too frequently to unprotect and reprotect memory
//  when it needs to be modified.
//
struct AlarmDynamic : public Permanent
{
   //  Initializes members.
   //
   AlarmDynamic() : status_(NoAlarm),
      nextStatus_(NoAlarm), currStatusTime_(SteadyTime::GetInvalid()) { }

   //  The alarm's current status.
   //
   AlarmStatus status_;

   //  The level to which the alarm will be downgraded once DELAY has
   //  passed.  It is the highest value set for the alarm during the
   //  delay.
   //
   AlarmStatus nextStatus_;

   //  The most recent time at which the alarm was at its current level.
   //
   SteadyTime::Point currStatusTime_;
};

//==============================================================================
//
//> The maximum length of an alarm's name.
//
constexpr size_t MaxNameSize = 12;

//> The maximum length of the string that explains an alarm.
//
constexpr size_t MaxExplSize = 48;

//------------------------------------------------------------------------------

fn_name Alarm_ctor = "Alarm.ctor";

Alarm::Alarm(c_string name, c_string expl, uint32_t delay) :
   name_(name),
   expl_(expl),
   delay_(secs_t(delay))
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

   dyn_.reset(new AlarmDynamic);
   Singleton< AlarmRegistry >::Instance()->BindAlarm(*this);
}

//------------------------------------------------------------------------------

fn_name Alarm_dtor = "Alarm.dtor";

Alarm::~Alarm()
{
   Debug::ftnt(Alarm_dtor);

   Debug::SwLog(Alarm_dtor, UnexpectedInvocation, 0);
   Singleton< AlarmRegistry >::Extant()->UnbindAlarm(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Alarm::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Alarm* >(&local);
   return ptrdiff(&fake->aid_, fake);
}

//------------------------------------------------------------------------------

ostringstreamPtr Alarm::Create(c_string groupName, LogId id, AlarmStatus status)
{
   Debug::ft("Alarm.Create");

   auto now = SteadyTime::Now();
   ostringstreamPtr log(nullptr);

   if(status > dyn_->status_)
   {
      //  Increase the alarm's level immediately.
      //
      SetStatus(status);
      log = Log::Create(groupName, id, name_.c_str(), dyn_->status_);
   }
   else if(status == dyn_->status_)
   {
      //  The alarm is still at its current level.
      //
      dyn_->nextStatus_ = NoAlarm;
      dyn_->currStatusTime_ = now;
   }
   else
   {
      //  After the delay has passed, decrease the alarm to the
      //  highest level recorded during the delay period.
      //
      if(status > dyn_->nextStatus_) dyn_->nextStatus_ = status;

      if(now - dyn_->currStatusTime_ >= delay_)
      {
         SetStatus(dyn_->nextStatus_);
         log = Log::Create(groupName, id, name_.c_str(), dyn_->status_);
      }
   }

   return log;
}

//------------------------------------------------------------------------------

void Alarm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << AlarmStatusSymbol(dyn_->status_);
   stream << name_ << SPACE << '(' << expl_ << ')' << CRLF;
}

//------------------------------------------------------------------------------

void Alarm::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Alarm::SetStatus(AlarmStatus status)
{
   Debug::ft("Alarm.SetStatus");

   dyn_->status_ = status;
   dyn_->nextStatus_ = NoAlarm;
   dyn_->currStatusTime_ = SteadyTime::Now();
}

//------------------------------------------------------------------------------

void Alarm::Shutdown(RestartLevel level)
{
   Debug::ft("Alarm.Shutdown");

   //  Clear an alarm during all restarts by using placement new to
   //  reset dyn_.
   //
   new (dyn_.get()) AlarmDynamic();
}

//------------------------------------------------------------------------------

AlarmStatus Alarm::Status() const
{
   return dyn_->status_;
}
}
