//==============================================================================
//
//  Timer.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include <cstdint>
#include "Q1Link.h"
#include "Q2Link.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  An instance of a timer running on a PSM.  See also TimerProtocol.h.
//
class Timer : public NodeBase::Pooled
{
   friend class NodeBase::Q1Way<Timer>;
   friend class NodeBase::Q2Way<Timer>;
   friend class ProtocolSM;
   friend class TimerRegistry;
   friend class TimerTrace;
public:
   //  Returns the timer's PSM.
   //
   ProtocolSM* Psm() const { return psm_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Starts a timer on PSM, owned by OWNER, identified by TID, that will
   //  expire in SECS, and repeatedly if REPEAT is true.  Private because
   //  applications create timers via ProtocolSM::StartTimer.
   //
   Timer(ProtocolSM& psm, Base& owner, TimerId tid, uint32_t secs, bool repeat);

   //  Private because applications delete timers via ProtocolSM::StopTimer.
   //  Not subclassed.
   //
   ~Timer();

   //  Returns the timer's owner.
   //
   Base* Owner() const { return owner_; }

   //  Returns the timer's identifier.
   //
   TimerId Tid() const { return tid_; }

   //  Removes the timer from its PSM's timer queue.
   //
   void Exqueue();

   //  Removes the timer from the timer registry.
   //
   void Deregister();

   //  Used by the timer thread to send a message to the PSM associated with
   //  the timer when the timer expires.
   //
   void SendTimeout();

   //  Restarts a repetitive timer each time it expires.
   //
   void Restart();

   //  Overridden to obtain a timer from its object pool.
   //
   static void* operator new(size_t size);

   //  Timer queue identifier.
   //
   typedef int QId;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to remove the timer from the timer registry during error
   //  recovery.
   //
   void Cleanup() override;

   //  Nil timer queue identifier, which allows array index 0 to be used.
   //
   static const QId NilQId = -1;

   //  Maximum timer queue identifier (3600 seconds = 1 hour).  The first
   //  3601 slots in the timewheel are for timers expiring in 0 to 3600
   //  seconds, and the final slot is for long duration timers (over 3600
   //  seconds).
   //
   static const QId MaxQId = 3601;

   //  The PSM on which the timer is running.
   //
   ProtocolSM* psm_;

   //  The timer's owner.
   //
   Base* const owner_;

   //  The owner's identifier for the timer.
   //
   TimerId tid_;

   //  Set if the timer should repeatedly expire every duration_ seconds.
   //
   const bool repeat_;

   //  The queue where the timer resides in the timer registry.
   //
   QId qid_;

   //  The two-way queue link for the timer registry.
   //
   NodeBase::Q2Link link_;

   //  The length of the timer in seconds.
   //
   const uint32_t secs_;

   //  How long until the timer expires (only used for long timers).
   //
   uint32_t remaining_;
};
}
#endif
