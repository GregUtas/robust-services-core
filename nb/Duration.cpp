//==============================================================================
//
//  Duration.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "Duration.h"
#include "Debug.h"
#include "SysTickTimer.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
Duration::Duration() : ticks_(0) { }

//------------------------------------------------------------------------------

fn_name Duration_ctor = "Duration.ctor";

Duration::Duration(int64_t value, TimeUnits units) : ticks_(value)
{
   if((value == 0) || (value == Infinity) || (units == TICKS)) return;

   auto timer = SysTickTimer::Extant();
   if(timer == nullptr) return;

   int64_t tps = timer->TicksPerSec();

   switch(units)
   {
   case uSECS:
      ticks_ = (ticks_ * tps) / 1000000LL;
      return;
   case mSECS:
      ticks_ = (ticks_ * tps) / 1000LL;
      return;
   case SECS:
      ticks_ *= tps;
      return;
   case MINUTES:
      ticks_ *= (60LL * tps);
      return;
   case HOURS:
      ticks_ *= (3660LL * tps);
      return;
   case DAYS:
      ticks_ *= (86400LL * tps);
      return;
   default:
      Debug::SwLog(Duration_ctor, "invalid units", units);
   }
}

//------------------------------------------------------------------------------

Duration Duration::Immed()
{
   return Duration(0, TICKS);
}

//------------------------------------------------------------------------------

Duration Duration::Never()
{
   return Duration(Infinity, TICKS);
}

//------------------------------------------------------------------------------

Duration Duration::operator+()
{
   return Duration(*this);
}

//------------------------------------------------------------------------------

Duration Duration::operator-()
{
   if(ticks_ == Infinity) return Duration(INT64_MIN, TICKS);
   return Duration(-ticks_, TICKS);
}

//------------------------------------------------------------------------------

Duration& Duration::operator++()
{
   if(ticks_ == Infinity) return *this;
   ++ticks_;
   return *this;
}

//------------------------------------------------------------------------------

Duration Duration::operator++(int)
{
   if(ticks_ == Infinity) return *this;
   ticks_++;
   return Duration(*this);
}

//------------------------------------------------------------------------------

Duration& Duration::operator--()
{
   if(ticks_ == Infinity) return *this;
   --ticks_;
   return *this;
}

//------------------------------------------------------------------------------

Duration Duration::operator--(int)
{
   if(ticks_ == Infinity) return *this;
   ticks_--;
   return Duration(*this);
}

//------------------------------------------------------------------------------

Duration& Duration::operator+=(const Duration& rhs)
{
   if(ticks_ == Infinity) return *this;

   if(rhs.ticks_ == Infinity)
      ticks_ = rhs.ticks_;
   else
      ticks_ += rhs.ticks_;

   return *this;
}

//------------------------------------------------------------------------------

Duration& Duration::operator-=(const Duration& rhs)
{
   if(ticks_ == Infinity) return *this;

   if(rhs.ticks_ == Infinity)
      ticks_ = 0;
   else
      ticks_ -= rhs.ticks_;

   return *this;
}

//------------------------------------------------------------------------------

Duration& Duration::operator*=(int64_t rhs)
{
   if(ticks_ != Infinity) ticks_ *= rhs;
   return *this;
}

//------------------------------------------------------------------------------

Duration& Duration::operator/=(int64_t rhs)
{
   if(ticks_ != Infinity) ticks_ /= rhs;
   return *this;
}

//------------------------------------------------------------------------------

Duration& Duration::operator%=(const Duration& rhs)
{
   if(ticks_ != Infinity) ticks_ %= rhs.Ticks();
   return *this;
}

//------------------------------------------------------------------------------

Duration& Duration::operator<<=(int8_t shift)
{
   if(ticks_ != Infinity) ticks_ <<= shift;
   return *this;
}

//------------------------------------------------------------------------------

Duration& Duration::operator>>=(int8_t shift)
{
   if(ticks_ != Infinity) ticks_ >>= shift;
   return *this;
}

//------------------------------------------------------------------------------

fn_name Duration_To = "Duration.To";

