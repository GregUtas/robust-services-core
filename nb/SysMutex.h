//==============================================================================
//
//  SysMutex.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SYSMUTEX_H_INCLUDED
#define SYSMUTEX_H_INCLUDED

#include "Permanent.h"
#include "Clock.h"
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

   //  Creates a mutex.  Not subclassed.
   //
   SysMutex();

   //  Deletes the mutex.
   //
   ~SysMutex();

   //  Acquires the mutex.  TIMEOUT specifies how long to wait.
   //
   Rc Acquire(msecs_t timeout, Thread* owner = nullptr);

   //  Releases the mutex.
   //
   void Release();

   //  Returns the native identifier of the thread that owns the mutex.
   //
   SysThreadId OwnerId() const { return nid_; }

   //  Returns the thread, if any, that currently owns the mutex.
   //
   Thread* Owner() const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to prohibit copying.
   //
   SysMutex(const SysMutex& that);
   void operator=(const SysMutex& that);

   //  A handle to the native mutex.
   //
   SysMutex_t mutex_;

   //  The native identifier of the thread that owns the mutex.
   //
   SysThreadId nid_;

   //  The thread that owns the mutex, if provided.
   //
   Thread* owner_;
};
}
#endif
