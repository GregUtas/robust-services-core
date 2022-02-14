//==============================================================================
//
//  StatisticsThread.cpp
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
#include "StatisticsThread.h"
#include <cstdint>
#include <sstream>
#include <string>
#include "Debug.h"
#include "Log.h"
#include "NbDaemons.h"
#include "NbLogs.h"
#include "Restart.h"
#include "Singleton.h"
#include "StatisticsRegistry.h"
#include "SysTime.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The number of seconds between statistics reports and the rollover
//  of statistics (default = 15 minutes).
//
constexpr secs_t LongIntervalSecs = 900;

//> The number of seconds between the rollover of the short interval
//  for thread statistics (default = 5 seconds).
//
constexpr secs_t ShortIntervalSecs = 5;

//  The number of wakeups between statistics reports, which is equal
//  to SecondsInStatsInterval / SecondsInThreadInterval.  The thread
//  wakes up frequently to roll over thread statistics, but does so
//  for other statistics every SecondsInStatsInterval.
//
constexpr size_t WakeupsBetweenReports =
   LongIntervalSecs / ShortIntervalSecs;

//> The interval between the times when the thread starts to run.
//
static const Duration SleepInterval = Duration(ShortIntervalSecs, SECS);

//------------------------------------------------------------------------------

StatisticsThread::StatisticsThread() :
   Thread(BackgroundFaction, Singleton< StatisticsDaemon >::Instance()),
   countdown_(WakeupsBetweenReports),
   delayed_(false)
{
   Debug::ft("StatisticsThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

StatisticsThread::~StatisticsThread()
{
   Debug::ftnt("StatisticsThread.dtor");
}

//------------------------------------------------------------------------------

c_string StatisticsThread::AbbrName() const
{
   return StatisticsDaemonName;
}

//------------------------------------------------------------------------------

fn_name StatisticsThread_CalcFirstDelay = "StatisticsThread.CalcFirstDelay";

Duration StatisticsThread::CalcFirstDelay()
{
   Debug::ft(StatisticsThread_CalcFirstDelay);

   SysTime timeNow;

   //  Start the first short interval for thread statistics at the next
   //  time that is at least half the distance between short intervals.
   //
   auto tshort = timeNow;
   tshort.AddMsecs(1000 * ShortIntervalSecs);
   tshort.Round(SecsField, ShortIntervalSecs);
   auto delay = timeNow.MsecsUntil(tshort);

   if((delay < 0) || (delay > 1500 * int32_t(ShortIntervalSecs)))
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

   if((delta < 0) || (delta > 1500 * int32_t(LongIntervalSecs)))
   {
      Debug::SwLog(StatisticsThread_CalcFirstDelay, "invalid delta", delta);
      countdown_ = WakeupsBetweenReports;
   }
   else
   {
      countdown_ = delta / (1000 * int32_t(ShortIntervalSecs)) + 1;
   }

   Duration sleepTime(delay, mSECS);
   wakeupTime_ = TimePoint::Now() + sleepTime;
   return sleepTime;
}

//------------------------------------------------------------------------------

void StatisticsThread::Destroy()
{
   Debug::ft("StatisticsThread.Destroy");

   Singleton< StatisticsThread >::Destroy();
}

//------------------------------------------------------------------------------

void StatisticsThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "wakeupTime : " << wakeupTime_.Ticks() << CRLF;
   stream << prefix << "countdown  : " << countdown_ << CRLF;
   stream << prefix << "delayed    : " << delayed_ << CRLF;
}

//------------------------------------------------------------------------------

void StatisticsThread::Enter()
{
   Debug::ft("StatisticsThread.Enter");

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
      wakeupTime_ += SleepInterval;
      sleep = wakeupTime_ - TimePoint::Now();
   }
}

//------------------------------------------------------------------------------

bool StatisticsThread::ExitOnRestart(RestartLevel level) const
{
   Debug::ft("StatisticsThread.ExitOnRestart");

   //  Generate a statistics report if statistics will disappear
   //  during the restart.
   //
   auto reg = Singleton< StatisticsRegistry >::Instance();

   if(Restart::ClearsMemory(reg->MemType()))
   {
      auto log = Log::Create(StatsLogGroup, StatsReport);

      if(log != nullptr)
      {
         *log << Log::Tab;
         reg->DisplayStats(*log, VerboseOpt);
         Log::Submit(log);
      }
   }

   return true;
}

//------------------------------------------------------------------------------

void StatisticsThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}
