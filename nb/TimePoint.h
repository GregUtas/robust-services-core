//==============================================================================
//
//  TimePoint.h
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
#ifndef TIMEPOINT_H_INCLUDED
#define TIMEPOINT_H_INCLUDED

#include <cstdint>
#include <string>
#include "Duration.h"

namespace NodeBase
{
   class SysTime;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A point in time (timestamp).
//
class TimePoint
{
public:
   //  Converts TIME to a timestamp.
   //
   explicit TimePoint(const SysTime& time);

   //  For constructing a timestamp from a tick count.
   //  Not intended for use by applications.
   //
   explicit TimePoint(int64_t value);

   //  Constructs an invalid timestamp.
   //
   TimePoint();

   //  Not subclassed.
   //
   ~TimePoint() = default;

   //  Copy constructor.
   //
   TimePoint(const TimePoint& that) = default;

   //  Copy operator.
   //
   TimePoint& operator=(const TimePoint& that) = default;

   //  Returns the time when the executable initialized.
   //
   static TimePoint TimeZero();

   //  Returns the time (string) when the executable initialized.
   //
   static std::string TimeZeroStr();

   //  Returns the time now.
   //
   static TimePoint Now();

   //  Returns a time that will never be reached (Infinity).
   //
   static TimePoint Never();

   //  Converts the timepoint to a time-of-day string (hh:mm:ss.mmm).
   //  Truncates the string if FIELD is MinsField, SecsField, or MsecsField.
   //
   std::string to_str(TimeField field = HoursField) const;

   //  Returns the timestamp in ticks.
   //
   int64_t Ticks() const { return ts_; }

   //  Returns true if the timestamp has been set.
   //
   bool IsValid() const { return (ts_ != 0); }

   //  Adds a tick to the timestamp.
   //
   TimePoint& operator++();
   TimePoint operator++(int);

   //  Subtracts a tick from the timestamp.
   //
   TimePoint& operator--();
   TimePoint operator--(int);

   //  Adds RHS to the timestamp.
   //
   TimePoint& operator+=(const Duration& rhs);

   //  Subtracts RHS from the timestamp.
   //
   TimePoint& operator-=(const Duration& rhs);
private:
   //  The timestamp, which is always in ticks.
   //
   int64_t ts_;
};

//  TimePoint comparison operators.
//
bool operator==(const TimePoint& lhs, const TimePoint& rhs);
bool operator!=(const TimePoint& lhs, const TimePoint& rhs);
bool operator<(const TimePoint& lhs, const TimePoint& rhs);
bool operator<=(const TimePoint& lhs, const TimePoint& rhs);
bool operator>(const TimePoint& lhs, const TimePoint& rhs);
bool operator>=(const TimePoint& lhs, const TimePoint& rhs);

//  TimePoint arithmetic operators.
//
TimePoint operator+(const TimePoint& lhs, const Duration& rhs);
TimePoint operator+(const Duration& lhs, const TimePoint& rhs);
Duration operator-(const TimePoint& lhs, const Duration& rhs);
Duration operator-(const TimePoint& lhs, const TimePoint& rhs);
}
#endif
