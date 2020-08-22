//==============================================================================
//
//  MutexGuard.h
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
#ifndef MUTEXGUARD_H_INCLUDED
#define MUTEXGUARD_H_INCLUDED

namespace NodeBase
{
   class SysMutex;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Automatically releases a mutex when it goes out of scope.
//
class MutexGuard
{
public:
   //  Acquires MUTEX using TIMEOUT_NEVER.  If MUTEX is nullptr,
   //  all actions equate to a noop.
   //
   explicit MutexGuard(SysMutex* mutex);

   //  Releases the mutex.
   //
   ~MutexGuard();

   //  Releases the mutex.  Used to release it before the guard
   //  goes out of scope.
   //
   void Release();

   //  Deleted to prohibit copying.
   //
   MutexGuard(const MutexGuard& that) = delete;
   MutexGuard& operator=(const MutexGuard& that) = delete;
private:
   //  The mutex.
   //
   SysMutex* mutex_;
};
}
#endif