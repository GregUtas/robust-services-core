//==============================================================================
//
//  Event.h
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
#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include <cstdint>
#include "EventHandler.h"
#include "SbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Each subclass defines an event for a service's state machine.
//
class Event : public NodeBase::Pooled
{
   friend class AnalyzeSapEvent;
   friend class ServiceSM;
   friend class SsmContext;
public:
   //  Allows "Id" to refer to an event identifier in this class hierarchy.
   //
   typedef EventId Id;

   //> Highest valid event identifier.
   //
   static const Id MaxId = INT8_MAX;

   //  Returns true if EID is a valid identifier.
   //
   static bool IsValidId(Id eid)
   {
      return ((eid != NodeBase::NIL_ID) && (eid <= MaxId));
   }

   //  Event identifiers used within the SessionBase framework.
   //
   static const Id AnalyzeMsg      = 1;
   static const Id AnalyzeSap      = 2;
   static const Id AnalyzeSnp      = 3;
   static const Id ForceTransition = 4;
   static const Id InitiationReq   = 5;
   static const Id MediaFailure    = 10;

   //  Returns true if an application can handle EID.
   //
   static bool AppCanHandle(Id eid)
   {
      return ((eid >= MediaFailure) && (eid <= MaxId));
   }

   //  Applications must start to number their events from here.
   //
   static const Id NextId = 11;

   //  Returns the event's identifier.
   //
   Id Eid() const { return eid_; }

   //  The framework deletes an event after a service has processed it, so
   //  a service only needs to delete an event if an event handler raises it
   //  but then decides that it should not be handled after all.  Virtual to
   //  allow subclassing.
   //
   virtual ~Event();

   //  Returns the event's owner (the state machine to which it belongs).
   //
   ServiceSM* Owner() const { return owner_; }

   //  Where an event is currently located.
   //
   enum Location
   {
      Active,     // being processed (on owner's active event queue)
      Pending,    // on owner's pending event queue
      Saved,      // on owner's saved event queue
      Location_N  // number of locations
   };

   //  Saves the event's context when a modifier returns EventHandler::Suspend
   //  o after being created by an Initiator that reacted to an SAP, or
   //  o when already active and reacting to a subsequent SAP.
   //  The modifier invokes this function on its parent's current SAP event.
   //  It must then save a pointer to the SAP so that it can eventually invoke
   //  RestoreContext (for EventHandler::Resume) or FreeContext (when forcing
   //  its parent to take another path).  Returns true on success.
   //
   virtual bool SaveContext();

   //  Restores the event's context when its processing should resume.  The
   //  appropriate return code for resuming the processing of the returned
   //  event is provided; the return event is nullptr on error conditions.
   //  Processing resumes
   //  o with the next Initiator in the trigger's queue, if the modifier
   //    invoked SaveContext when it was initiated, or
   //  o with the next SSM in the SSMQ, if the modifier invoked SaveContext
   //    in reaction to a subsequent SAP.
   //
   virtual Event* RestoreContext(EventHandler::Rc& rc);

   //  Purges the event's context when its processing should not resume.
   //  If freeMsg is true, the saved message is also deleted; otherwise,
   //  it is restored as the context message.
   //
   virtual void FreeContext(bool freeMsg);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to obtain an event from its object pool.
   //
   static void* operator new(size_t size);
protected:
   //  Sets the corresponding member variables.  Protected because this class
   //  is virtual.
   //    OWNER must be valid if the context has a root SSM.  The constructor
   //  for a subclass provides the correct value for EID when it invokes a base
   //  class constructor.  In almost all cases, LOC is Active.  Pending is used,
   //  for example, when a message analyzer creates an InitiationReqEvent to
   //  handle a message that contains a service invocation parameter which is
   //  to be processed after the work associated with the message's signal.
   //
   Event(Id eid, ServiceSM* owner, Location loc = Active);

   //  Moves an active event to its owner's saved event queue.
   //
   virtual bool Save();

   //  Moves a saved event back to its owner's active event queue.
   //
   virtual Event* Restore(EventHandler::Rc& rc);

   //  Frees a saved event.
   //
   virtual void Free();
private:
   //  Invoked on an event to create its SAP.  An event that is not passed
   //  to modifiers as an SAP overrides this function to return nullptr.
   //  If the event is its own SAP, it returns a reference to itself.
   //
   virtual Event* BuildSap(ServiceSM& owner, TriggerId tid);

   //  Invoked on an event to create its SNP.  An event that is not passed
   //  to modifiers as an SNP overrides this function to return nullptr.
   //  If the event is its own SNP, it returns a reference to itself.
   //
   virtual Event* BuildSnp(ServiceSM& owner, TriggerId tid);

   //  Invoked to save the current position in the SSMQ during SAP or SNP
   //  processing.  The default version does nothing and must be overridden
   //  by events that support SaveContext.
   //
   virtual void SetCurrSsm(ServiceSM* ssm);

   //  Invoked to save the current position in a trigger's initiator queue
   //  during SAP or SNP processing.  The default version does nothing and
   //  must be overridden by events that support SaveContext.
   //
   virtual void SetCurrInitiator(const Initiator* init);

   //  Sets the event's location.
   //
   void SetLocation(Location loc);

   //  Returns the event's location.
   //
   Location GetLocation() const { return location_; }

   //  Traces the event after it has been handled in STATE, which belongs
   //  to the service identified by SID.  The event handler returned RC.
   //
   virtual void Capture
      (ServiceId sid, const State& state, EventHandler::Rc rc) const;

   //  Sets the event's owner unless it already exists.  Invoked on the
   //  event raised by a PSM when the root SSM is allocated.
   //
   void SetOwner(RootServiceSM& owner);

   //  The event's identifier.
   //
   const Id eid_;

   //  The state machine that owns the event.
   //
   ServiceSM* owner_;

   //  The event's location.
   //
   Location location_;
};
}
#endif
