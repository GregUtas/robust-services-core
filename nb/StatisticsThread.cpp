//==============================================================================
//
//  StatisticsThread.cpp
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
#include "StatisticsThread.h"
#include <sstream>
#include <string>
#include "Debug.h"
#include "Log.h"
#include "NbLogs.h"
#include "Singleton.h"
#include "StatisticsRegistry.h"
#include "SysTime.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
secs_t StatisticsThread::LongIntervalSecs = 900;  // must be a multiple of 60
secs_t StatisticsThread::ShortIntervalSecs = 5;   // must be a divisor of 60

size_t StatisticsThread::WakeupsBetweenReports =
   StatisticsThread::LongIntervalSecs / StatisticsThread::ShortIntervalSecs;

ticks_t StatisticsThread::PrevToCurrTicks =
   Clock::SecsToTicks(StatisticsThread::ShortIntervalSecs);

//------------------------------------------------------------------------------

fn_name StatisticsThread_ctor = "StatisticsThread.ctor";

StatisticsThread::StatisticsThread() : Thread(BackgroundFaction),
   wakeupTicks_(0),
   countdown_(WakeupsBetweenReports),
   delayed_(false)
{
   Debug::ft(StatisticsThread_ctor);
}

//------------------------------------------------------------------------------

fn_name StatisticsThread_dtor = "StatisticsThread.dtor";

StatisticsThread::~StatisticsThread()
{
   Debug::ft(StatisticsThread_dtor);
}

//------------------------------------------------------------------------------

c_string StatisticsThread::AbbrName() const
{
   return "stats";
}

//------------------------------------------------------------------------------

fn_name StatisticsThread_CalcFirstDelay = "StatisticsThread.CalcFirstDelay";

msecs_t StatisticsThread::CalcFirstDelay()
{
   Debug::ft(StatisticsThread_CalcFirstDelay);

   SysTime timeNow;
   auto ticksNow = Clock::TicksNow();

   //  Start the first short interval for thread statistics at the next
   //  time that is at least half the distance between short intervals.
   //
   auto tshort = timeNow;
   tshort.AddMsecs(1000 * ShortIntervalSecs);
   tshort.Round(SecsField, ShortIntervalSecs);
   auto delay = timeNow.MsecsUntil(tshort);

   if((delay < 0) || (delay > 1500 * ShortIntervalSecs))
   {
      Debug::SwLog(StatisticsThread_CalcFirstDelay, "invalid delay", delay);
      delay = 1000 * ShortIntervalSecs;
   }

   //  Start the first long interval for thread statistics at the next
   //  time that is at least half the distance between long intervals.
   //
   auto tlong = timeNow;
   tlong.AddMsecs(1000 * LongIntervalSecs);
   tlong.Round(MinsField, LongIntervalSecs / 60);
   auto delta = tshort.MsecsUntil(tlong);

   if((delta < 0) || (delta > 1500 * LongIntervalSecs))
   {
      Debug::SwLog(StatisticsThread_CalcFirstDelay, "invalid delta", delta);
      countdown_ = WakeupsBetweenReports;
   }
   else
   {
      countdown_ = (delta / (1000 * ShortIntervalSecs)) + 1;
   }

   wakeupTicks_ = ticksNow + Clock::MsecsToTicks(delay);
   return delay;
}

//------------------------------------------------------------------------------

fn_name StatisticsThread_Destroy = "StatisticsThread.Destroy";

void StatisticsThread::Destroy()
{
   Debug::ft(StatisticsThread_Destroy);

   Singleton< StatisticsThread >::Destroy();
}

//------------------------------------------------------------------------------

void StatisticsThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "wakeupTicks : " << wakeupTicks_ << CRLF;
   stream << prefix << "countdown   : " << countdown_ << CRLF;
   stream << prefix << "delayed     : " << delayed_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name StatisticsThread_Enter = "StatisticsThread.Enter";

void StatisticsThread::Enter()
{
   Debug::ft(StatisticsThread_Enter);

   auto reg = Singleton< StatisticsRegistry >::Instance();
   auto sleep = CalcFirstDelay();

   while(true)
   {
      Pause(sleep);

      //  Start the next short interval for thread statistics.
      //
      Thread::StartShortInterval();

      if(countdown_ > 0) --countdown_;

      if((countdown_ == 0) || delayed_)
      {
         //  Generate a statistics report.
         //
         auto log = Log::Create(StatsLogGroup, StatsReport);

         if(log != nullptr)
         {
            *log << Log::Tab;
            reg->DisplayStats(*log, VerboseOpt);
            Log::Submit(log);
            delayed_ = false;
         }
         else
         {
            //  Setting this flag will cause repeated attempts to
            //  generate the failed statistics report.
            //
            delayed_ = true;
         }

         if(countdown_ == 0)
         {
            //  At the end of the interval, start a new one in which
            //  statistics from the "current" interval become the ones
            //  from the "previous" interval and get merged into the
            //  overall statistics.
            //
            countdown_ = WakeupsBetweenReports;
            reg->StartInterval(false);
         }
      }

      //  Calculate the time when we want to wake up and sleep until then.
      //
      wakeupTicks_ += PrevToCurrTicks;
      sleep = Clock::TicksToMsecs(Clock::TicksUntil(wakeupTicks_));
   }
}

//------------------------------------------------------------------------------

void StatisticsThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}
