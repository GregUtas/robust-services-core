//==============================================================================
//
//  Duration.h
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
#ifndef DURATION_H_INCLUDED
#define DURATION_H_INCLUDED

#include <cstdint>
#include <string>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A field in a full time representation.  This is used mostly in SysTime
//  but is also required here.
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
//  Time intervals.  These are used for values that will be converted to, or
//  have been converted from, a Duration (below).
//
typedef uint32_t secs_t;   // seconds
typedef uint32_t msecs_t;  // milliseconds
typedef uint32_t usecs_t;  // microseconds

//------------------------------------------------------------------------------
//
//  Units for a time interval.
//
enum TimeUnits
{
   TICKS,    // system-specific
   uSECS,    // microseconds
   mSECS,    // milliseconds
   SECS,     // seconds
   MINUTES,
   HOURS,
   DAYS
};

//------------------------------------------------------------------------------
//
//  A time interval.
//
class Duration
{
public:
   //  Constructor for a duration of VALUE, in UNITS.
   //
   Duration(int64_t value, TimeUnits units);

   //  Constructs a duration of 0.
   //
   Duration();

   //  Not subclassed.
   //
   ~Duration() = default;

   //  Copy constructor.
   //
   Duration(const Duration& that) = default;

   //  Copy operator.
   //
   Duration& operator=(const Duration& that) = default;

   //  Returns a string that represents the duration in UNITS.
   //
   std::string to_str(TimeUnits units) const;

   //  Returns the interval until now (zero).
   //
   static Duration Immed();

   //  Returns the interval that will never expire (Infinity).
   //
   static Duration Never();

   //  Returns the interval in ticks.
   //
   int64_t Ticks() const { return ticks_; }

   //  Returns the interval in UNITS.
   //
   int64_t To(TimeUnits units) const;

   //  Returns the interval in milliseconds, mapping Infinity to
   //  UINT32_MAX and negative times to 0.
   //
   uint32_t ToMsecs() const;

   //  Returns the interval, unchanged.
   //
   Duration operator+();

   //  Returns the interval, negated.
   //
   Duration operator-();

   //  Adds a tick to the interval.
   //
   Duration& operator++();
   Duration operator++(int);

   //  Subtracts a tick from the interval.
   //
   Duration& operator--();
   Duration operator--(int);

   //  Adds RHS to the interval.
   //
   Duration& operator+=(const Duration& rhs);

   //  Subtracts RHS from the interval.
   //
   Duration& operator-=(const Duration& rhs);

   //  Multiplies the interval by RHS.
   //
   Duration& operator*=(int64_t rhs);

   //  Divides the interval by RHS.
   //
   Duration& operator/=(int64_t rhs);

   //  Finds the reminder of the interval when divided by RHS.
   //
   Duration& operator%=(const Duration& rhs);

   //  Changes the interval by a power of 2.
   //
   Duration& operator<<=(int8_t shift);
   Duration& operator>>=(int8_t shift);

   //  The value that represents infinity.
   //
   static const int64_t Infinity = INT64_MAX;
private:
   //  The duration, which is always in ticks.
   //
   int64_t ticks_;
};

//  Duration comparison operators.
//
bool operator==(const Duration& lhs, const Duration& rhs);
bool operator!=(const Duration& lhs, const Duration& rhs);
bool operator<(const Duration& lhs, const Duration& rhs);
bool operator<=(const Duration& lhs, const Duration& rhs);
bool operator>(const Duration& lhs, const Duration& rhs);
bool operator>=(const Duration& lhs, const Duration& rhs);

//  Duration arithmetic operators.
//
Duration operator+(const Duration& lhs, const Duration& rhs);
Duration operator-(const Duration& lhs, const Duration& rhs);
Duration operator*(const Duration& lhs, int64_t rhs);
Duration operator*(int64_t lhs, const Duration& rhs);
Duration operator/(const Duration& lhs, int64_t rhs);
int64_t  operator/(const Duration& lhs, const Duration& rhs);
Duration operator%(const Duration& lhs, const Duration& rhs);
Duration operator<<(const Duration& lhs, int8_t shift);
Duration operator>>(const Duration& lhs, int8_t shift);

//  Duration constants.  These are initialized in Thread.cpp because
//  of the "static initialization order fiasco".
//
extern const Duration TIMEOUT_IMMED;
extern const Duration TIMEOUT_NEVER;
extern const Duration ZERO_SECS;
extern const Duration ONE_uSEC;
extern const Duration ONE_mSEC;
extern const Duration ONE_SEC;
extern const int64_t TICKS_PER_uSEC;
extern const int64_t TICKS_PER_mSEC;
extern const int64_t TICKS_PER_SEC;
}
#endif
