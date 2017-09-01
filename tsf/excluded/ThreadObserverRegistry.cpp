//=============================================================================
//
//  ThreadObserverRegistry.cpp
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#include "ThreadObserverRegistry.h"
#include "SysDefs.h"
#include "Singleton.h"
#include "Debug.h"
#include "Formatters.h"
#include "Object.h"
#include "Node.h"
#include "Thread.h"
#include "ThreadRegistry.h"
#include "ThreadObserver.h"

using namespace NodeBase;

//-----------------------------------------------------------------------------

const string ThreadObserverRegistry_ctor = "ThreadObserverRegistry.ctor";

ThreadObserverRegistry::ThreadObserverRegistry(void)
{
   Debug::ft(&ThreadObserverRegistry_ctor);

   observerq_.Initq1(ThreadObserver::LinkOffset());
}

//-----------------------------------------------------------------------------

const string ThreadObserverRegistry_dtor = "ThreadObserverRegistry.dtor";

ThreadObserverRegistry::~ThreadObserverRegistry(void)
{
   Debug::ft(&ThreadObserverRegistry_dtor);
}

//-----------------------------------------------------------------------------

const string ThreadObserverRegistry_BindObserver =
   "ThreadObserverRegistry.BindObserver";

bool ThreadObserverRegistry::BindObserver(ThreadObserver &observer)
{
   Debug::ft(&ThreadObserverRegistry_BindObserver);

   return observerq_.Enq1(observer);
}

//-----------------------------------------------------------------------------

void ThreadObserverRegistry::Display
   (ostream &stream, col_t indent, bool verbose) const
{
   ProtectedObject::Display(stream, indent, verbose);

   stream << spaces(indent) << "observers :" << endl;
   observerq_.Display(stream, indent+2, verbose);
}

//-----------------------------------------------------------------------------

const string ThreadObserverRegistry_Notify = "ThreadObserverRegistry.Notify";

void ThreadObserverRegistry::Notify(ThreadObserver::Event evt, Thread::Id tid)
{
   Debug::ft(&ThreadObserverRegistry_Notify);

   for(auto o = observerq_.First(); o != nullptr; observerq_.Next(o))
   {
      o->EventOccurred(evt, tid);
   }

   //  Generate the all threads entered event if appropriate.
   //
   if(Node::Running()) return;

   if((evt == ThreadObserver::Entered) || (evt == ThreadObserver::Reentered))
   {
      auto reg = Singleton < ThreadRegistry >::Instance();

      if(!reg->AllThreadsEntered()) return;

      Node::SetRunning(true);

      for(auto o = observerq_.First(); o != nullptr; observerq_.Next(o))
      {
         o->EventOccurred(ThreadObserver::AllEntered, Thread::NilId);
      }
   }
}

//-----------------------------------------------------------------------------

void ThreadObserverRegistry::Patch(sel_t selector, void *arguments)
{
   ProtectedObject::Patch(selector, arguments);
}

//-----------------------------------------------------------------------------

const string ThreadObserverRegistry_UnbindObserver =
   "ThreadObserverRegistry.UnbindObserver";

void ThreadObserverRegistry::UnbindObserver(ThreadObserver &observer)
{
   Debug::ft(&ThreadObserverRegistry_UnbindObserver);

   observerq_.Exq1(observer);
}