int64_t Duration::To(TimeUnits units) const
{
   if((ticks_ == 0) || (ticks_ == Infinity) || (units == TICKS)) return ticks_;

   auto timer = SysTickTimer::Extant();
   if(timer == nullptr) return ticks_;

   int64_t tps = timer->TicksPerSec();

   switch(units)
   {
   case uSECS:
      return ticks_ * 1000000LL / tps;
   case mSECS:
      return ticks_ * 1000LL / tps;
   case SECS:
      return ticks_ / tps;
   case MINUTES:
      return ticks_ / (60LL * tps);
   case HOURS:
      return ticks_ / (3600LL * tps);
   case DAYS:
      return ticks_ / (86400LL * tps);
   default:
      Debug::SwLog(Duration_To, "invalid units", units);
   }

   return ticks_;
}

//------------------------------------------------------------------------------

string Duration::to_str(TimeUnits units) const
{
   if(ticks_ == Infinity) return "infinity";

   auto n = To(units);
   auto str = std::to_string(n);
   str.push_back(SPACE);

   switch(units)
   {
   case TICKS:
      return str.append("ticks");
   case uSECS:
      return str.append("usecs");
   case mSECS:
      return str.append("msecs");
   case SECS:
      return str.append("secs");
   case MINUTES:
      return str.append("mins");
   case HOURS:
      return str.append("hrs");
   case DAYS:
      return str.append("days");
   }

   return str.append(ERROR_STR);
}

//------------------------------------------------------------------------------

uint32_t Duration::ToMsecs() const
{
   if(ticks_ <= 0) return 0;
   if(ticks_ == Infinity) return UINT32_MAX;
   return To(mSECS);
}

//------------------------------------------------------------------------------

bool operator==(const Duration& lhs, const Duration& rhs)
{
   return (lhs.Ticks() == rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator!=(const Duration& lhs, const Duration& rhs)
{
   return (lhs.Ticks() != rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator<(const Duration& lhs, const Duration& rhs)
{
   return (lhs.Ticks() < rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator<=(const Duration& lhs, const Duration& rhs)
{
   return (lhs.Ticks() <= rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator>(const Duration& lhs, const Duration& rhs)
{
   return (lhs.Ticks() > rhs.Ticks());
}

//------------------------------------------------------------------------------

bool operator>=(const Duration& lhs, const Duration& rhs)
{
   return (lhs.Ticks() >= rhs.Ticks());
}

//------------------------------------------------------------------------------

Duration operator+(const Duration& lhs, const Duration& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return Duration::Never();
   return Duration(t1 + t2, TICKS);
}

//------------------------------------------------------------------------------

Duration operator-(const Duration& lhs, const Duration& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return Duration::Immed();
   return Duration(t1 - t2, TICKS);
}

//------------------------------------------------------------------------------

Duration operator*(const Duration& lhs, int64_t rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   return Duration(t1 * rhs, TICKS);
}

//------------------------------------------------------------------------------

Duration operator*(int64_t lhs, const Duration& rhs)
{
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return Duration::Never();
   return Duration(lhs * t2, TICKS);
}

//------------------------------------------------------------------------------

Duration operator/(const Duration& lhs, int64_t rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   return Duration(t1 / rhs, TICKS);
}

//------------------------------------------------------------------------------

int64_t operator/(const Duration& lhs, const Duration& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return INT64_MAX;
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return 0;
   return (t1 / t2);
}

//------------------------------------------------------------------------------

Duration operator%(const Duration& lhs, const Duration& rhs)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   auto t2 = rhs.Ticks();
   if(t2 == Duration::Infinity) return Duration::Never();
   return Duration(t1 % t2, TICKS);
}

//------------------------------------------------------------------------------

Duration operator<<(const Duration& lhs, int8_t shift)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   return Duration(t1 << shift, TICKS);
}

//------------------------------------------------------------------------------

Duration operator>>(const Duration& lhs, int8_t shift)
{
   auto t1 = lhs.Ticks();
   if(t1 == Duration::Infinity) return Duration::Never();
   return Duration(t1 >> shift, TICKS);
}
}
