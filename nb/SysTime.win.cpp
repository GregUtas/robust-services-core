//==============================================================================
//
//  SysTime.win.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifdef OS_WIN
#include "SysTime.h"
#include <ctime>
#include <sys/timeb.h>
#include <windows.h>
#include "Debug.h"
#include "SysTypes.h"

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
      Debug::SwErr(SysTime_ctor2, err, 0);
   }

   err = localtime_s(&timeofday, &longtime);

   if(err != 0)
   {
      Debug::SwErr(SysTime_ctor2, err, 1);
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
