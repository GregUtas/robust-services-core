//==============================================================================
//
//  Trigger.h
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
#ifndef TRIGGER_H_INCLUDED
#define TRIGGER_H_INCLUDED

#include "Immutable.h"
#include "Q1Way.h"
#include "SbTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   template<class T> class Registry;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A Trigger allows an Initiator to observe a service's behavior and create a
//  ServiceSM associated with the Initiator to modify that service's behavior.
//  It is subclassed by a modifiable service.  Subclasses must be singletons.
//
//  A modifiable service may define a TriggerId without defining a concrete
//  Trigger subclass which registers against that identifier.  This may, in
//  fact, be a frequent occurrence.  Its purpose is to provide a TriggerId
//  to modifiers that have already been triggered, through their ProcessSap
//  and ProcessSnp functions.  These functions receive an event that supports
//  a TriggerId.  If a TriggerId is not provided, a modifier must analyze
//  the current state and event (for an SAP) and also the next state (for an
//  SNP) to determine what is occurring in its parent's state machine.  Such
//  analysis is eliminated by providing a TriggerId.  A Trigger only needs
//  to be registered against this TriggerId, however, if an Initiator needs
//  to *create* a modifier when the SAP or SNP occurs.
//
class Trigger : public NodeBase::Immutable
{
   friend class NodeBase::Registry<Trigger>;
   friend class Initiator;
   friend class ServiceSM;
public:
   //  Allows "Id" to refer to a trigger identifier in this class hierarchy.
   //
   typedef TriggerId Id;

   //  Deleted to prohibit copying.
   //
   Trigger(const Trigger& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Trigger& operator=(const Trigger& that) = delete;

   //> Highest valid trigger identifier.
   //
   static const Id MaxId = 63;

   //  Returns true if TID is a valid trigger identifier.
   //
   static bool IsValidId(Id tid)
   {
      return ((tid != NodeBase::NIL_ID) && (tid <= MaxId));
   }

   //  Returns the trigger's identifier.
   //
   Id Tid() const { return tid_; }

   //  Returns the initiators registered with the trigger.
   //
   const NodeBase::Q1Way<Initiator>& Initiators() const { return initq_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display each trigger.
   //
   size_t Summarize(std::ostream& stream, uint32_t selector) const override;
protected:
   //  Sets tid_.  Protected because this class is virtual.
   //
   explicit Trigger(Id tid);

   //  Deletes all initiators.  Protected because subclasses should
   //  be singletons.
   //
   virtual ~Trigger();
private:
   //  Adds INIT to the trigger's queue of initiators.  Invoked by
   //  Initiator's base class constructor.  Private to restrict usage.
   //
   bool BindInitiator(Initiator& init);

   //  Removes INIT from the trigger's queue of initiators.  Invoked
   //  by Initiator's base class destructor.  Private to restrict usage.
   //
   void UnbindInitiator(Initiator& init);

   //  The identifier for this trigger.
   //
   const Id tid_;

   //  The queue of initiators registered with this trigger.
   //
   NodeBase::Q1Way<Initiator> initq_;
};
}
#endif
