//==============================================================================
//
//  SysLock.h
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
#ifndef SYSLOCK_H_INCLUDED
#define SYSLOCK_H_INCLUDED

#include <iosfwd>
#include <mutex>
#include <string>
#include "SysDecls.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: lightweight mutex.
//
//  This mutex is similar to SysMutex but is specifically intended for any
//  scenario in which a mutex must frequently be acquired.  It is *not*
//  intended for general use: it neither invokes Debug::ft nor registers with
//  MutexRegistry.  It is strongly recommended that SysMutex be used first,
//  converting to this mutex only after thorough testing if the performance
//  improvement justifies it.  In the same way that there is MutexGuard for
//  SysMutex, there is LockGuard for SysLock (see below).
//
class SysLock
{
public:
   //  Creates the mutex.
   //
   SysLock();

   //  Deletes the mutex.
   //
   ~SysLock();

   //  Deleted to prohibit copying.
   //
   SysLock(const SysLock& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   SysLock& operator=(const SysLock& that) = delete;

   //  Acquires the mutex with an infinite timeout.  Returns immediately
   //  if the thread already owns the mutex.
   //
   void Acquire();

   //  Releases the mutex.
   //
   void Release();

   //  Returns the native identifier of the thread that owns the mutex.
   //
   SysThreadId Owner() const { return owner_; }

   //  Displays member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const;
private:
   //  A handle to the native mutex.
   //
   std::mutex mutex_;

   //  The native identifier of the thread that owns the mutex.
   //
   SysThreadId owner_;
};

//------------------------------------------------------------------------------
//
//  Automatically releases a lock when it goes out of scope.
//
class LockGuard
{
public:
   //  Acquires LOCK.  If LOCK is nullptr, all actions equate to a noop.
   //
   explicit LockGuard(SysLock* lock);

   //  Releases the lock.
   //
   ~LockGuard();

   //  Deleted to prohibit copying.
   //
   LockGuard(const LockGuard& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   LockGuard& operator=(const LockGuard& that) = delete;

   //  Releases the lock.  Used to release it before the guard goes
   //  out of scope.
   //
   void Release();
private:
   //  The lock.
   //
   SysLock* lock_;
};
}
#endif
