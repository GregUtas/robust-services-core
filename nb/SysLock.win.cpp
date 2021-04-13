//==============================================================================
//
//  SysLock.win.cpp
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
#ifdef OS_WIN
#include "SysLock.h"
#include <windows.h>
#include "Debug.h"
#include "SysThread.h"
#include "ThreadAdmin.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
SysLock::SysLock() :
   mutex_(nullptr),
   owner_(NIL_ID)
{
   mutex_ = CreateMutex(nullptr, false, nullptr);
   Debug::Assert(mutex_ != nullptr);
}

//------------------------------------------------------------------------------

fn_name SysLock_dtor = "SysLock.dtor";

SysLock::~SysLock()
{
   if(owner_ != NIL_ID)
   {
      Debug::SwLog(SysLock_dtor, "lock has owner", owner_);
   }

   if(mutex_ != nullptr)
   {
      if(CloseHandle(mutex_))
         mutex_ = nullptr;
      else
         Debug::SwLog(SysLock_dtor, "lock not freed", GetLastError());
   }
}

//------------------------------------------------------------------------------

void SysLock::Acquire()
{
   auto curr = SysThread::RunningThreadId();
   if(owner_ == curr) return;

   auto result = WaitForSingleObject(mutex_, INFINITE);
   switch(result)
   {
   case WAIT_ABANDONED:
      //
      //  The thread holding the lock failed to release it before exiting.
      //
      ThreadAdmin::Incr(ThreadAdmin::Unreleased);
      //  [[fallthrough]]
   case WAIT_OBJECT_0:
      //
      //  Success.
      //
      owner_ = SysThread::RunningThreadId();
      return;
   default:
      //
      //  There was no timeout, so this shouldn't occur.
      //
      Debug::Assert(false, result);
   }
}

//------------------------------------------------------------------------------

fn_name SysLock_Release = "SysLock.Release";

void SysLock::Release()
{
   auto curr = SysThread::RunningThreadId();
   if(owner_ != curr) return;

   //  Clear owner_ first, in case releasing the mutex results in another
   //  thread acquiring it and running immediately, in which case it will
   //  set owner_ itself.
   //
   owner_ = NIL_ID;

   if(!ReleaseMutex(mutex_))
   {
      Debug::SwLog(SysLock_Release, "failed to release mutex", GetLastError());
   }
}
}
#endif