//==============================================================================
//
//  SysTickTimer.win.cpp
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
#include "SysTickTimer.h"
#include <sys/timeb.h>
#include <windows.h>
#include "Debug.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysTickTimer_ctor = "SysTickTimer.ctor";

SysTickTimer::SysTickTimer() :
   ticks_per_sec_(1000),
   startTick_(0),
   available_(false)
{
   Debug::ft(SysTickTimer_ctor);

   LARGE_INTEGER frequency;

   if(QueryPerformanceFrequency(&frequency))
   {
      available_ = true;
      ticks_per_sec_ = frequency.QuadPart;
   }

   startTick_ = TicksNow();
   startTime_ = SysTime();

   startTimeStr_ = startTime_.to_str(SysTime::Numeric);
   auto pos = startTimeStr_.find('.');
   if(pos != string::npos) startTimeStr_[pos] = '-';
}

//------------------------------------------------------------------------------

fn_name SysTickTimer_dtor = "SysTickTimer.dtor";

SysTickTimer::~SysTickTimer()
{
   Debug::ft(SysTickTimer_dtor);
}

//------------------------------------------------------------------------------

ticks_t SysTickTimer::TicksNow() const
{
   if(available_)
   {
      LARGE_INTEGER now;
      QueryPerformanceCounter(&now);
      return now.QuadPart;
   }
   else
   {
      _timeb now;
      _ftime_s(&now);
      ticks_t msecs = 1000 * now.time;
      return msecs + now.millitm;
   }
}
}
#endif