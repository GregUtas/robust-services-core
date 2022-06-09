//==============================================================================
//
//  Mutex.h
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
#ifndef MUTEX_H_INCLUDED
#define MUTEX_H_INCLUDED

#include "Permanent.h"
#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include "Duration.h"
#include "RegCell.h"
#include "SysDecls.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Thread;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: recursive, timed mutex.
//
//  The implementation uses C++11's timed_mutex.  Recursion is implemented
//  within this wrapper so that the mutex can be unlocked if a thread traps
//  while having acquired the mutex recursively.  It does not seem possible
//  to do this safely when using a recursive mutex provided by <mutex>.
//
//  DESIGN GUIDELINES
//
//  Threads that run unpreemptably are mutually excluded, so mutexes are only
//  needed to interact with preemptable or high priority threads.  This is the
//  rationale for running threads unpreemptably ("locked") and only pausing
//  between logical units of work.  If the locked thread blocks on a mutex, no
//  other locked thread can run, but a preemptable or high priority thread
//  should be holding the mutex, and it should be able to run and release it.
//
//  1. Whenever possible, declare a mutex at file scope in a .cpp.
//     A mutex--especially when locked--should not be deleted.  The risk of
//     this increases when a mutex is allocated in memory that can be freed.
//     Deleting a locked mutex produces a bizarre function traceback from
//     Debug.SwLog.  And if a mutex is allocated in memory whose heap is
//     freed during a restart, the handle to it will be lost--although it
//     will still exist--unless it is deleted before the heap is freed.
//
//  2. Use MutexGuard whenever possible.
//     This is a stack variable that, when it goes out of scope, automatically
//     releases its mutex.  This means that you may not need to write any code
//     to explicitly release it.  The mutex will even be released if you trap.
//     The only limitation of MutexGuard is that it assumes that you're willing
//     to block until the mutex becomes available.
//
//  3. Do not perform a blocking operation while holding a mutex.
//     EnterBlockingOperation generates a log if this occurs.  A mutex should
//     be held for a short time, to perform an indivisible operation, so it is
//     hard to see how this could legitimately involve a blocking operation.
//
class Mutex : public Permanent
{
public:
   //  Creates a mutex identified by NAME.  Not subclassed.
   //
   explicit Mutex(c_string name);

   //  Deletes the mutex.
   //
   ~Mutex();

   //  Deleted to prohibit copying.
   //
   Mutex(const Mutex& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Mutex& operator=(const Mutex& that) = delete;

   //  Acquires the mutex.  TIMEOUT specifies how long to wait.  Returns
   //  true if the mutex was acquired, and false if the timeout occurred.
   //
   bool Acquire(const msecs_t& timeout);

   //  Releases the mutex.  If ABANDON is set, the mutex is released
   //  (if owned by this thread) no matter how many times it had been
   //  recursively acquired.
   //
   void Release(bool abandon = false);

   //  Returns the native identifier of the thread that owns the mutex.
   //
   SysThreadId OwnerId() const { return nid_; }

   //  Returns the thread, if any, that currently owns the mutex.
   //
   Thread* Owner() const;

   //  Returns the mutex's name.
   //
   const std::string& Name() const { return name_; }

   //  Returns the mutex's index in the global MutexRegistry.
   //
   id_t Mid() const { return mid_.GetId(); }

   //  Returns the offset to mid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  The mutex's name.
   //
   const std::string name_;

   //  The mutex's index in MutexRegistry.
   //
   RegCell mid_;

   //  The mutex.
   //
   std::timed_mutex mutex_;

   //  The native identifier of the thread that owns the mutex.
   //
   SysThreadId nid_;

   //  The thread that owns the mutex, if provided.
   //
   Thread* owner_;

   //  The number of times the mutex was acquired recursively.
   //
   std::atomic_size_t locks_;
};

//------------------------------------------------------------------------------
//
//  Automatically releases a mutex when it goes out of scope.
//
class MutexGuard
{
public:
   //  Acquires MUTEX using TIMEOUT_NEVER.  If MUTEX is nullptr,
   //  all actions equate to a noop.
   //
   explicit MutexGuard(Mutex* mutex);

   //  Releases the mutex.
   //
   ~MutexGuard();

   //  Deleted to prohibit copying.
   //
   MutexGuard(const MutexGuard& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   MutexGuard& operator=(const MutexGuard& that) = delete;

   //  Releases the mutex.  Used to release it before the guard
   //  goes out of scope.
   //
   void Release();
private:
   //  The mutex.
   //
   Mutex* mutex_;
};
}
#endif
