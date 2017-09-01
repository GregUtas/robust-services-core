//==============================================================================
//
//  PotsTrafficThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSTRAFFICTHREAD_H_INCLUDED
#define POTSTRAFFICTHREAD_H_INCLUDED

#include "BcAddress.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "Clock.h"
#include "NbTypes.h"
#include "SysTypes.h"
#include "Thread.h"

using namespace NodeBase;
using namespace CallBase;

namespace PotsBase
{
   class TrafficCall;
}

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Thread for running POTS calls to test the system under load.
//
class PotsTrafficThread : public Thread
{
   friend class Singleton< PotsTrafficThread >;
public:
   //  The maximum call rate that can be supported.  It is based on the
   //  number of DNs that are available (Address::LastDN - StartDN) and
   //  HoldingTimeSecs, as well as wanting about 33% of DNs to be idle
   //  at any given time.
   //
   static const size_t MaxCallsPerMin;

   //  Criteria used when searching for a DN.
   //
   enum DnStatus
   {
      Unassigned,  // no circuit
      Assigned,    // idle or busy
      Idle,        // idle circuit
      Busy         // busy circuit
   };

   //  Sets the number of calls to be generated per minute.
   //
   void SetRate(word rate);

   //  Returns the number of calls to be generated per minute.
   //
   word GetRate() const { return callsPerMin_; }

   //  Displays status information.
   //
   void Query(std::ostream& stream) const;

   //  Returns a DN with the specified STATUS.
   //
   Address::DN FindDn(DnStatus status) const;

   //  Displays the number of traffic calls in each state.
   //
   static void DisplayStateCounts
      (std::ostream& stream, const std::string& prefix);

   //  Records the number of SECS that a POTS line was active on a call.
   //
   void RecordHoldingTime(secs_t secs);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  The frequency at which the thread wakes up to send messages when
   //  generating traffic.
   //
   static const msecs_t MsecsPerTick;

   //  The longest time horizon at which a future event can be scheduled.
   //
   static const secs_t MaxDelaySecs;

   //  The number of entries in the timewheel.  Successive entries are
   //  processed every MsecsPerTick.
   //
   static const size_t NumOfSlots;

   //  The first DN that will be allocated for running traffic.  It is
   //  assumed that all DNs between this one and Address::LastDN can be
   //  allocated.
   //
   static const Address::DN StartDN;

   //  The average call holding time, which can be found using the
   //  >traffic query command.
   //
   static const secs_t HoldingTimeSecs;

   //  The average number of POTS lines involved in 100 calls, which
   //  can be found using the >traffic query command.
   //
   static const size_t DNsPer100Calls;

   //  Private because this singleton is not subclassed.
   //
   PotsTrafficThread();

   //  Private because this singleton is not subclassed.
   //
   ~PotsTrafficThread();

   //  Creates new calls and progresses existing calls.
   //
   void SendMessages();

   //  Invoked when a call has progressed to its next state and wants to
   //  delay for MSECS.  If MSECS is 0, the call is deleted, else it is
   //  queued the timeslot that will be reached in MSECS.
   //
   void Enqueue(TrafficCall& call, msecs_t delay);

   //  Releases the resources that were allocated to run traffic.
   //
   void Takedown();

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to send messages to calls.
   //
   virtual void Enter() override;

   //  Overridden to essentially run until we have no work remaining.
   //
   virtual msecs_t InitialMsecs() const override;

   //  Overridden to survive warm restarts.
   //
   virtual bool ExitOnRestart(RestartLevel level) const override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;

   //  The frequency at which the thread is waking up to perform work.
   //
   msecs_t timeout_;

   //  The number of calls to generate per minute.
   //
   word callsPerMin_;

   //  The maximum number of calls to generate during each tick.  It
   //  is set to twice the target rate.
   //
   word maxCallsPerTick_;

   //  The fractional number of calls (in thousandths) to generate
   //  during each tick.
   //
   word milCallsPerTick_;

   //  The first DN created for running traffic.
   //
   Address::DN firstDN_;

   //  The last DN created for running traffic.
   //
   Address::DN lastDN_;

   //  The timeslot in which work is currently being performed.
   //
   size_t currSlot_;

   //  The total number of calls created.
   //
   word totalCalls_;

   //  The number of active calls.
   //
   word activeCalls_;

   //  The total holding times for all POTS lines.
   //
   word totalTimes_;

   //  The number of holding times that were reported.
   //
   word totalReports_;

   //  The number of times an idle DN could not be found to originate
   //  a call.
   //
   word overflows_;

   //  Each active call is queued against the timeslot in which it will
   //  decide what to do next (typically, to send a messsage).
   //
   Q1Way< TrafficCall >* timewheel_;
};
}
#endif
