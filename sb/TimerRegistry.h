//==============================================================================
//
//  TimerRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TIMERREGISTRY_H_INCLUDED
#define TIMERREGISTRY_H_INCLUDED

#include "Dynamic.h"
#include "Clock.h"
#include "NbTypes.h"
#include "Q2Way.h"
#include "Timer.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for timers.
//
class TimerRegistry : public Dynamic
{
   friend class Singleton< TimerRegistry >;
   friend class Timer;
public:
   //  Invokes SendTimeout on each timer that has expired.
   //
   void ProcessWork();

   //  Overridden to traverse all timer queues in the registry.
   //
   virtual void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   TimerRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~TimerRegistry();

   //  Determines the queue where a timer that will expire in SECS
   //  should be placed.
   //
   Timer::QId CalcQId(secs_t secs) const;

   //  Sends a timeout on behalf of TMR.
   //
   void SendTimeout(Timer* tmr);

   //  Overridden to prohibit copying.
   //
   TimerRegistry(const TimerRegistry& that);
   void operator=(const TimerRegistry& that);

   //  timerq_[s] contains timers expiring in (s - nextQid_) seconds; the
   //  last queue is for timers of Timer::MaxQId seconds or more.
   //
   Q2Way< Timer > timerq_[Timer::MaxQId + 1];

   //  The timer queue that will be serviced next.
   //
   Timer::QId nextQid_;

   //  The timer currently being processed.  If this timer is encountered
   //  again, it must have previously caused a trap, so it is deleted.
   //
   const Timer* currTimer_;

   //  Used while the audit traverses the timer queues.
   //
   bool corrupt_;
};
}
#endif
