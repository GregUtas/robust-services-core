//==============================================================================
//
//  SysTickTimer.win.cpp
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
#ifdef OS_WIN

#include "SysTickTimer.h"
#include <sys/timeb.h>
#include <windows.h>
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
SysTickTimer::SysTickTimer() :
   ticks_per_sec_(1000),
   available_(false)
{
   Debug::ft("SysTickTimer.ctor");

   LARGE_INTEGER frequency;

   if(QueryPerformanceFrequency(&frequency))
   {
      available_ = true;
      ticks_per_sec_ = frequency.QuadPart;
   }

   startPoint_ = Now();
   startTime_ = SysTime();

   startTimeStr_ = startTime_.to_str(SysTime::Numeric).c_str();
   auto pos = startTimeStr_.find('.');
   if(pos != string::npos) startTimeStr_[pos] = '-';
}

//------------------------------------------------------------------------------

TimePoint SysTickTimer::Now() const
{
   if(available_)
   {
      LARGE_INTEGER now;
      QueryPerformanceCounter(&now);
      return TimePoint(now.QuadPart);
   }
   else
   {
      _timeb now;
      _ftime_s(&now);
      auto msecs = 1000LL * now.time;
      return TimePoint(msecs + now.millitm);
   }
}

//------------------------------------------------------------------------------

void SysTickTimer::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}
}
#endif
