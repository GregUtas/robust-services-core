//==============================================================================
//
//  SysTime.win.cpp
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
#ifdef OS_WIN
#include "SysTime.h"
#include <ctime>
#include <sys/timeb.h>
#include <windows.h>
#include "Debug.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysTime_ctor2 = "SysTime.ctor(now)";

SysTime::SysTime()
{
   Debug::ft(SysTime_ctor2);

   errno_t err;
   time_t longtime;
   _timeb timebuff;
   tm timeofday;

   time(&longtime);

   err = _ftime_s(&timebuff);

   if(err != 0)
   {
      Debug::SwLog(SysTime_ctor2, "_ftime_s failed", err);
   }

   err = localtime_s(&timeofday, &longtime);

   if(err != 0)
   {
      Debug::SwLog(SysTime_ctor2, "localtime_s failed", err);
   }

   time_[YearsField] = timeofday.tm_year + 1900;
   time_[MonthsField] = timeofday.tm_mon;
   time_[DaysField] = timeofday.tm_mday;
   time_[HoursField] = timeofday.tm_hour;
   time_[MinsField] = timeofday.tm_min;
   time_[SecsField] = timeofday.tm_sec;
   time_[MsecsField] = timebuff.millitm;
}
}
#endif
