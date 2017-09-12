//=============================================================================
//
//  ThreadObserver.h
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#ifndef THREADOBSERVER_H_INCLUDED
#define THREADOBSERVER_H_INCLUDED

#include "SysDefs.h"
#include "Object.h"
#include "Queues.h"
#include "ProtectedObject.h"
#include "Thread.h"

//-----------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for observers of thread events.  Each subclass should be a
//  singleton.
//
//  Removed for lack of use cases.  To restore, create in NbModule and
//  reinstate the following immediately before invoking Thread::Enter:
//
//    if(faction_ < System)
//    {
//       auto reg = Singleton < ThreadObserverRegistry >::Instance();
//       auto evt = ThreadObserver::Entered;
//       if(stats_->traps > 0) evt = ThreadObserver::Reentered;
//       reg->Notify(evt, Tid());
//    }
//
class ThreadObserver : public ProtectedObject
{
public:
   //  Observable thread events.
   //
   enum Event
   {
      Entered,    // thread entered after initialization
      Reentered,  // thread reentered after trap recovery
      AllEntered  // all threads entered after initialization
   };

   //  Deregisters the observer (ThreadObserverRegistry::Deregister).
   //  Virtual to allow subclassing.
   //
   virtual ~ThreadObserver(void);

   //  Invoked as a callback when EVENT has occurred.  TID identifies the
   //  thread associated with EVENT, if any.  The default version does
   //  nothing and must be overridden.
   //
   virtual void EventOccurred(Event evt, Thread::Id tid) const = 0;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkOffset(void);

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void *arguments) override;
protected:
   //  Registers the observer (ThreadObserverRegistry::Register).
   //  Protected because this class is virtual.
   //
   ThreadObserver(void);
private:
   //  The next observer in ThreadObserverRegistry.
   //
   Q1Link link_;
};

};
#endif
