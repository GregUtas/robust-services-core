//==============================================================================
//
//  TimerRegistry.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "TimerRegistry.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Restart.h"
#include "SbPools.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ThisThread.h"
#include "TimerThread.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name TimerRegistry_ctor = "TimerRegistry.ctor";

TimerRegistry::TimerRegistry() :
   nextQid_(0),
   currTimer_(nullptr),
   corrupt_(false)
{
   Debug::ft(TimerRegistry_ctor);

   for(auto i = 0; i <= Timer::MaxQId; ++i)
   {
      timerq_[i].Init(Timer::LinkDiff());
   }
}

//------------------------------------------------------------------------------

fn_name TimerRegistry_dtor = "TimerRegistry.dtor";

TimerRegistry::~TimerRegistry()
{
   Debug::ftnt(TimerRegistry_dtor);

   Debug::SwLog(TimerRegistry_dtor, UnexpectedInvocation, 0);

   for(auto i = 0; i <= Timer::MaxQId; ++i)
   {
      timerq_[i].Purge();
      ThisThread::PauseOver(95);
   }
}

//------------------------------------------------------------------------------

fn_name TimerRegistry_CalcQId = "TimerRegistry.CalcQId";

Timer::QId TimerRegistry::CalcQId(secs_t secs) const
{
   Debug::ft(TimerRegistry_CalcQId);

   //  The timer thread wakes up 1000 msecs after it last began to run.
   //  If it last began to run over 500 msecs ago, the next timer queue
   //  will be served in less than half a second, so increment SECS.
   //
   auto thr = Singleton< TimerThread >::Instance();
   auto incr = (thr->CurrTimeRunning().To(mSECS) >= 500 ? 1 : 0);

   secs += incr;
   if(secs >= Timer::MaxQId) return Timer::MaxQId;

   auto qid = nextQid_ + secs - 1;

   if(qid >= Timer::MaxQId) qid = qid - Timer::MaxQId;

   return qid;
}

//------------------------------------------------------------------------------

fn_name TimerRegistry_ClaimBlocks = "TimerRegistry.ClaimBlocks";

void TimerRegistry::ClaimBlocks()
{
   Debug::ft(TimerRegistry_ClaimBlocks);

   //  This doesn't actually claim timers in the timer registry.  Each timer is
   //  owned by a PSM, so timers are claimed via ProtocolSMPool::ClaimBlocks.
   //  What this does, however, is traverse all of the timer queues to ensure
   //  that they are not corrupt.
   //
   if(corrupt_)
   {
      Restart::Initiate(RestartCold, TimerQueueCorruption, 0);
   }

   corrupt_ = true;

   for(auto i = 0; i <= Timer::MaxQId; ++i)
   {
      for(auto t = timerq_[i].First(); t != nullptr; timerq_[i].Next(t));
   }

   corrupt_ = false;
}

//------------------------------------------------------------------------------

void TimerRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "nextQid : " << nextQid_ << CRLF;
   stream << prefix << "corrupt : " << corrupt_ << CRLF;

   auto lead = prefix + spaces(2);

   stream << prefix << "timerq [Timer::QId] (first entry only)" << CRLF;

   for(auto i = nextQid_; i <= Timer::MaxQId; ++i)
   {
      auto t = timerq_[i].First();

      if(t != nullptr)
      {
         auto psm = reinterpret_cast< const Base* >(t->Psm());
         stream << lead << strIndex(i) << strObj(psm) << CRLF;
         return;
      }
   }

   for(auto i = 0; i < nextQid_; ++i)
   {
      auto t = timerq_[i].First();

      if(t != nullptr)
      {
         auto psm = reinterpret_cast< const Base* >(t->Psm());
         stream << lead << strIndex(i) << strObj(psm) << CRLF;
         return;
      }
   }

   if(options.test(DispVerbose)) stream << lead << "No timers." << CRLF;
}

//------------------------------------------------------------------------------

void TimerRegistry::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name TimerRegistry_ProcessWork = "TimerRegistry.ProcessWork";

void TimerRegistry::ProcessWork()
{
   Debug::ft(TimerRegistry_ProcessWork);

   //  Service the next timer queue.
   //
   auto tq = &timerq_[nextQid_];

   for(auto t = tq->First(); t != nullptr; t = tq->First())
   {
      SendTimeout(t);
   }

   //  Advance to the next queue.
   //
   if(++nextQid_ >= Timer::MaxQId) nextQid_ = 0;

   //  Service the queue of long timers.
   //
   tq = &timerq_[Timer::MaxQId];

   for(Timer* curr = tq->First(), *next; curr != nullptr; curr = next)
   {
      next = tq->Next(*curr);
      if(--curr->remaining_ <= 0) SendTimeout(curr);
   }
}

//------------------------------------------------------------------------------

fn_name TimerRegistry_SendTimeout = "TimerRegistry.SendTimeout";

void TimerRegistry::SendTimeout(Timer* tmr)
{
   Debug::ft(TimerRegistry_SendTimeout);

   //  If this timer was the last one encountered, it must have trapped
   //  when sending its timeout, so just delete it.
   //
   if(currTimer_ == tmr)
   {
      delete tmr;
   }
   else
   {
      currTimer_ = tmr;
      tmr->SendTimeout();
      Singleton< TimerPool >::Instance()->IncrTimeouts();
   }

   currTimer_ = nullptr;
}
}
