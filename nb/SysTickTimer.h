//==============================================================================
//
//  SysTickTimer.h
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
#ifndef SYSTICKTIMER_H_INCLUDED
#define SYSTICKTIMER_H_INCLUDED

#include "Immutable.h"
#include <string>
#include "Clock.h"
#include "SysTime.h"

namespace NodeBase
{
   template< typename T > class Singleton;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: raw tick timer.
//
class SysTickTimer : public Immutable
{
   friend class Singleton< SysTickTimer >;
public:
   //  Returns the number of ticks in one second.
   //
   const ticks_t& TicksPerSec() const { return ticks_per_sec_; }

   //  Returns the current time as a raw tick count.  It is assumed
   //  that the value returned by this function increases over time
   //  and is only reset if the system is rebooted.
   //
   ticks_t TicksNow() const;

   //  Returns the time (in ticks) when the system was booted.
   //
   const ticks_t& StartTick() const { return startTick_; }

   //  Returns the time (in full) when the system was booted.
   //
   const SysTime& StartTime() const { return startTime_; }

   //  Returns the time (yymmdd-hhmmss) when the system was booted.
   //
   const std::string& StartTimeStr() const { return startTimeStr_; }

   //  Returns true if this platform supports fine-grained timing.  If
   //  it returns false, timing is only accurate to 1 millisecond, so
   //  it's time to look for a proper platform.
   //
   bool TickTimingAvailable() const { return available_; }
private:
   //  Private because this singleton is not subclassed.
   //
   SysTickTimer();

   //  Private because this singleton is not subclassed.
   //
   ~SysTickTimer();

   //  The number of ticks in one second.
   //
   ticks_t ticks_per_sec_;

   //  The tick count when the system was initialized.
   //
   ticks_t startTick_;

   //  The full clock time when the system was initialized.
   //
   SysTime startTime_;

   //  startTime_ as a string (yymmdd-hhmmss).
   //
   std::string startTimeStr_;

   //  Set if this platform supports fine-grained tick timing.
   //
   bool available_;
};
}
#endif
