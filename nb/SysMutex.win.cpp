//==============================================================================
//
//  SysMutex.win.cpp
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
#include "SysMutex.h"
#include <new>
#include <windows.h>
#include "Algorithms.h"
#include "Debug.h"
#include "Duration.h"
#include "MutexRegistry.h"
#include "Singleton.h"
#include "SysThread.h"
#include "SysTypes.h"
#include "Thread.h"
#include "ThreadAdmin.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
SysMutex::SysMutex(const char* name) :
   name_(name),
   mutex_(nullptr),
   nid_(NIL_ID),
   owner_(nullptr),
   locks_(0)
{
   Debug::ft("SysMutex.ctor");

   mutex_ = CreateMutex(nullptr, false, nullptr);
   Debug::Assert(mutex_ != nullptr);

   Singleton< MutexRegistry >::Instance()->BindMutex(*this);
}

//------------------------------------------------------------------------------

fn_name SysMutex_dtor = "SysMutex.dtor";

SysMutex::~SysMutex()
{
   Debug::ftnt(SysMutex_dtor);

   if(nid_ != NIL_ID)
   {
      Debug::SwLog(SysMutex_dtor, name_, nid_);
   }

   Singleton< MutexRegistry >::Extant()->UnbindMutex(*this);

   if(mutex_ != nullptr)
   {
      if(CloseHandle(mutex_))
         mutex_ = nullptr;
      else
         Debug::SwLog(SysMutex_dtor, name_, GetLastError());
   }
}

//------------------------------------------------------------------------------

fn_name SysMutex_Acquire = "SysMutex.Acquire";

SysMutex::Rc SysMutex::Acquire(const Duration& timeout)
{
   Debug::ftnt(SysMutex_Acquire);

   auto curr = SysThread::RunningThreadId();

   if(nid_ == curr)
   {
      ++locks_;
      return Acquired;
   }

   auto thr = Thread::RunningThread(std::nothrow);
   auto result = Error;
   if(thr != nullptr) thr->UpdateMutex(this);
   auto rc = WaitForSingleObject(mutex_, timeout.ToMsecs());
   if(thr != nullptr) thr->UpdateMutex(nullptr);

   switch(rc)
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
      nid_ = curr;
      owner_ = thr;
      if(thr != nullptr) thr->UpdateMutexCount(true);
      locks_ = 1;
      result = Acquired;
      break;
   case WAIT_TIMEOUT:
      //
      //  The timeout interval expired before the lock could be acquired.
      //
      result = TimedOut;
      break;
   default:
      Debug::SwLog(SysMutex_Acquire, name_, GetLastError());
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name SysMutex_Release = "SysMutex.Release";

void SysMutex::Release(bool abandon)
{
   Debug::ftnt(SysMutex_Release);

   auto curr = SysThread::RunningThreadId();

   if(nid_ != curr)
   {
      Debug::SwLog(SysMutex_Release, name_, pack2(curr, nid_));
      return;
   }

   if(!abandon && (--locks_ > 0)) return;

   //  Clear owner_ and nid_ first, in case releasing the mutex results in
   //  another thread acquiring the mutex, running immediately, and setting
   //  those fields to their new values.
   //
   if(owner_ != nullptr) owner_->UpdateMutexCount(false);
   owner_ = nullptr;
   nid_ = NIL_ID;

   if(!ReleaseMutex(mutex_))
   {
      Debug::SwLog(SysMutex_Release, name_, GetLastError());
   }
}
}
#endif
