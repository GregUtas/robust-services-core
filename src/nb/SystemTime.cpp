//==============================================================================
//
//  SystemTime.cpp
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
#include "SystemTime.h"
#include <iomanip>
#include <iosfwd>
#include <ratio>
#include <sstream>
#include "Duration.h"
#include "SoftwareException.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
const SystemTime::Point SysBootTime = std::chrono::system_clock::now();

//------------------------------------------------------------------------------

SystemTime::Point SystemTime::GetInvalid()
{
   return Point::min();
}

//------------------------------------------------------------------------------

bool SystemTime::IsValid(const Point& time)
{
   return (time != GetInvalid());
}

//------------------------------------------------------------------------------

SystemTime::Point SystemTime::Now()
{
   return std::chrono::system_clock::now();
}

//------------------------------------------------------------------------------

SystemTime::Point SystemTime::TimeZero()
{
   return SysBootTime;
}

//------------------------------------------------------------------------------

void to_calendar_time
   (const SystemTime::Point& time, tm& ymdhms, uint32_t& msecs)
{
   //  Windows does it one way, Linux the other!
   //
   auto ok = false;
   auto ttyp = std::chrono::system_clock::to_time_t(time);

#ifdef OS_WIN
   localtime_s(&ymdhms, &ttyp);
   ok = true;
#endif

#ifdef OS_LINUX
   localtime_r(&ttyp, &ymdhms);
   ok = true;
#endif

   if(!ok)
   {
      throw SoftwareException("Platform needs to support ToCalendarTime", 0);
   }

   //  Extract the milliseconds.
   //
   nsecs_t nsecs(time.time_since_epoch());
   msecs = (nsecs.count() % std::nano::den) / NS_TO_MS;
}

//------------------------------------------------------------------------------

std::string to_string(const SystemTime::Point& time, TimeFormat format)
{
   //  Convert the time to YYYY-MM-DD-HH-MM-SS.mmm.
   //
   tm ymdhms;
   uint32_t msecs;
   std::ostringstream stream;

   to_calendar_time(time, ymdhms, msecs);

   switch(format)
   {
   case FullAlpha:
      stream << std::put_time(&ymdhms, "%b-%d-%Y %T.");
      stream << std::setw(3) << std::setfill('0') << msecs;
      break;
   case HighAlpha:
      stream << std::put_time(&ymdhms, "%b-%d-%Y");
      break;
   case LowAlpha:
      stream << std::put_time(&ymdhms, "%T.");
      stream << std::setw(3) << std::setfill('0') << msecs;
      break;
   case FullNumeric:
      stream << std::put_time(&ymdhms, "%y%m%d-%H%M%S");
      break;
   case HighNumeric:
      stream << std::put_time(&ymdhms, "%y%m%d");
      break;
   case LowNumeric:
      stream << std::put_time(&ymdhms, "%H%M%S.");
      stream << std::setw(3) << std::setfill('0') << msecs;
      break;
   case MinSecMsecs:
      stream << std::put_time(&ymdhms, "%M:%S.");
      stream << std::setw(3) << std::setfill('0') << msecs;
      break;
   }

   return stream.str();
}
}
