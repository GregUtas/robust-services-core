//=============================================================================
//
//  ThreadObserverRegistry.h
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#ifndef THREADOBSERVERREGISTRY_H_INCLUDED
#define THREADOBSERVERREGISTRY_H_INCLUDED

#include "SysDefs.h"
#include "Object.h"
#include "Singleton.h"
#include "Registry.h"
#include "ProtectedObject.h"
#include "Thread.h"
#include "ThreadObserver.h"

using namespace NodeBase;

//-----------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for thread observers.
//
class ThreadObserverRegistry : public ProtectedObject
{
   friend class Singleton < ThreadObserverRegistry >;
   friend class ThreadObserver;
public:
   //  Notifies all observers that EVENT has occurred for TID.
   //
   void Notify(ThreadObserver::Event evt, Thread::Id tid);

   //  Overridden to display member variables.
   //
   virtual void Display
      (ostream &stream, col_t indent, bool verbose) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void *arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ThreadObserverRegistry(void);

   //  Private because this singleton is not subclassed.
   //
   ~ThreadObserverRegistry(void);

   //  Add OBSERVER to the registry.  Returns false on failure.
   //
   bool BindObserver(ThreadObserver &observer);

   //  Removes OBSERVER from the registry.
   //
   void UnbindObserver(ThreadObserver &observer);

   //  The global registry of thread observers.
   //
   Q1Way < ThreadObserver > observerq_;
};

};
#endif