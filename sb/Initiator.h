//==============================================================================
//
//  Initiator.h
//
//  Copyright (C) 2017  Greg Utas
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
#ifndef INITIATOR_H_INCLUDED
#define INITIATOR_H_INCLUDED

#include "Persistent.h"
#include <cstddef>
#include <cstdint>
#include "EventHandler.h"
#include "Q1Link.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  An Initiator requests the creation of a ServiceSM which then modifies the
//  behavior of a root service.  A modifier registers an Initiator with each
//  trigger (usually one) that it uses in order to observe its root service's
//  behavior and create its own ServiceSM at the appropriate time.
//
class Initiator : public NodeBase::Persistent
{
   friend class NodeBase::Q1Way< Initiator >;
public:
   //  Each initiator specifies a priority when it registers with its trigger.
   //  The trigger queues initiators in descending order of priority, meaning
   //  that an initiator with a higher priority will get the chance to request
   //  the creation of its modifier before an initiator with a lower priority.
   //  This is important in resolving service interactions.  All priorities
   //  must be defined in the interface that defines the associated trigger.
   //
   typedef uint8_t Priority;

   //  Deleted to prohibit copying.
   //
   Initiator(const Initiator& that) = delete;
   Initiator& operator=(const Initiator& that) = delete;

   //  Returns the service associated with the initiator.
   //
   ServiceId Sid() const { return sid_; }

   //  Returns the initiator's priority.
   //
   Priority GetPriority() const { return prio_; }

   //  Invokes the initiator's ProcessEvent function.
   //
   EventHandler::Rc InvokeHandler
      (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the corresponding member variables.  Adds the initiator to
   //  the trigger identified by AID and TID, which must already exist.
   //  SID is the initiator's service, and PRIO is its priority with
   //  respect to other services that use the same trigger.  Protected
   //  because this class is virtual.
   //
   Initiator(ServiceId sid, ServiceId aid, TriggerId tid, Priority prio);

   //  Removes the initiator from its trigger.  Protected to restrict
   //  deletion.
   //
   virtual ~Initiator();
private:
   //  The initiator's event handler, which receives either an SAP or SNP,
   //  depending on the trigger with which it has registered.  It can either
   //  pass currEvent onwards or create an InitiationReqEvent and return it in
   //  nextEvent to request the creation of its ServiceSM.  The default version
   //  generates a log and returns EventHandler::Pass and must be overridden.
   //
   virtual EventHandler::Rc ProcessEvent
      (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const;

   //  Returns the trigger where the initiator is located.
   //
   Trigger* GetTrigger() const;

   //  Used by InvokeHandler to clean up when an error is detected
   //  during event processing.
   //
   EventHandler::Rc EventError(Event*& evt, EventHandler::Rc rc) const;

   //  The service associated with the initiator.
   //
   const ServiceId sid_;

   //  The service associated with the trigger (the initiator's ancestor).
   //
   const ServiceId aid_;

   //  The trigger associated with the initiator.
   //
   const TriggerId tid_;

   //  The initiator's priority.
   //
   const Priority prio_;

   //  The next initiator in the trigger's queue of initiators.
   //
   NodeBase::Q1Link link_;
};
}
#endif
