//==============================================================================
//
//  SysLock.h
//
//  Copyright (C) 2012-2015 Greg Utas.  All rights reserved.
//
#ifndef SYSLOCK_H_INCLUDED
#define SYSLOCK_H_INCLUDED

#include "Clock.h"
#include "SysDefs.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: synchronization lock.
//
class SysLock
{
public:
   //  Outcomes when trying to acquire a lock.
   //
   enum Rc
   {
      Acquired,   // success
      Recovered,  // success; previous owner trapped before releasing lock
      TimedOut,   // failed to acquire lock within desired interval
      Failed      // error (e.g. lock does not exist)
   };

   //  Allocates a lock.
   //
   static SysLock_t Create();

   //  Deletes a lock.
   //
   static void Destroy(SysLock_t& lock);

   //  Acquires a lock.
   //
   static Rc Acquire(SysLock_t& lock, msecs_t timeout);

   //  Releases a lock.
   //
   static bool Release(SysLock_t& lock);
private:
   //  Private because this class only has static members.
   //
   SysLock();
};
}
#endif
