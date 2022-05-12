//==============================================================================
//
//  LeakyBucketCounter.h
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
#ifndef LEAKYBUCKETCOUNTER_H_INCLUDED
#define LEAKYBUCKETCOUNTER_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include <cstdint>
#include "Duration.h"
#include "SteadyTime.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A leaky bucket counter overflows when more than N events occur within S
//  seconds.  An application uses it to determine if an event has occurred
//  more often than allowed.  The event is usually a fault whose occasional
//  occurrence is acceptable but which points to a more serious problem if
//  it occurs with sufficient frequency.
//
class LeakyBucketCounter : public Object
{
public:
   //  Public so that instances can be declared as members.
   //
   LeakyBucketCounter();

   //  Virtual to allow subclassing.
   //
   virtual ~LeakyBucketCounter();

   //  Initializes the counter to detect LIMIT events in SECONDS.
   //
   void Initialize(size_t limit, uint32_t seconds);

   //  Invoked by the application when an event occurs.  Returns
   //  true if more than LIMIT events have occurred in SECONDS.
   //
   bool HasReachedLimit();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  The length of the interval during which a threshold number
   //  of events have to occur to drain the bucket.
   //
   msecs_t interval_;

   //  The last time that HasReachedLimit was invoked (i.e. the
   //  last time than an event occurred).
   //
   SteadyTime::Point lastTime_;

   //  The maximum number of events allowed during an interval.
   //
   size_t limit_;

   //  Initialized to 0, incremented by an event, and decremented
   //  at a constant rate (but never dropping below zero).
   //
   size_t count_;
};
}
#endif

