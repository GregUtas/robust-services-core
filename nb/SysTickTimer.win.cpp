//==============================================================================
//
//  SysTickTimer.win.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   auto index = startTimeStr_.find('.');
   if(index != string::npos) startTimeStr_.erase(index);
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