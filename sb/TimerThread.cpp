//==============================================================================
//
//  TimerThread.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "TimerThread.h"
#include "Clock.h"
#include "Debug.h"
#include "SbTracer.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TimerRegistry.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name TimerThread_ctor = "TimerThread.ctor";

TimerThread::TimerThread() : Thread(PayloadFaction)
{
   Debug::ft(TimerThread_ctor);
}

//------------------------------------------------------------------------------

fn_name TimerThread_dtor = "TimerThread.dtor";

TimerThread::~TimerThread()
{
   Debug::ft(TimerThread_dtor);
}

//------------------------------------------------------------------------------

const char* TimerThread::AbbrName() const
{
   return "timer";
}

//------------------------------------------------------------------------------

TraceStatus TimerThread::CalcStatus(bool dynamic) const
{
   //  When determining whether to trace a timer thread, this function takes
   //  the ">include/exclude/clear timers" commands into account.  Settings
   //  have the following precedence:
   //  1. The setting for this thread.
   //  2. The setting for timer work.
   //  3. The setting for this thread's faction.
   //  4. The ">include all on/off" command.
   //  The latter two are handled by invoking the Thread class.
   //
   auto status = GetStatus();
   if(status != TraceDefault) return status;
   status = Singleton< SbTracer >::Instance()->TimersStatus();
   if(status != TraceDefault) return status;
   return Thread::CalcStatus(dynamic);
}

//------------------------------------------------------------------------------

fn_name TimerThread_Destroy = "TimerThread.Destroy";

void TimerThread::Destroy()
{
   Debug::ft(TimerThread_Destroy);

   Singleton< TimerThread >::Destroy();
}

//------------------------------------------------------------------------------

fn_name TimerThread_Enter = "TimerThread.Enter";

void TimerThread::Enter()
{
   Debug::ft(TimerThread_Enter);

   //  Every second, tell our registry to process the next timer queue.
   //
   auto reg = Singleton< TimerRegistry >::Instance();
   auto sleep = TIMEOUT_1_SEC;

   while(true)
   {
      Pause(sleep);
      reg->ProcessWork();

      //  Sleep for one second, minus the amount of time that we just ran.
      //
      auto runTime = MsecsSinceStart();
      sleep = (runTime > 1000 ? 0 : 1000 - runTime);
   }
}

//------------------------------------------------------------------------------

void TimerThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}