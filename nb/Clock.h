//==============================================================================
//
//  Clock.h
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
#ifndef CLOCK_H_INCLUDED
#define CLOCK_H_INCLUDED

#include <cstdint>
#include <string>

namespace NodeBase
{
   class SysTime;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Time intervals.
//
typedef uint32_t secs_t;   // seconds
typedef uint32_t msecs_t;  // milliseconds
typedef uint32_t usecs_t;  // microseconds (limited to ~1:11:35) on 32-bit CPU
typedef uint64_t ticks_t;  // ticks (platform dependent)

//  Timeout intervals.
//
constexpr msecs_t TIMEOUT_IMMED = 0;           // expires immediately
constexpr msecs_t TIMEOUT_1_SEC = 1000;        // expires in one second
constexpr msecs_t TIMEOUT_NEVER = UINT32_MAX;  // never expires

//  A field in a full time representation.
//
enum TimeField
{
   YearsField,
   MonthsField,
   DaysField,
   HoursField,
   MinsField,
   SecsField,
   MsecsField,
   TimeField_N
};

//------------------------------------------------------------------------------
//
//  Basic timing functions.
//
namespace Clock
{
   //  Returns the number of ticks in one second.
   //
   ticks_t TicksPerSec();

   //  Returns the current time in ticks.  Must not return 0, so that
   //  this can be used as a nil value.  Return 1 instead.
   //
   ticks_t TicksNow();

   //  Returns the time between now and PAST.  Returns 0 if PAST is
   //  in the future.
   //
   ticks_t TicksSince(const ticks_t& past);

   //  Returns the time between now and PAST, a previous timestamp.
   //  Updates NOW to TicksNow.
   //
   ticks_t TicksSince(const ticks_t& past, ticks_t& now);

   //  Returns the time between now and FUTURE.  Returns 0 if FUTURE
   //  is in the past.
   //
   ticks_t TicksUntil(const ticks_t& future);

   //  Converts a tick time (TICKS) to a time-of-day string (hh:mm:ss.mmm).
   //  Truncates the string if FIELD is MinsField, SecsField, or MsecsField.
   //
   std::string TicksToTime(const ticks_t& ticks, TimeField field = HoursField);

   //  Returns the number of seconds in TICKS.
   //
   secs_t TicksToSecs(const ticks_t& ticks);

   //  Returns the number of milliseconds in TICKS.
   //
   msecs_t TicksToMsecs(const ticks_t& ticks);

   //  Returns the number of microseconds in TICKS.
   //
   usecs_t TicksToUsecs(const ticks_t& ticks);

   //  Returns the number of ticks in SECS.
   //
   ticks_t SecsToTicks(secs_t secs);

   //  Returns the number of ticks in MSECS.
   //
   ticks_t MsecsToTicks(msecs_t msecs);

   //  Returns the number of ticks in USECS.
   //
   ticks_t UsecsToTicks(usecs_t usecs);

   //  Returns the time (full) when the clock was initialized.
   //
   const SysTime& TimeZero();

   //  Returns the time (string) when the clock was initialized.
   //
   std::string TimeZeroStr();

   //  Returns the time (ticks) when the clock was initialized.
   //
   ticks_t TicksZero();
}
}
#endif
