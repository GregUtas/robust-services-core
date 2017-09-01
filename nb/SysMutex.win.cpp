//==============================================================================
//
//  SysMutex.win.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifdef OS_WIN
#include "SysMutex.h"
#include <windows.h>
#include "Debug.h"
#include "SysThread.h"
#include "SysTypes.h"
#include "ThreadAdmin.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysMutex_ctor = "SysMutex.ctor";

SysMutex::SysMutex() : mutex_(nullptr), nid_(NIL_ID), owner_(nullptr)
{
   Debug::ft(SysMutex_ctor);

   mutex_ = CreateMutex(nullptr, false, nullptr);
   Debug::Assert(mutex_ != nullptr);
}

//------------------------------------------------------------------------------

fn_name SysMutex_dtor = "SysMutex.dtor";

SysMutex::~SysMutex()
{
   Debug::ft(SysMutex_dtor);

   if(nid_ != NIL_ID)
   {
      Debug::SwErr(SysMutex_dtor, debug64_t(mutex_), nid_);
   }

   if(mutex_ != nullptr)
   {
      if(CloseHandle(mutex_))
         mutex_ = nullptr;
      else
         Debug::SwErr(SysMutex_dtor, debug64_t(mutex_), GetLastError());
   }
}

//------------------------------------------------------------------------------

fn_name SysMutex_Acquire = "SysMutex.Acquire";

SysMutex::Rc SysMutex::Acquire(msecs_t timeout, Thread* owner)
{
   auto nid = SysThread::RunningThreadId();
   auto result = Error;
   auto msecs = (timeout == TIMEOUT_NEVER ? INFINITE: timeout);
   auto rc = WaitForSingleObject(mutex_, msecs);

   switch(rc)
   {
   case WAIT_ABANDONED:
      //
      //  The thread holding the lock failed to release it before exiting.
      //
      ThreadAdmin::Incr(ThreadAdmin::Unreleased);
   case WAIT_OBJECT_0:
      //
      //  Success.
      //
      nid_ = nid;
      owner_ = owner;
      result = Acquired;
      break;
   case WAIT_TIMEOUT:
      //
      //  The timeout interval expired before the lock could be acquired.
      //
      result = TimedOut;
      break;
   default:
      Debug::SwErr(SysMutex_Acquire, debug64_t(mutex_), GetLastError());
   }

   //  Debug::ft is deferred because Windows threads sometimes run *before*
   //  their Thread object has been fully constructed.  This causes a trap
   //  when this function is invoked from Thread::EnterThread.
   //
   Debug::ft(SysMutex_Acquire);
   return result;
}

//------------------------------------------------------------------------------

fn_name SysMutex_Release = "SysMutex.Release";

void SysMutex::Release()
{
   Debug::ft(SysMutex_Release);

   auto owner = owner_;
   auto nid = nid_;

   owner_ = nullptr;
   nid_ = NIL_ID;

   if(!ReleaseMutex(mutex_))
   {
      nid_ = nid;
      owner_ = owner;
      Debug::SwErr(SysMutex_Release, debug64_t(mutex_), GetLastError());
   }
}
}
#endif