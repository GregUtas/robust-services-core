//==============================================================================
//
//  Alarm.h
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
#ifndef ALARM_H_INCLUDED
#define ALARM_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include "Duration.h"
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

namespace NodeBase
{
   struct AlarmDynamic;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Interface for defining alarms.  Alarms are closely coupled to logs
//  and therefore survive all restarts.  An alarm is set or cleared by
//  Log::Create (see Logs.h).
//
class Alarm : public Immutable
{
   friend class AlarmsCommand;
public:
   //  Deleted to prohibit copying.
   //
   Alarm(const Alarm& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Alarm& operator=(const Alarm& that) = delete;

   //  Creates an alarm identified by NAME and explained by EXPL.  DELAY is
   //  for hysteresis control: the alarm's level cannot be decreased until
   //  DELAY has passed.  A value of TIMEOUT_IMMED avoids hysteresis control.
   //  Instead of Log::Create, the application must invoke Alarm::Create,
   //  which returns nullptr unless a log should be generated.
   //
   Alarm(c_string name, c_string expl, secs_t delay);

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
   ostringstreamPtr Create(c_string groupName, LogId id, AlarmStatus status);

   //  Returns the alarm's name.
   //
   c_string Name() const { return name_.c_str(); }

   //  Returns the explanation for the alarm.
   //
   c_string Expl() const { return expl_.c_str(); }

   //  Returns the alarm's status.
   //
   AlarmStatus Status() const;

   //  Returns the offset to aid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;
private:
   //  Sets the alarm's status.
   //
   void SetStatus(AlarmStatus status);

   //  The alarm's name.
   //
   const ImmutableStr name_;

   //  The string that explains the alarm.
   //
   const ImmutableStr expl_;

   //  The delay when downgrading the alarm.
   //
   const Duration delay_;

   //  The alarm's index in AlarmRegistry.
   //
   RegCell aid_;

   //  Data that changes too frequently to unprotect and reprotect memory
   //  when it needs to be modified.
   //
   std::unique_ptr< AlarmDynamic > dyn_;
};
}
#endif
