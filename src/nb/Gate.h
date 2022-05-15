//==============================================================================
//
//  Gate.h
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
#ifndef GATE_H_INCLUDED
#define GATE_H_INCLUDED

#include <atomic>
#include <condition_variable>
#include <mutex>
#include "Duration.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Wraps a condition variable that is used by a specific thread so that
//  o if spurious unblocking occurs, the thread will wait again;
//  o if the condition is signalled while the thread is not waiting, the
//    thread will not block the next time it waits.
//  This is a platform-independent implementation of Window's CreateEvent
//  (constructor), SetEvent (Notify), and WaitForSingleObject (WaitFor).
//
class Gate
{
public:
   //  Not subclassed.
   //
   Gate() = default;

   //  Not subclassed.
   //
   ~Gate() = default;

   //  Signals the condition.
   //
   void Notify();

   //  Waits on the condition until TIMEOUT.  If TIMEOUT is TIMEOUT_NEVER,
   //  the thread will only unblock when the condition has been signalled.
   //
   std::cv_status WaitFor(const msecs_t& timeout);
private:
   //  The condition.
   //
   std::condition_variable gate_;

   //  A mutex for the condition.
   //
   std::mutex mutex_;

   //  Set when the condition is signalled.
   //
   std::atomic_bool flag_;
};
}
#endif
