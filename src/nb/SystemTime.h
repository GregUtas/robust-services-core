//==============================================================================
//
//  SystemTime.h
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
#ifndef SYSTEMTIME_H_INCLUDED
#define SYSTEMTIME_H_INCLUDED

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>

namespace NodeBase
{
//  Formats for displaying time.
//
enum TimeFormat
{
   FullAlpha,    // DD-MMM-YYYY HH:MM:SS.mmm
   HighAlpha,    // DD-MMM-YYYY
   LowAlpha,     // HH:MM:SS.mmm
   FullNumeric,  // YYMMDD-HHMMSS
   HighNumeric,  // YYMMDD
   LowNumeric,   // HHMMSS.mmm
   MinSecMsecs   // MM:SS.mmm
};

//------------------------------------------------------------------------------
//
//  System time (time of day clock).
//
namespace SystemTime
{
   //  The underlying type for the time.
   //
   using Point = std::chrono::system_clock::time_point;

   //  Returns the current time.
   //
   Point Now();

   //  Returns the time when the system booted.
   //
   Point TimeZero();

   //  Returns an invalid time (the clock's epoch) for reinitialization
   //  purposes.  This is also the value set by the default constructor.
   //
   Point GetInvalid();

   //  Returns false if the time is not the value returned by GetInvalid.
   //
   bool IsValid(const Point& time);
}

//  Returns a string that displays the time in FORMAT.
//
std::string to_string(const SystemTime::Point& time, TimeFormat format);

//  Converts TIME to YMDHMS and the MSECS fraction.
//
void to_calendar_time
   (const SystemTime::Point& time, tm& ymdhms, uint32_t& msecs);
}
#endif
