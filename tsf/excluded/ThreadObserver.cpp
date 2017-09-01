//=============================================================================
//
//  ThreadObserver.cpp
//
//  Copyright (C) 2013-2014 Greg Utas.  All rights reserved.
//
#include "ThreadObserver.h"
#include "SysDefs.h"
#include "Singleton.h"
#include "Object.h"
#include "Debug.h"
#include "Queues.h"
#include "ThreadObserverRegistry.h"

using namespace NodeBase;

//-----------------------------------------------------------------------------

const string ThreadObserver_ctor = "ThreadObserver.ctor";

ThreadObserver::ThreadObserver(void)
{
   Debug::ft(&ThreadObserver_ctor);

   auto reg = Singleton < ThreadObserverRegistry >::Instance();
   reg->BindObserver(*this);
}

//-----------------------------------------------------------------------------

const string ThreadObserver_dtor = "ThreadObserver.dtor";

ThreadObserver::~ThreadObserver(void) //d
{
   Debug::ft(&ThreadObserver_dtor);

   auto reg = Singleton < ThreadObserverRegistry >::Instance();
   reg->UnbindObserver(*this);
}

//-----------------------------------------------------------------------------

const string ThreadObserver_EventOccurred = "ThreadObserver.EventOccurred";

void ThreadObserver::EventOccurred(Event evt, Thread::Id tid) const
{
   Debug::ft(&ThreadObserver_EventOccurred);

   //  This is a pure virtual function.
   //
   Debug::SwErr(&ThreadObserver_EventOccurred, 0, 0, Debug::Abort);
}

//-----------------------------------------------------------------------------

ptrdiff_t ThreadObserver::LinkOffset(void)
{
   word local;
   auto fake = (ThreadObserver*) &local;
   return (uptr) &fake->link_ - (uptr) fake;
}

//-----------------------------------------------------------------------------

void ThreadObserver::Patch(sel_t selector, void *arguments)
{
   ProtectedObject::Patch(selector, arguments);
}
