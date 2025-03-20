//==============================================================================
//
//  Deferred.h
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
#ifndef DEFERRED_H_INCLUDED
#define DEFERRED_H_INCLUDED

#include "MsgBuffer.h"
#include <cstddef>
#include <cstdint>
#include "Q2Link.h"

namespace NodeBase
{
   class Thread;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A work item that will be executed when a timeout or an event occurs.
//
class Deferred : public MsgBuffer
{
   friend class Q2Way<Deferred>;
   friend class DeferredRegistry;
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Deferred();

   //  Deleted to prohibit copying.
   //
   Deferred(const Deferred& that) = delete;

   //  An event for a work item.  The only standard event is a timeout.
   //  Other events are defined by owners of work items.
   //
   typedef uint32_t Event;

   static const Event Timeout = 0;

   //  Forwards the item to THREAD.
   //
   void SendToThread(Thread* thread);

   //  Resets the item with a new timeout in SECS.
   //
   void Restart(uint32_t secs);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a deferred work item owned by OWNER that will be notified of a
   //  timeout in SECS, and that will survive a warm restart if WARM is set.
   //  Protected because this class is virtual.
   //
   Deferred(Base& owner, uint32_t secs, bool warm);
private:
   //  Notifies the work item of EVENT.  When this is invoked, the item is
   //  unowned (not queued).  Unless it is reassigned to an owner, usually
   //  by invoking SendToThread or Restart, it is deleted after this function
   //  returns.
   //
   virtual void EventHasOccurred(Event event) = 0;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to remove the item from the work queue.
   //
   void Cleanup() override;

   //  The two-way queue link for the registry.
   //
   Q2Link link_;

   //  The item's owner.
   //
   Base* const owner_;

   //  How long until the timeout occurs.
   //
   uint32_t secs_;

   //  Set if the item should survive a warm restart.
   //
   const bool warm_;
};
}
#endif
