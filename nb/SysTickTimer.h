//==============================================================================
//
//  SysTickTimer.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef SYSTICKTIMER_H_INCLUDED
#define SYSTICKTIMER_H_INCLUDED

#include "Immutable.h"
#include <cstdint>
#include "NbTypes.h"
#include "SysTime.h"
#include "SysTypes.h"
#include "TimePoint.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: raw tick timer.
//
class SysTickTimer : public Immutable
{
public:
   //  Deleted to prohibit copying.
   //
   SysTickTimer(const SysTickTimer& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   SysTickTimer& operator=(const SysTickTimer& that) = delete;

   //  Returns the timer after creating it if it doesn't yet exist.
   //
   static SysTickTimer* Instance();

   //  Returns the timer.
   //
   static SysTickTimer* Extant() { return Instance_; }

   //  Returns the number of ticks in one second.
   //
   int64_t TicksPerSec() const { return ticks_per_sec_; }

   //  Returns the current time as a raw tick count.  It is assumed
   //  that the value returned by this function increases over time
   //  and is only reset if the system is rebooted.
   //
   TimePoint Now() const;

   //  Returns the time (in ticks) when the system was booted.
   //
   TimePoint StartPoint() const { return startPoint_; }

   //  Returns the time (in full) when the system was booted.
   //
   const SysTime& StartTime() const { return startTime_; }

   //  Returns the time (yymmdd-hhmmss) when the system was booted.
   //
   c_string StartTimeStr() const { return startTimeStr_.c_str(); }

   //  Returns true if this platform supports fine-grained timing.  If
   //  it returns false, timing is only accurate to 1 millisecond, so
   //  it's time to look for a proper platform.
   //
   bool TickTimingAvailable() const { return available_; }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   SysTickTimer();

   //  Private because this is a singleton.
   //
   ~SysTickTimer();

   //  The number of ticks in one second.
   //
   int64_t ticks_per_sec_;

   //  The time when the system was initialized.
   //
   TimePoint startPoint_;

   //  The full clock time when the system was initialized.
   //
   SysTime startTime_;

   //  startTime_ as a string (yymmdd-hhmmss).
   //
   ImmutableStr startTimeStr_;

   //  The timer instance.
   //
   static SysTickTimer* Instance_;

   //  Set if this platform supports fine-grained tick timing.
   //
   bool available_;
};
}
#endif
