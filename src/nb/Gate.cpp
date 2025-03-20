//==============================================================================
//
//  Gate.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "Gate.h"
#include <chrono>
#include <ratio>
#include "Debug.h"
#include "SteadyTime.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
void Gate::Notify()
{
   Debug::ft("Gate.Notify");

   //  This locks the mutex to prevent the following:
   //  o The thread invokes WaitFor.  The flag is not set, so it enters
   //    the while loop but happens to immediately get scheduled out.
   //  o We set the flag and invoke notify_one.  The thread is not yet
   //    waiting, so nothing happens.
   //  o When the thread resumes execution, it gets blocked until the
   //    next time that Notify is invoked.
   //  Not locking the mutex would improve performance and be low risk:
   //  o If a thread is supposed to run but doesn't, this function is
   //    reinvoked when InitThread reinvokes Thread::Interrupt.
   //  o A thread that sleeps until a work item arrives is notified each
   //    time a work item arrives, so the risk is a situation in which
   //    the next work item doesn't arrive for quite some time, leaving
   //    the previous item queued during that time.
   //
   std::unique_lock<std::mutex> lock(mutex_);
   flag_.store(true);
   gate_.notify_one();
}

//------------------------------------------------------------------------------

std::cv_status Gate::WaitFor(const msecs_t& timeout)
{
   Debug::ft("Gate.WaitFor");

   auto result = std::cv_status::no_timeout;

   //  The wait function must be called with the mutex locked.  Before
   //  blocking the thread, the wait function releases the mutex, which
   //  allows Notify to obtain it and unblock the waiting thread.
   //
   if(timeout == TIMEOUT_NEVER)
   {
      while(!flag_.load())
      {
         std::unique_lock<std::mutex> lock(mutex_);
         gate_.wait(lock);
      }
   }
   else
   {
      //  A thread uses a timeout when it wants to sleep for a finite
      //  length of time, so exit the condition if the timeout occurs.
      //  Spurious wakeups have been observed in DelayCommand, so go
      //  back to sleep unless flag_ has been set.  Using wait_for
      //  allows a thread to be woken *before* the timeout occurs.
      //  This capability is used, for example, to wake up a watchdog
      //  with a heartbeat or to wake up a thread and request that it
      //  immediately exit when a restart is initiated.
      //
      auto deadline = SteadyTime::Now() + timeout;

      while(!flag_.load())
      {
         std::unique_lock<std::mutex> lock(mutex_);
         auto sleep = deadline - SteadyTime::Now();

         if(sleep.count() <= 0)
         {
            result = std::cv_status::timeout;
            break;
         }

         result = gate_.wait_for(lock, sleep);
         if(SteadyTime::Now() > deadline) break;
      }
   }

   flag_.store(false);
   return result;
}
}
