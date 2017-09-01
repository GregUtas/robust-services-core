//==============================================================================
//
//  Clock.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Clock.h"
#include "Debug.h"
#include "Singleton.h"
#include "SysTickTimer.h"
#include "SysTime.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
ticks_t Clock::MsecsToTicks(msecs_t msecs)
{
   auto timer = Singleton< SysTickTimer >::Instance();
   return ((msecs * timer->TicksPerSec()) / 1000);
}

//------------------------------------------------------------------------------

ticks_t Clock::SecsToTicks(secs_t secs)
{
   auto timer = Singleton< SysTickTimer >::Instance();
   return (secs * timer->TicksPerSec());
}

//------------------------------------------------------------------------------

ticks_t Clock::TicksNow()
{
   return Singleton< SysTickTimer >::Instance()->TicksNow();
}

//------------------------------------------------------------------------------

ticks_t Clock::TicksPerSec()
{
   return Singleton< SysTickTimer >::Instance()->TicksPerSec();
}

//------------------------------------------------------------------------------

ticks_t Clock::TicksSince(const ticks_t& past)
{
   if(past == 0) return 0;
   auto now = Singleton< SysTickTimer >::Instance()->TicksNow();
   if(past > now) return 0;
   return now - past;
}

//------------------------------------------------------------------------------

ticks_t Clock::TicksSince(const ticks_t& past, ticks_t& now)
{
   if(past == 0) return 0;
   now = Singleton< SysTickTimer >::Instance()->TicksNow();
   if(past > now) return 0;
   return now - past;
}

//------------------------------------------------------------------------------

msecs_t Clock::TicksToMsecs(const ticks_t& ticks)
{
   auto timer = Singleton< SysTickTimer >::Instance();
   return ((ticks * 1000) / timer->TicksPerSec());
}

//------------------------------------------------------------------------------

secs_t Clock::TicksToSecs(const ticks_t& ticks)
{
   auto timer = Singleton< SysTickTimer >::Instance();
   return (ticks / timer->TicksPerSec());
}

//------------------------------------------------------------------------------

fn_name Clock_TicksToTime = "Clock.TicksToTime";

string Clock::TicksToTime(const ticks_t& ticks, TimeField field)
{
   Debug::ft(Clock_TicksToTime);

   if(ticks == 0) return ERROR_STR;

   auto timer = Singleton< SysTickTimer >::Instance();
   auto startTime = timer->StartTime();
   auto msecDiff = TicksToMsecs(ticks - timer->StartTick());

   startTime.AddMsecs(msecDiff);
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

usecs_t Clock::TicksToUsecs(const ticks_t& ticks)
{
   auto timer = Singleton< SysTickTimer >::Instance();
   return ((ticks * 1000000) / timer->TicksPerSec());
}

//------------------------------------------------------------------------------

ticks_t Clock::TicksUntil(const ticks_t& future)
{
   auto now = Singleton< SysTickTimer >::Instance()->TicksNow();
   if(future < now) return 0;
   return future - now;
}

//------------------------------------------------------------------------------

const ticks_t& Clock::TicksZero()
{
   return Singleton< SysTickTimer >::Instance()->StartTick();
}

//------------------------------------------------------------------------------

const SysTime& Clock::TimeZero()
{
   return Singleton< SysTickTimer >::Instance()->StartTime();
}

//------------------------------------------------------------------------------

const string& Clock::TimeZeroStr()
{
   return Singleton< SysTickTimer >::Instance()->StartTimeStr();
}

//------------------------------------------------------------------------------

ticks_t Clock::UsecsToTicks(usecs_t usecs)
{
   auto timer = Singleton< SysTickTimer >::Instance();
   return ((usecs * timer->TicksPerSec()) / 1000000);
}
}