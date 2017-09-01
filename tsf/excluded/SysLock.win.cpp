//==============================================================================
//
//  SysLock.win.cpp
//
//  Copyright (C) 2012-2015 Greg Utas.  All rights reserved.
//
#include "SysLock.h"
#include <string>
#include <Windows.h>
#include "Debug.h"
#include "SysThread.h"
#include "ThreadAdmin.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const string SysLock_Acquire = "SysLock.Acquire";

SysLock::Rc SysLock::Acquire(SysLock_t& lock, msecs_t timeout)
{
   auto msecs = (timeout == NeverTimeout ? INFINITE: timeout);

   auto rc = WaitForSingleObject(lock, msecs);

   //  Debug::ft is deferred until now because Windows threads sometimes run
   //  *before* their Thread object has been fully constructed.  This causes
   //  a trap when this function is invoked from Thread::EnterThread.
   //
   Debug::ft(&SysLock_Acquire);

   switch(rc)
   {
   case WAIT_OBJECT_0:
      //
      //  Success.
      //
      return Acquired;
   case WAIT_ABANDONED:
      //
      //  The thread holding the lock failed to release it before exiting.
      //
      ThreadAdmin::Incr(ThreadAdmin::Unreleased);
      return Recovered;
   case WAIT_TIMEOUT:
      //
      //  The timeout interval expired before the lock could be acquired.
      //
      return TimedOut;
   default:
      Debug::SwErr
         (&SysLock_Acquire, GetLastError(), SysThread::RunningThreadId());
      return Failed;
   }
}

//------------------------------------------------------------------------------

const string SysLock_Create = "SysLock.Create";

SysLock_t SysLock::Create()
{
   Debug::ft(&SysLock_Create);

   auto mutex = CreateMutex(nullptr, false, nullptr);
   Debug::Assert(mutex != nullptr);
   return mutex;
}

//------------------------------------------------------------------------------

const string SysLock_Destroy = "SysLock.Destroy";

void SysLock::Destroy(SysLock_t& lock)
{
   Debug::ft(&SysLock_Destroy);

   if(lock != nullptr)
   {
      if(CloseHandle(lock))
         lock = nullptr;
      else
         Debug::SwErr(&SysLock_Destroy, GetLastError(), 0);
   }
}

//------------------------------------------------------------------------------

const string SysLock_Release = "SysLock.Release";

bool SysLock::Release(SysLock_t& lock)
{
   Debug::ft(&SysLock_Release);

   if(ReleaseMutex(lock)) return true;

   Debug::SwErr(&SysLock_Release, GetLastError(), SysThread::RunningThreadId());
   return false;
}
}