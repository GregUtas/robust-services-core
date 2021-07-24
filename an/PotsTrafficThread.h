//==============================================================================
//
//  PotsTrafficThread.h
//
//  Copyright (C) 2013-2021  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef POTSTRAFFICTHREAD_H_INCLUDED
#define POTSTRAFFICTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "BcAddress.h"
#include "Duration.h"
#include "NbTypes.h"
#include "SysTypes.h"

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
   static const uint32_t MaxCallsPerMin;

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
   void SetRate(uint32_t rate);

   //  Returns the number of calls to be generated per minute.
   //
   uint32_t GetRate() const { return callsPerMin_; }

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

   //  Records the length of TIME that a POTS line was active on a call.
   //
   void RecordHoldingTime(const Duration& time);

   //  Records an aborted call.
   //
   void RecordAbort() { ++aborts_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this is a singleton.
   //
   PotsTrafficThread();

   //  Private because this is a singleton.
   //
   ~PotsTrafficThread();

   //  Creates new calls and progresses existing calls.
   //
   void SendMessages();

   //  Invoked when a call has progressed to its next state and wants to
   //  delay for DELAY.  If DELAY is 0, the call is deleted, else it is
   //  queued on the timeslot that will be reached in DELAY.
   //
   void Enqueue(TrafficCall& call, msecs_t delay);

   //  Releases the resources that were allocated to run traffic.
   //
   void Takedown();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to send messages to calls.
   //
   void Enter() override;

   //  Overridden to survive warm restarts.
   //
   bool ExitOnRestart(RestartLevel level) const override;

   //  Overridden to essentially run until we have no work remaining.
   //  This ensures that the system will get overloaded rather than
   //  limiting the amount of traffic generated by this thread.
   //
   Duration InitialTime() const override;

   //  How long the thread sleeps before waking up to perform more work.
   //
   Duration timeout_;

   //  The number of calls to generate per minute.
   //
   uint32_t callsPerMin_;

   //  The maximum number of calls to generate during each tick.  It
   //  is set to twice the target rate.
   //
   uint32_t maxCallsPerTick_;

   //  The fractional number of calls (in thousandths) to generate
   //  during each tick.
   //
   uint32_t milCallsPerTick_;

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

   //  The number of times a call was aborted because the traffic thread
   //  did not have enough time to do its work, resulting in a timeout
   //  in the POTS call.
   //
   word aborts_;

   //  Each active call is queued against the timeslot in which it will
   //  decide what to do next (typically, to send a message).
   //
   Q1Way< TrafficCall >* timewheel_;
};
}
#endif
