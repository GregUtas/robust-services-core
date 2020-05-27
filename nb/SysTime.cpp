//==============================================================================
//
//  SysTime.cpp
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
#include "SysTime.h"
#include <iomanip>
#include <iosfwd>
#include <sstream>
#include "Debug.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const int16_t MinValues[TimeField_N] =
{
   SysTime::MinYear, 0, SysTime::MinDay, 0, 0, 0, 0
};

const int16_t MaxValues[TimeField_N] =
{
   SysTime::MaxYear, SysTime::MaxMonth, SysTime::MaxDay,
   SysTime::MaxHour, SysTime::MaxMin, SysTime::MaxSec, SysTime::MaxMsec
};

const int16_t MsecMultipliers[TimeField_N] =
{
   0, 0, 24, 60, 60, 1000, 1
};

const int16_t NonLeapYearDaysPerMonth[SysTime::MaxMonth + 1] =
{
   31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

const int16_t LeapYearDaysPerMonth[SysTime::MaxMonth + 1] =
{
   31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

fixed_string MonthStrings[SysTime::MaxMonth + 2] =
{
   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ERROR_STR
};

fixed_string WeekDayStrings[SysTime::MaxWeekDay + 2] =
{
   "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", ERROR_STR
};

//------------------------------------------------------------------------------

const int16_t SysTime::YearOfT0 = 1900;  // T0 = January 1 1900
const int16_t SysTime::WeekDayOfT0 = 1;  // January 1 1900 was a Monday

const int64_t SysTime::SecsInLeapYear    = 366 * 24 * 60 * 60;
const int64_t SysTime::SecsInNonLeapYear = 365 * 24 * 60 * 60;

//------------------------------------------------------------------------------

fn_name SysTime_ctor1 = "SysTime.ctor(explicit)";

SysTime::SysTime(int16_t year, int16_t month, int16_t day,
   int16_t hour, int16_t min, int16_t sec, int16_t msec)
{
   Debug::ft(SysTime_ctor1);

   time_[YearsField] = year;
   time_[MonthsField] = month;
   time_[DaysField] = day;
   time_[HoursField] = hour;
   time_[MinsField] = min;
   time_[SecsField] = sec;
   time_[MsecsField] = msec;

   Verify();
}

//------------------------------------------------------------------------------

fn_name SysTime_AddDays = "SysTime.AddDays";

void SysTime::AddDays(size_t days)
{
   Debug::ft(SysTime_AddDays);

   for(auto dpm = DaysPerMonth(time_[YearsField]); days > 0; --days)
   {
      time_[DaysField]++;

      if(time_[DaysField] > dpm[time_[MonthsField]])
      {
         time_[MonthsField]++;

         if(time_[MonthsField] > MaxMonth)
         {
            time_[YearsField]++;

            if(time_[YearsField] > MaxYear)
            {
               OutOfRange(YearsField);
               return;
            }

            dpm = DaysPerMonth(time_[YearsField]);
            time_[MonthsField] = 0;
         }

         time_[DaysField] = 1;
      }
   }
}

//------------------------------------------------------------------------------

fn_name SysTime_AddMsecs = "SysTime.AddMsecs";

void SysTime::AddMsecs(msecs_t msecs)
{
   Debug::ft(SysTime_AddMsecs);

   int carry;

   if(msecs == 0) return;
   time_[MsecsField] += (msecs % 1000);
   carry = (time_[MsecsField] >= 1000 ? 1 : 0);
   if(carry == 1) time_[MsecsField] -= 1000;
   msecs /= 1000;
   msecs += carry;

   if(msecs == 0) return;
   time_[SecsField] += (msecs % 60);
   carry = (time_[SecsField] >= 60 ? 1 : 0);
   if(carry == 1) time_[SecsField] -= 60;
   msecs /= 60;
   msecs += carry;

   if(msecs == 0) return;
   time_[MinsField] += (msecs % 60);
   carry = (time_[MinsField] >= 60 ? 1 : 0);
   if(carry == 1) time_[MinsField] -= 60;
   msecs /= 60;
   msecs += carry;

   if(msecs == 0) return;
   time_[HoursField] += (msecs % 24);
   carry = (time_[HoursField] >= 24 ? 1 : 0);
   if(carry == 1) time_[HoursField] -= 24;
   msecs /= 24;
   msecs += carry;

   if(msecs == 0) return;
   AddDays(msecs);
}

//------------------------------------------------------------------------------

int16_t SysTime::DayOfWeek() const
{
   //  This could be sped up by creating, during system initialization, an
   //  array that maps each year to one of the 14 perpetual calendars.
   //
   return (WeekDayOfT0 + DaysSinceT0()) % 7;
}

//------------------------------------------------------------------------------

int16_t SysTime::DayOfYear() const
{
   auto dpm = DaysPerMonth(time_[YearsField]);
   auto days = 0;

   for(auto m = time_[MonthsField] - 1; m >= 0; --m)
   {
      days += dpm[m];
   }

   days += (time_[DaysField] - 1);
   return days;
}

//------------------------------------------------------------------------------

const int16_t* SysTime::DaysPerMonth(int16_t year)
{
   if(IsLeapYear(year)) return LeapYearDaysPerMonth;
   return NonLeapYearDaysPerMonth;
}

//------------------------------------------------------------------------------

size_t SysTime::DaysSinceT0() const
{
   size_t days = 0;

   for(auto y = time_[YearsField] - 1; y >= YearOfT0; --y)
   {
      days += (IsLeapYear(y) ? 366 : 365);
   }

   return days + DayOfYear();
}

//------------------------------------------------------------------------------

void SysTime::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "year  : " << time_[YearsField] << CRLF;
   stream << prefix << "month : " << time_[MonthsField] << CRLF;
   stream << prefix << "day   : " << time_[DaysField] << CRLF;
   stream << prefix << "hours : " << time_[HoursField] << CRLF;
   stream << prefix << "mins  : " << time_[MinsField] << CRLF;
   stream << prefix << "secs  : " << time_[SecsField] << CRLF;
   stream << prefix << "msecs : " << time_[MsecsField] << CRLF;
}

//------------------------------------------------------------------------------

bool SysTime::IsLeapYear(int16_t year)
{
   if(year % 4 != 0) return false;
   if(year % 400 == 0) return true;
   return (year % 100 != 0);
}

//------------------------------------------------------------------------------

fn_name SysTime_MsecsFromNow = "SysTime.MsecsFromNow";

int32_t SysTime::MsecsFromNow() const
{
   Debug::ft(SysTime_MsecsFromNow);

   return SysTime().MsecsUntil(*this);
}

//------------------------------------------------------------------------------

int64_t SysTime::MsecsSinceT0() const
{
   int64_t msecs = 0;
   int64_t msly = SecsInLeapYear * 1000;
   int64_t msnly = SecsInNonLeapYear * 1000;

   for(auto y = time_[YearsField] - 1; y >= YearOfT0; --y)
   {
      msecs += (IsLeapYear(y) ? msly : msnly);
   }

   msecs += (1000LL * DayOfYear() * 24 * 60 * 60);
   msecs += (1000LL * time_[HoursField] * 60 * 60);
   msecs += (1000LL * time_[MinsField] * 60);
   msecs += (1000LL * time_[SecsField]);
   msecs += time_[MsecsField];
   return msecs;
}

//------------------------------------------------------------------------------

fn_name SysTime_MsecsUntil = "SysTime.MsecsUntil";

int32_t SysTime::MsecsUntil(const SysTime& time) const
{
   Debug::ft(SysTime_MsecsUntil);

   auto ms0 = MsecsSinceT0();
   auto ms1 = time.MsecsSinceT0();

   if(ms0 <= ms1)
   {
      auto diff = ms1 - ms0;
      if(diff <= INT32_MAX) return diff;
      Debug::SwLog(SysTime_MsecsUntil, "overflow", diff);
      return INT32_MAX;
   }

   auto diff = ms0 - ms1;
   if(diff <= INT32_MAX) return -diff;
   Debug::SwLog(SysTime_MsecsUntil, "underflow", diff);
   return INT32_MIN;
}

//------------------------------------------------------------------------------

fn_name SysTime_OutOfRange = "SysTime.OutOfRange";

bool SysTime::OutOfRange(TimeField field)
{
   Debug::ft(SysTime_OutOfRange);

   Debug::SwLog(SysTime_OutOfRange, "value out range", time_[field]);
   for(auto f = 0; f < TimeField_N; ++f) time_[f] = 0;
   return false;
}

//------------------------------------------------------------------------------

void SysTime::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SysTime_Round = "SysTime.Round";

void SysTime::Round(TimeField field, int16_t interval)
{
   Debug::ft(SysTime_Round);

   switch(field)
   {
   case HoursField:
   case MinsField:
   case SecsField:
   case MsecsField:
      if((interval > 0) &&
         (MsecMultipliers[field - 1] >= interval) &&
         (MsecMultipliers[field - 1] % interval == 0))
      {
         int32_t msecs = interval;
         int32_t above = 0;

         for(auto f = int(field); f < TimeField_N; ++f)
         {
            msecs *= MsecMultipliers[f];

            if(f == field)
               above += (time_[f] % interval);
            else
               above += time_[f];

            above *= MsecMultipliers[f];
         }

         Truncate(TimeField(int(field) + 1));

         time_[field] = (time_[field] / interval) * interval;

         if(above * 2 >= msecs)
         {
            AddMsecs(msecs);
         }
      }
      else
      {
         Debug::SwLog(SysTime_Round, "invalid field", field);
      }
      return;

   default:
      Debug::SwLog(SysTime_Round, "unexpected field", field);
      return;
   }
}

//------------------------------------------------------------------------------

c_string SysTime::strMonth() const
{
   auto month = time_[MonthsField];
   if((month >= 0) && (month <= MaxMonth)) return MonthStrings[month];
   return MonthStrings[MaxMonth + 1];
}

//------------------------------------------------------------------------------

c_string SysTime::strWeekDay() const
{
   return WeekDayStrings[DayOfWeek()];
}

//------------------------------------------------------------------------------

fn_name SysTime_SubDays = "SysTime.SubDays";

void SysTime::SubDays(size_t days)
{
   Debug::ft(SysTime_SubDays);

   for(auto dpm = DaysPerMonth(time_[YearsField]); days > 0; --days)
   {
      time_[DaysField]--;

      if(time_[DaysField] == 0)
      {
         time_[MonthsField]--;

         if(time_[MonthsField] < 0)
         {
            time_[YearsField]--;

            if(time_[YearsField] < MinYear)
            {
               OutOfRange(YearsField);
               return;
            }

            dpm = DaysPerMonth(time_[YearsField]);
            time_[MonthsField] = MaxMonth;
         }

         time_[DaysField] = dpm[time_[MonthsField]];
      }
   }
}

//------------------------------------------------------------------------------

fn_name SysTime_SubMsecs = "SysTime.SubMsecs";

void SysTime::SubMsecs(msecs_t msecs)
{
   Debug::ft(SysTime_SubMsecs);

   int borrow;

   if(msecs == 0) return;
   time_[MsecsField] -= (msecs % 1000);
   borrow = (time_[MsecsField] < 0 ? 1 : 0);
   if(borrow == 1) time_[MsecsField] += 1000;
   msecs /= 1000;
   msecs += borrow;

   if(msecs == 0) return;
   time_[SecsField] -= (msecs % 60);
   borrow = (time_[SecsField] < 0 ? 1 : 0);
   if(borrow == 1) time_[SecsField] += 60;
   msecs /= 60;
   msecs += borrow;

   if(msecs == 0) return;
   time_[MinsField] -= (msecs % 60);
   borrow = (time_[MinsField] < 0 ? 1 : 0);
   if(borrow == 1) time_[MinsField] += 60;
   msecs /= 60;
   msecs += borrow;

   if(msecs == 0) return;
   time_[HoursField] -= (msecs % 24);
   borrow = (time_[HoursField] < 0 ? 1 : 0);
   if(borrow == 1) time_[HoursField] += 24;
   msecs /= 24;
   msecs += borrow;

   if(msecs == 0) return;
   SubDays(msecs);
}

//------------------------------------------------------------------------------

string SysTime::to_str(Format format) const
{
   std::ostringstream stream;

   stream << std::setfill('0');

   switch(format)
   {
   case Alpha:
   case HighAlpha:
      stream << time_[DaysField] << '-';
      stream << strMonth() << '-';
      stream << time_[YearsField];
      if(format == HighAlpha) break;
      stream << SPACE;
      //  [[fallthrough]]
   case LowAlpha:
      stream << setw(2) << time_[HoursField] << ':';
      stream << setw(2) << time_[MinsField] << ':';
      stream << setw(2) << time_[SecsField] << '.';
      stream << setw(3) << time_[MsecsField];
      break;

   case Numeric:
   case HighNumeric:
      stream << setw(2) << time_[YearsField] % 100;
      stream << setw(2) << time_[MonthsField] + 1;
      stream << setw(2) << time_[DaysField];
      if(format == HighNumeric) break;
      stream << '-';
      //  [[fallthrough]]
   case LowNumeric:
      stream << setw(2) << time_[HoursField];
      stream << setw(2) << time_[MinsField];
      stream << setw(2) << time_[SecsField] << '.';
      stream << setw(3) << time_[MsecsField];
      break;
   }

   return stream.str();
}

//------------------------------------------------------------------------------

fn_name SysTime_Truncate = "SysTime.Truncate";

void SysTime::Truncate(TimeField field)
{
   Debug::ft(SysTime_Truncate);

   for(auto f = int(field); f < TimeField_N; ++f) time_[f] = MinValues[f];
}

//------------------------------------------------------------------------------

fn_name SysTime_Verify = "SysTime.Verify";

bool SysTime::Verify()
{
   Debug::ft(SysTime_Verify);

   for(auto f = 0; f < TimeField_N; ++f)
   {
      if((time_[f] < MinValues[f]) || (time_[f] > MaxValues[f]))
      {
         return OutOfRange(TimeField(f));
      }
   }

   auto dpm = DaysPerMonth(time_[YearsField]);
   if(time_[DaysField] > dpm[time_[MonthsField]]) return OutOfRange(DaysField);
   return true;
}
}
