//==============================================================================
//
//  Alarm.h
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
#ifndef ALARM_H_INCLUDED
#define ALARM_H_INCLUDED

#include "Dynamic.h"
#include <cstddef>
#include <string>
#include "Clock.h"
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Interface for defining alarms.  Alarm definitions survive warm restarts
//  but must be recreated during all others.  An alarm is set or cleared by
//  Log::Create (see Logs.h).
//
class Alarm : public Dynamic
{
   friend class AlarmsCommand;
public:
   //> The maximum length of an alarm's name.
   //
   static const size_t MaxNameSize;

   //> The maximum length of the string that explains an alarm.
   //
   static const size_t MaxExplSize;

   //  Creates an alarm identified by NAME and explained by EXPL.  DELAY is
   //  for hysteresis control: the alarm's level cannot be decreased until
   //  DELAY seconds have passed.  A value of 0 avoids hysteresis control.
   //  Instead of Log::Create, the application must invoke Alarm::Create,
   //  which returns nullptr unless a log should be generated.
   //
   Alarm(const std::string& name, const std::string& expl, secs_t delay);

   //  Not subclassed.
   //
   ~Alarm();

   //  Creates an instance of the log identified by groupName and ID, with
   //  STATUS used to set or clear this alarm.  Only trouble and threshold
   //  logs should set an alarm, but any type of log can clear one.  Returns
   //  the stream allocated for the log after formatting its header, which
   //  ends in a CRLF.  When adding more info to the log, start each line
   //  with Log::Tab so that the asterisks used to highlight an active alarm
   //  remain prominent.
   //
   //  NOTE: Unless nullptr is returned, Log::Submit must still be invoked.
   //
   ostringstreamPtr Create
      (fixed_string groupName, LogId id, AlarmStatus status);

   //  Returns the alarm's name.
   //
   c_string Name() const { return name_.c_str(); }

   //  Returns the explanation for the alarm.
   //
   c_string Expl() const { return expl_.c_str(); }

   //  Returns the alarm's status.
   //
   AlarmStatus Status() const { return status_; }

   //  Returns the offset to aid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Sets the alarm's status.
   //
   void SetStatus(AlarmStatus status);

   //  The alarm's name.
   //
   const DynString name_;

   //  The string that explains the alarm.
   //
   const DynString expl_;

   //  The delay when downgrading the alarm.
   //
   const ticks_t delay_;

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
   ticks_t currStatusTime_;

   //  The alarm's index in AlarmRegistry.
   //
   RegCell aid_;
};
}
#endif
