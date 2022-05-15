//==============================================================================
//
//  Gate.cpp
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
#include "Gate.h"
#include <chrono>
#include <ratio>
#include "Debug.h"

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
   //    reinvoked when InitThread reinvokes Thread.Interrupt.
   //  o A thread that sleeps until a work item arrives is notified each
   //    time a work item arrives, so the risk is a situation in which
   //    the next work item doesn't arrive for quite some time, leaving
   //    the previous item queued during that time.
   //
   std::unique_lock< std::mutex > lock(mutex_);
   flag_.store(true);
   gate_.notify_one();
}

//------------------------------------------------------------------------------

std::cv_status Gate::WaitFor(const msecs_t& timeout)
{
   Debug::ft("Gate.WaitFor");

   auto result = std::cv_status::no_timeout;

   while(!flag_.load())
   {
      //  The wait function must be called with the mutex locked.  Before
      //  the wait returns, it unlocks the mutex, which allows Notify to
      //  obtain it and unblock the waiting thread.
      //
      std::unique_lock< std::mutex > lock(mutex_);

      if(timeout == TIMEOUT_NEVER)
      {
         gate_.wait(lock);
      }
      else
      {
         //  A timeout is only used when a thread wants to sleep for a
         //  fixed period of time.  When the timeout occurs, we simply
         //  exit the condition, because the thread blocks until it is
         //  told to proceed.  Using WaitFor allows the thread to be
         //  woken *before* the timeout expires.  This capability is
         //  used, for example, to tell threads to exit during restarts
         //  and to notify watchdogs that necessary work is still being
         //  performed.
         //
         result = gate_.wait_for(lock, timeout);
         break;
      }
   }

   flag_.store(false);
   return result;
}
}
