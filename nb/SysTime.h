//==============================================================================
//
//  SysTime.h
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
#ifndef SYSTIME_H_INCLUDED
#define SYSTIME_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include "Clock.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: calendar time.
//
class SysTime : public Object
{
public:
   //  Ranges of fields in the time_ structure.
   //
   static const int16_t MinYear    = 1900;
   static const int16_t MaxYear    = 2100;
   static const int16_t MaxMonth   = 11;
   static const int16_t MinDay     = 1;
   static const int16_t MaxDay     = 31;
   static const int16_t MaxHour    = 23;
   static const int16_t MaxMin     = 59;
   static const int16_t MaxSec     = 59;
   static const int16_t MaxMsec    = 999;
   static const int16_t MaxWeekDay = 6;

   //  Formats for to_str.
   //
   enum Format
   {
      Alpha,        // DD-MMM-YYYY HH:MM:SS.mmm
      HighAlpha,    // DD-MMM-YYYY
      LowAlpha,     // HH:MM:SS.mmm
      Numeric,      // YYMMDD-HHMMSS.mmm
      HighNumeric,  // YYMMDD
      LowNumeric    // HHMMSS.mmm
   };

   //  Sets the time to the current time.
   //
   SysTime();

   //  Sets the time to the indicated values.
   //
   SysTime(int16_t year, int16_t month, int16_t day,
      int16_t hour, int16_t min, int16_t sec, int16_t msec);

   //  Returns the value of the specified field.
   //
   int16_t Get(TimeField field) const { return time_[field]; }

   //  Returns the day of the week (0 to 6, Sunday = 0).
   //
   int16_t DayOfWeek() const;

   //  Returns the day of the year (0 to 365, January 1st = 0).
   //
   int16_t DayOfYear() const;

   //  Returns true if YEAR is a leap year.
   //
   static bool IsLeapYear(int16_t year);

   //  Truncates least significant fields, starting at FIELD.
   //
   void Truncate(TimeField field);

   //  Rounds off the time starting at FIELD, which is rounded to the
   //  nearest whole multiple of INTERVAL.  Not supported if FIELD is
   //  Year, Month, or Day.  For example:
   //  o Round(Sec, 1) rounds the seconds up or down and zeroes the msecs.
   //  o Round(Min, 15) rounds the minutes to the nearest multiple of 15
   //    (hh:15, hh:30, hh:45, or hh:00) and zeroes the seconds and msecs.
   //
   void Round(TimeField field, int16_t interval);

   //  Adds MSECS to the time.
   //
   void AddMsecs(msecs_t msecs);

   //  Subtracts MSECS from the time.
   //
   void SubMsecs(msecs_t msecs);

   //  Returns the number of msecs from now to this time.  If
   //  this time is earlier than now, the result is negative.
   //
   int32_t MsecsFromNow() const;

   //  Returns the number of msecs between this time and TIME.
   //  If TIME is earlier than this time, the result is negative.
   //
   int32_t MsecsUntil(const SysTime& time) const;

   //  Adds DAYS to the time.
   //
   void AddDays(size_t days);

   //  Subtracts DAYS from the time.
   //
   void SubDays(size_t days);

   //  Returns a three-character string for the Month.
   //
   c_string strMonth() const;

   //  Returns a three-character string for the DayOfWeek.
   //
   c_string strWeekDay() const;

   //  Returns a string for displaying the time in the indicated format.
   //
   std::string to_str(Format format) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Returns a pointer to a 12-element array that contains the number
   //  of days in each month, based on whether YEAR is a leap year.
   //
   static const int16_t* DaysPerMonth(int16_t year);

   //  The year of T0 (1900).
   //
   static const int16_t YearOfT0;

   //  The weekday of T0 (January 1, 1900 was a Monday).
   //
   static const int16_t WeekDayOfT0;

   //  The number of seconds in a leap year.
   //
   static const int64_t SecsInLeapYear;

   //  The number of seconds in a non leap year.
   //
   static const int64_t SecsInNonLeapYear;

   //  Returns the number of days since T0 (Jan 1, 1900).
   //
   size_t DaysSinceT0() const;

   //  Returns the number of msecs since T0 (Jan 1, 1900).
   //
   int64_t MsecsSinceT0() const;

   //  Verifies that the time is valid.
   //
   bool Verify();

   //  Generates a software log containing the contents of FIELD, zeroes
   //  all fields, and returns false.
   //
   bool OutOfRange(TimeField field);

   //  The time.
   //
   int16_t time_[TimeField_N];
};
}
#endif
