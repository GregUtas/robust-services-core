//==============================================================================
//
//  TimerRegistry.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef TIMERREGISTRY_H_INCLUDED
#define TIMERREGISTRY_H_INCLUDED

#include "Dynamic.h"
#include <cstdint>
#include "NbTypes.h"
#include "Q2Way.h"
#include "SbTypes.h"
#include "Timer.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for timers.
//
class TimerRegistry : public NodeBase::Dynamic
{
   friend class NodeBase::Singleton<TimerRegistry>;
   friend class Timer;
public:
   //  Deleted to prohibit copying.
   //
   TimerRegistry(const TimerRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   TimerRegistry& operator=(const TimerRegistry& that) = delete;

   //  Invokes SendTimeout on each timer that has expired.
   //
   void ProcessWork();

   //  Overridden to traverse all timer queues in the registry.
   //
   void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   TimerRegistry();

   //  Private because this is a singleton.
   //
   ~TimerRegistry();

   //  Determines the queue where a timer that will expire in SECS
   //  should be placed.
   //
   Timer::QId CalcQId(uint32_t secs) const;

   //  Sends a timeout on behalf of TMR.
   //
   void SendTimeout(Timer* tmr);

   //  timerq_[s] contains timers expiring in (s - nextQid_) seconds; the
   //  last queue is for timers of Timer::MaxQId seconds or more.
   //
   NodeBase::Q2Way<Timer> timerq_[Timer::MaxQId + 1];

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
