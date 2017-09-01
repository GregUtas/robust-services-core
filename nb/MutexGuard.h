//==============================================================================
//
//  MutexGuard.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
private:
   //  Overridden to prohibit copying.
   //
   MutexGuard(const MutexGuard& that);
   void operator=(const MutexGuard& that);

   //  The mutex.
   //
   SysMutex* mutex_;
};
}
#endif