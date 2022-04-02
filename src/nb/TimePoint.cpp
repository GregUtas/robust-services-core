//==============================================================================
//
//  TimePoint.cpp
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
#include "TimePoint.h"
#include "SysTickTimer.h"
#include "SysTime.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
TimePoint::TimePoint() : ts_(0) { }

//------------------------------------------------------------------------------

TimePoint::TimePoint(const SysTime& time) : ts_(0)
{
   auto timer = SysTickTimer::Extant();
   if(timer == nullptr) return;
   auto msecs1 = timer->StartTime().MsecsSinceT0();
   auto msecs2 = time.MsecsSinceT0();
   Duration diff(msecs2 - msecs1, mSECS);
   auto time0 = TimeZero();
   ts_ = time0.ts_ + diff.Ticks();
}

//------------------------------------------------------------------------------

TimePoint::TimePoint(int64_t value) : ts_(value) { }

//------------------------------------------------------------------------------

TimePoint TimePoint::Never()
{
   return TimePoint(Duration::Infinity);
}

//------------------------------------------------------------------------------

TimePoint TimePoint::Now()
{
   auto timer = SysTickTimer::Extant();
   if(timer == nullptr) return TimePoint(0);
   return TimePoint(timer->Now());
}

//------------------------------------------------------------------------------

TimePoint& TimePoint::operator++()
{
   if(ts_ == Duration::Infinity) return *this;
   ++ts_;
   return *this;
}

//------------------------------------------------------------------------------

TimePoint TimePoint::operator++(int)
{
   if(ts_ == Duration::Infinity) return *this;
   ts_++;
   return TimePoint(*this);
}

//------------------------------------------------------------------------------

TimePoint& TimePoint::operator--()
{
   if(ts_ == Duration::Infinity) return *this;
   --ts_;
   return *this;
}

//------------------------------------------------------------------------------

TimePoint TimePoint::operator--(int)
{
   if(ts_ == Duration::Infinity) return *this;
   ts_--;
   return TimePoint(*this);
}

//------------------------------------------------------------------------------

TimePoint& TimePoint::operator+=(const Duration& rhs)
{
   if(ts_ == Duration::Infinity) return *this;

   auto ticks = rhs.Ticks();

   if(ticks == Duration::Infinity)
      ts_ = Duration::Infinity;
   else
      ts_ += ticks;

   return *this;
}

//------------------------------------------------------------------------------

TimePoint& TimePoint::operator-=(const Duration& rhs)
{
   if(ts_ == Duration::Infinity) return *this;

   auto ticks = rhs.Ticks();

   if(ticks == Duration::Infinity)
      ts_ = 0;
   else
      ts_ -= ticks;

   return *this;
}

//------------------------------------------------------------------------------

TimePoint TimePoint::TimeZero()
{
   auto timer = SysTickTimer::Extant();
   if(timer == nullptr) return TimePoint(0);
   return TimePoint(timer->StartPoint());
}

//------------------------------------------------------------------------------

string TimePoint::TimeZeroStr()
{
   auto timer = SysTickTimer::Extant();
   if(timer == nullptr) return ERROR_STR;
   return timer->StartTimeStr();
}

//------------------------------------------------------------------------------

string TimePoint::to_str(TimeField field) const
{
   if(ts_ == 0) return "--:--.---";

   auto timer = SysTickTimer::Extant();
   if(timer == nullptr) return ERROR_STR;
   auto startTime = timer->StartTime();
   auto diff = (*this - timer->StartPoint());

   startTime.AddMsecs(diff.To(mSECS));
   auto time = startTime.to_str(SysTime::LowAlpha);

   switch(field)
   {
   case HoursField: return time;
   case MinsField: return time.substr(3);
   case SecsField: return time.substr(6);
   case MsecsField: return time.substr(9);
   }

   return time;
}

//------------------------------------------------------------------------------

bool operator==(const TimePoint& lhs, const TimePoint& rhs)
{
   return (lhs.Ticks() == rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator!=(const TimePoint& lhs, const TimePoint& rhs)
{
   return (lhs.Ticks() != rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator<(const TimePoint& lhs, const TimePoint& rhs)
{
   return (lhs.Ticks() < rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator<=(const TimePoint& lhs, const TimePoint& rhs)
{
   return (lhs.Ticks() <= rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator>(const TimePoint& lhs, const TimePoint& rhs)
{
   return (lhs.Ticks() > rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator>=(const TimePoint& lhs, const TimePoint& rhs)
{
   return (lhs.Ticks() >= rhs.Ticks());
}

//------------------------------------------------------------------------------

TimePoint operator+(const TimePoint& lhs, const Duration& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return TimePoint::Never();
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return TimePoint::Never();
   return TimePoint(t1 + t2);
}

//------------------------------------------------------------------------------

TimePoint operator+(const Duration& lhs, const TimePoint& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return TimePoint::Never();
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return TimePoint::Never();
   return TimePoint(t1 + t2);
}

//------------------------------------------------------------------------------

Duration operator-(const TimePoint& lhs, const Duration& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return Duration::Immed();
   return Duration(t1 - t2, TICKS);
}

//------------------------------------------------------------------------------

Duration operator-(const TimePoint& lhs, const TimePoint& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return Duration::Immed();
   return Duration(t1 - t2, TICKS);
}
}
