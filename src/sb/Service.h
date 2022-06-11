//==============================================================================
//
//  Service.h
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
#ifndef SERVICE_H_INCLUDED
#define SERVICE_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <cstdint>
#include "Event.h"
#include "EventHandler.h"
#include "RegCell.h"
#include "Registry.h"
#include "SbTypes.h"
#include "State.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Each SessionBase application provides a singleton subclass, which contains
//  other singletons that define the application's state machine.
//
class Service : public NodeBase::Immutable
{
   friend class NodeBase::Registry<Service>;
   friend class State;
public:
   //  Allows "Id" to refer to a service identifier in this class hierarchy.
   //
   typedef ServiceId Id;

   //  Deleted to prohibit copying.
   //
   Service(const Service& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Service& operator=(const Service& that) = delete;

   //> Highest valid service identifier.
   //
   static const Id MaxId = 511;

   //  Returns the service's identifier.
   //
   Id Sid() const { return Id(sid_.GetId()); }

   //  Allows "PortId" to refer to a port identifier in this class hierarchy.
   //
   typedef ServicePortId PortId;

   //  Port identifiers.
   //
   static const PortId UserPort = 1;     // direction towards user (client)
   static const PortId NetworkPort = 2;  // direction towards network (server)
   static const PortId NextPortId = 3;   // next available port identifier

   //  Returns true if PID is a valid port identifier.
   //
   static bool IsValidPortId(PortId pid)
   {
      return ((pid != NodeBase::NIL_ID) && (pid <= MaxServicePortId));
   }

   //  Returns a symbolic name for the port identified by PID.  Should be
   //  overridden by a subclass that defines an application-specific port.
   //
   virtual NodeBase::c_string PortName(PortId pid) const;

   //  Returns the name of the event associated with EID.
   //
   NodeBase::c_string EventName(EventId eid) const;

   //  Returns the total number of events known to the service.
   //
   size_t EventCount() const;

   //  Returns the trigger registered against TID.
   //
   Trigger* GetTrigger(TriggerId tid) const;

   //  Invoked by SessionBase to create a modifier's service state machine
   //  (SSM) during service initiation.  Must be overridden by a modifier
   //  to create an instance of its SSM:
   //
   //    ServiceSM* MySSM::AllocModifier() { return new MySSM; }
   //
   virtual ServiceSM* AllocModifier() const;

   //  Returns true if the service supports modifiers.
   //
   bool IsModifiable() const { return modifiable_; }

   //  Returns true if the service is a modifier.
   //
   bool IsModifier() const { return modifier_; }

   //  Takes a modifier out of service.  Instances of the modifier's state
   //  machine continue to run, but no new instances are created.  Calls to
   //  the modifier's initiators are suppressed.
   //
   bool Disable();

   //  Puts a modifier back into service.
   //
   bool Enable();

   //  The status of a service.
   //
   enum Status
   {
      NotRegistered,  // not in ServiceRegistry
      Disabled,       // creation of state machines disabled
      Enabled         // creation of state machines enabled
   };

   //  Returns a service's status.
   //
   Status GetStatus() const { return status_; }

   //  Returns the registry of states.  Used for iteration.
   //
   const NodeBase::Registry<State>& States() const { return states_; }

   //  Returns the registry of event handlers.  Used for iteration.
   //
   const NodeBase::Registry<EventHandler>& Handlers() const
      { return handlers_; }

   //  Returns the registry of triggers.  Used for iteration.
   //
   const NodeBase::Registry<Trigger>& Triggers() const { return triggers_; }

   //  Returns the offset to sid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display, based on INDEX, each state, event, event
   //  handler, or trigger.
   //
   void Summarize(std::ostream& stream, uint8_t index) const override;

   //  Constants for Summarize's INDEX parameter.
   //
   static const uint8_t SummarizeStates = 1;
   static const uint8_t SummarizeEvents = 2;
   static const uint8_t SummarizeHandlers = 3;
   static const uint8_t SummarizeTriggers = 4;
protected:
   //  Sets the corresponding member variables.  Sets the service's status to
   //  enabled, initializes its registries, registers system event handlers
   //  with it, and adds it to ServiceRegistry.  Protected because this class
   //  is virtual.
   //
   explicit Service(Id sid, bool modifiable = false, bool modifier = false);

   //  Deletes all states before removing the service from ServiceRegistry.
   //  Protected because subclasses should be singletons.
   //
   virtual ~Service();

   //  Registers HANDLER in the slot specified by EHID.  An event handler
   //  can be registered with more than one service, so individual service
   //  constructors must invoke this.
   //
   bool BindHandler(EventHandler& handler, EventHandlerId ehid);

   //  Registers NAME as the class name associated with EID.  A service
   //  should register a name for each event that it defines and invoke
   //  this function in its constructor.
   //
   bool BindEventName(NodeBase::c_string name, EventId eid);

   //  Registers TRIGGER with the service.  The service must be modifiable.
   //  A trigger can register with more than one service, so individual
   //  service constructors must invoke this.
   //
   bool BindTrigger(Trigger& trigger);
private:
   //  Adds STATE to the service.
   //
   bool BindState(State& state);

   //  Removes STATE from the service.
   //
   void UnbindState(State& state);

   //  Registers HANDLER against EHID.
   //
   bool BindSystemHandler(EventHandler& handler, EventHandlerId ehid);

   //  The service's identifier.
   //
   NodeBase::RegCell sid_;

   //  Whether the service is registered and, if so, whether it is enabled.
   //
   Status status_;

   //  Registry for the service's states.
   //
   NodeBase::Registry<State> states_;

   //  Registry for the service's event handlers.
   //
   NodeBase::Registry<EventHandler> handlers_;

   //  Registry for the service's event names.
   //
   NodeBase::c_string eventNames_[Event::MaxId + 1];

   //  Registry for the service's triggers (if it is modifiable).
   //
   NodeBase::Registry<Trigger> triggers_;

   //  Set if the service supports modifiers.
   //
   const bool modifiable_;

   //  Set if the service is a modifier.
   //
   const bool modifier_;
};
}
#endif
