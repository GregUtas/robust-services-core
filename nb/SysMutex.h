//==============================================================================
//
//  SysMutex.h
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
#ifndef SYSMUTEX_H_INCLUDED
#define SYSMUTEX_H_INCLUDED

#include "Permanent.h"
#include <atomic>
#include <cstddef>
#include "Clock.h"
#include "RegCell.h"
#include "SysDecls.h"

namespace NodeBase
{
   class Thread;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: recursive mutex.
//
//  1. Whenever possible, declare a mutex at file scope in a .cpp.
//     A mutex--especially when locked--should not be deleted.  The risk of
//     this increases when a mutex is allocated in memory that can be freed,
//     even during a restart.  Deleting a locked mutex produces a very bizarre
//     function traceback from Debug.SwLog.
//  2. Use MutexGuard whenever possible.
//     This is a stack variable that, when it goes out of scope, automatically
//     releases its mutex.  This means that you may not need to write any code
//     to explicitly release it.  The mutex also gets released after a trap.
//     The only limitation of MutexGuard is that it assumes that you're willing
//     to block until the mutex becomes available.
//  3. Do not perform a blocking operation while holding a mutex.
//     EnterBlockingOperation generates a log and releases all of the thread's
//     mutexes if this occurs.  The reason is that if the locked thread blocks
//     while holding a mutex, no other locked thread can run until the blocking
//     operation ends and the locked thread releases the mutex.  (If the locked
//     thread blocks on a *mutex*, no other locked thread can run either, but a
//     preemptable or high priority thread should be holding the mutex, and *it*
//     should be able to run and release it.)
//
class SysMutex : public Permanent
{
public:
   //  Outcomes when trying to acquire a mutex.
   //
   enum Rc
   {
      Acquired,  // success
      TimedOut,  // failed to acquire mutex within desired interval
      Error      // error (e.g. mutex does not exist)
   };

   //  Creates a mutex identified by NAME.  Not subclassed.
   //
   explicit SysMutex(const char* name);

   //  Deletes the mutex.
   //
   ~SysMutex();

   //  Deleted to prohibit copying.
   //
   SysMutex(const SysMutex& that) = delete;
   SysMutex& operator=(const SysMutex& that) = delete;

   //  Acquires the mutex.  TIMEOUT specifies how long to wait.
   //
   Rc Acquire(msecs_t timeout);

   //  Releases the mutex.  If LOG is set, a log is generated if the
   //  mutex was not released (because this thread didn't own it).
   //
   void Release(bool log = true);

   //  Returns the native identifier of the thread that owns the mutex.
   //
   SysThreadId OwnerId() const { return nid_; }

   //  Returns the thread, if any, that currently owns the mutex.
   //
   Thread* Owner() const;

   //  Returns the mutex's name.
   //
   const char* Name() const { return name_; }

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
   const char* const name_;

   //  The mutex's index in MutexRegistry.
   //
   RegCell mid_;

   //  A handle to the native mutex.
   //
   SysMutex_t mutex_;

   //  The native identifier of the thread that owns the mutex.
   //
   SysThreadId nid_;

   //  The thread that owns the mutex, if provided.
   //
   Thread* owner_;

   //  The number of times the mutex was acquired.
   //
   std::atomic_size_t locks_;
};
}
#endif
