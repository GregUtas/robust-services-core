//==============================================================================
//
//  DeferredRegistry.h
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
#ifndef DEFERREDREGISTRY_H_INCLUDED
#define TIMERREGISTRY_H_INCLUDED

#include "Dynamic.h"
#include "Deferred.h"
#include "NbTypes.h"
#include "Q2Way.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for deferred work items.
//
class DeferredRegistry : public Dynamic
{
   friend class Singleton< DeferredRegistry >;
   friend class Deferred;
   friend class DeferredThread;
public:
   //  Deleted to prohibit copying.
   //
   DeferredRegistry(const DeferredRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   DeferredRegistry& operator=(const DeferredRegistry& that) = delete;

   //  Deletes all work items owned by OWNER.
   //
   void EraseAll(Base* owner);

   //  Notifies all work items owned by OWNER of EVENT.
   //
   void NotifyAll(const Base* owner, Deferred::Event event);

   //  Overridden to traverse all queues in the registry.
   //
   void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   DeferredRegistry();

   //  Private because this is a singleton.
   //
   ~DeferredRegistry();

   //  Adds a work item to the queue.
   //
   void Insert(Deferred* item);

   //  Deletes a work item.
   //
   void Erase(Deferred* item);

   //  Removes a work item from the queue.
   //
   void Exqueue(Deferred* item);

   //  Raises an event for ITEM.
   //
   void RaiseEvent(Deferred* item, Deferred::Event event);

   //  Invokes RaiseEvent on each item whose timer has expired.
   //
   void RaiseTimeouts();

   //  The queue of work items.
   //
   Q2Way< Deferred > itemq_;

   //  Used while traversing the queue.
   //
   bool corrupt_;
};
}
#endif
