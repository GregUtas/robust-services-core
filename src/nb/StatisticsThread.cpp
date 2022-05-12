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
#include <chrono>
#include <cstdint>
#include <ratio>
#include <sstream>
#include <string>
#include "Debug.h"
#include "Log.h"
#include "NbDaemons.h"
#include "NbLogs.h"
#include "Restart.h"
#include "Singleton.h"
#include "StatisticsRegistry.h"
#include "SystemTime.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The number of seconds between statistics reports and the rollover
//  of statistics (default = 15 minutes).
//
constexpr uint32_t LongIntervalMins = 15;

//> The number of seconds between the rollover of the short interval
//  for thread statistics (default = 5 seconds).
//
constexpr uint32_t ShortIntervalSecs = 5;

//  The number of wakeups between statistics reports, which is equal
//  to SecondsInStatsInterval / SecondsInThreadInterval.  The thread
//  wakes up frequently to roll over thread statistics, but does so
//  for other statistics every SecondsInStatsInterval.
//
constexpr size_t WakeupsBetweenReports =
   60 * LongIntervalMins / ShortIntervalSecs;

//> The interval between the times when the thread starts to run.
//
static const msecs_t SleepInterval = msecs_t(SECS_TO_MS * ShortIntervalSecs);

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

msecs_t StatisticsThread::CalcFirstDelay()
{
   Debug::ft(StatisticsThread_CalcFirstDelay);

   //  Start the first short interval at the next time that is at least
   //  half the distance between short intervals.
   //
   nsecs_t nsecs(SystemTime::Now().time_since_epoch());
   uint64_t msecs = nsecs.count() / NS_TO_MS;

   int sleep = SECS_TO_MS * ShortIntervalSecs;
   auto msecs_past = msecs % sleep;
   int delay = sleep - msecs_past;
   if(delay < (sleep >> 1)) delay += sleep;

   if((delay < 0) || (delay > (sleep * 3) >> 1))
   {
      Debug::SwLog(StatisticsThread_CalcFirstDelay, "invalid delay", delay);
      delay = SECS_TO_MS * ShortIntervalSecs;
   }

   //  Start the first long interval for statistics at the next time
   //  that is at least half the distance between long intervals.
   //
   sleep = 60 * SECS_TO_MS * LongIntervalMins;
   msecs_past = msecs % sleep;
   int delta = sleep - msecs_past;
   if(delta < (sleep >> 1)) delta += sleep;

   if((delta < 0) || (delay > (delta * 3) >> 1))
   {
      Debug::SwLog(StatisticsThread_CalcFirstDelay, "invalid delta", delta);
      countdown_ = WakeupsBetweenReports;
   }
   else
   {
      countdown_ = delta / (SECS_TO_MS * ShortIntervalSecs) + 1;
   }

   msecs_t sleepTime(delay);
   wakeupTime_ = SteadyTime::Now() + sleepTime;
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

   stream << prefix << "countdown : " << countdown_ << CRLF;
   stream << prefix << "delayed   : " << delayed_ << CRLF;
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
      nsecs_t diff = wakeupTime_ - SteadyTime::Now();
      sleep = msecs_t(diff.count() / NS_TO_MS);
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
