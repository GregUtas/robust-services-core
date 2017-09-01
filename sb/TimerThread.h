//==============================================================================
//
//  TimerThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TIMERTHREAD_H_INCLUDED
#define TIMERTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Services the timer queues (in TimerRegistry) by sending timeout messages
//  to timers that have expired.
//
class TimerThread : public Thread
{
   friend class Singleton< TimerThread >;
public:
   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   TimerThread();

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Private because this singleton is not subclassed.
   //
   ~TimerThread();

   //  Overridden to support excluding or including all timer threads.
   //
   virtual TraceStatus CalcStatus(bool dynamic) const override;

   //  Overridden to enter a loop that tells the timer registry, once per
   //  second, to send timeout messages on behalf of expired timers.
   //
   virtual void Enter() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;
};
}
#endif
