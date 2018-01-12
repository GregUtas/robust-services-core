//==============================================================================
//
//  ServiceSM.h
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
#ifndef SERVICESM_H_INCLUDED
#define SERVICESM_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include "Event.h"
#include "EventHandler.h"
#include "Q1Way.h"
#include "SbTypes.h"
#include "Trigger.h"

namespace SessionBase
{
   class AnalyzeMsgEvent;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  ServiceSM is the base class for the run-time instance of a SessionBase
//  application.  Each application defines a subclass to support a run-time
//  instance of its state machine.  A modifier of a root service subclasses
//  from ServiceSM, whereas a root service (non-modifier) subclasses from
//  RootServiceSM.
//
class ServiceSM : public Pooled
{
   friend class Event;
   friend class Q1Way< ServiceSM >;
   friend class SbInitiationReq;
   friend class SsmContext;
public:
   //  Initial state for SSMs.  If an SSM is in this state at the end of
   //  a transaction, it is destroyed.  A modifier in this state does not
   //  receive SAPs, SNPs, or SIPs.
   //
   static const StateId Null = 1;

   //  Returns the port (local PSM identifier) associated with the message
   //  to be analyzed.  The PSM on which the message arrived is available
   //  as ame.Msg()->Psm().  Before it performs its own analysis, a modifier
   //  should invoke its parent's CalcPort function,  and a subclass should
   //  invoke its base class CalcPort function.
   //
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) = 0;

   //  Handles an SAP for a modifier SSM.  The default version returns
   //  EventHandler::Pass and must be overridden by a modifier that needs
   //  to observe its parent's behavior after being initiated.
   //
   virtual EventHandler::Rc ProcessSap(Event& currEvent, Event*& nextEvent);

   //  Handles an SNP for a modifier SSM.  The default version returns
   //  EventHandler::Pass and must be overridden by a modifier that needs
   //  to observe its parent's behavior after being initiated.
   //
   virtual EventHandler::Rc ProcessSnp(Event& currEvent, Event*& nextEvent);

   //  Returns the service identifier associated with this SSM.
   //
   ServiceId Sid() const { return sid_; }

   //  Returns the service associated with this SSM.
   //
   Service* GetService() const;

   //  Returns the SSM's current state.
   //
   StateId CurrState() const { return currState_; }

   //  Returns the SSM's next state.
   //
   StateId NextState() const { return nextState_; }

   //  Sets STID as the SSM's next state.  May be overridden for observation
   //  purposes, but the base class version must be invoked.
   //
   virtual void SetNextState(StateId stid);

   //  Informs the SSM that exPsm is being deleted.  The default version
   //  informs modifiers of the PSM deletion.  May be overridden to clear
   //  PSM pointers in member data, for example, but the base class version
   //  must be invoked.
   //
   virtual void PsmDeleted(ProtocolSM& exPsm);

   //  Returns true if the SSM has entered the Null state.
   //
   bool HasIdled() const { return idled_; }

   //  Returns a modifier's parent SSM.  Returns nullptr for a root SSM.
   //
   ServiceSM* Parent() const { return parentSsm_; }

   //  Sets SAP as the SSM's next service alteration point.  May be
   //  overridden for observation purposes, but the base class version
   //  must be invoked.
   //
   virtual void SetNextSap(TriggerId sap);

   //  Sets SNP as the SSM's next service notification point.  May be
   //  overridden for observation purposes, but the base class version
   //  must be invoked.
   //
   virtual void SetNextSnp(TriggerId snp);

   //  Returns true if the trigger identified by TID has completed its
   //  initiator processing.
   //
   bool HasTriggered(TriggerId tid) const;

   //  Sets sid_ to SID.  This has the effect of replacing the SSM's
   //  current states, triggers, and event handlers with those defined
   //  by SID.  If both sets of event handlers can use the same SSM,
   //  the change will be transparent.
   //
   virtual void MorphToService(ServiceId sid);

   //  Overridden to enumerate all objects that the SSM owns.
   //
   virtual void GetSubtended(Base* objects[], size_t& count) const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to obtain an SSM from its object pool.
   //
   static void* operator new(size_t size);
protected:
   //  Modifer SSMs are created by Service::AllocModifier.  Non-modifier SSMs
   //  subclass from RootServiceSM.  Protected because this class is virtual.
   //
   explicit ServiceSM(ServiceId sid);

   //  Invoked by EndOfTransaction if the SSM is in the Null state.  Deletes
   //  any modifiers in the SSMQ and any events in an event queue.  Protected
   //  to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~ServiceSM();

   //  Invoked at the end of each transaction.  Traverses the SSMQ to invoke
   //  this function on each modifier.  Deletes the SSM if it is in the Null
   //  state.  May be overridden to release resources before deletion, but
   //  the base class version must be invoked.
   //
   virtual void EndOfTransaction();
private:
   //  Handles an initiation ack for a modifier SSM that was just created.
   //  The default version kills the context and must be overridden.
   //
   virtual EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) = 0;

   //  Handles an initiation nack for a modifier SSM that was just created.
   //  The default version kills the context and must be overridden if the
   //  modifier's initiation request can be denied.
   //
   virtual EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent);

   //  Handles an SIP for a modifier SSM.  The default version returns
   //  EventHandler::Pass and must be overridden by a modifier that needs
   //  to observe the initiation of a sibling.
   //
   virtual EventHandler::Rc ProcessSip
      (Event& currEvent, Event*& nextEvent);

   //  Steps within the function ProcessEvent.
   //
   enum Phase
   {
      ModifierSapPhase,       // pass the event's SAP down the SSMQ
      ModifierReentryPhase,   // reenter the SSMQ upon a EventHandler::Resume
      InitiatorSapPhase,      // pass the event's SAP down the InitQ
      InitiatorReentryPhase,  // reenter the InitQ upon a EventHandler::Resume
      LocalEventPhase,        // pass the event to the context SSM, and then
                              // pass the event's SNP down the SSMQ and InitQ
      FreeEventPhase          // free the event
   };

   //  Enqueues EVT on the queue associated with LOC.
   //
   void EnqEvent(Event& evt, Event::Location loc);

   //  Exqueues EVT from the queue associated with LOC.
   //
   bool ExqEvent(Event& evt, Event::Location loc);

   //  Coordinates the event handling phases for currEvent.  nextEvent is
   //  the next event to be processed, if any.
   //
   EventHandler::Rc ProcessEvent(Event* currEvent, Event*& nextEvent);

   //  Used during error recovery.
   //
   EventHandler::Rc EventError2(Event*& evt, EventHandler::Rc rc) const;

   //  Sets a modifier's parent.
   //
   void SetParent(ServiceSM& parent);

   //  Adds MODIFIER at the front of the SSMQ.
   //
   void HenqModifier(ServiceSM& modifier);

   //  Routes an SAP down this SSM's SSMQ, starting at MODIFIER.
   //
   EventHandler::Rc ProcessSsmqSap
      (ServiceSM* modifier, Event& sapEvent, Event*& nextEvent, Phase& phase);

   //  Routes an SAP down TRIGGER's Initiator queue, starting at MODIFIER.
   //
   EventHandler::Rc ProcessInitqSap
      (const Trigger* trigger, const Initiator* modifier, Event& sapEvent,
      Event*& nextEvent, Phase& phase);

   //  Routes an SNP down this SSM's SSMQ, starting at MODIFIER.
   //
   void ProcessSsmqSnp(ServiceSM* modifier, Event& snpEvent);

   //  Routes an SNP down TRIGGER's Initiator queue, starting at MODIFIER.
   //
   void ProcessInitqSnp
      (const Trigger* trigger, const Initiator* modifier, Event& snpEvent);

   //  Routes an SIP down this SSM's SSMQ.
   //
   EventHandler::Rc ProcessInitReq
      (Event& currEvent, Event*& nextEvent, Phase& phase);

   //  Used during error recovery.
   //
   void EventError1(Event*& evt) const;

   //  Deletes the SSM if it is a modifier in the Null state.
   //
   void DeleteIdleModifier();

   //  The service identifier associated with this SSM.
   //
   ServiceId sid_;

   //  The SSM's current state.
   //
   StateId currState_;

   //  The SSM's next state.
   //
   StateId nextState_;

   //  Set if the SSM has entered the Null state.
   //
   bool idled_;

   //  The next SAP to be processed.
   //
   TriggerId nextSap_;

   //  The next SNP to be processed.
   //
   TriggerId nextSnp_;

   //  Set if an SAP has completed its routing down the InitQ.
   //
   bool triggered_[Trigger::MaxId + 1];

   //  The queue of modifiers.
   //
   Q1Way< ServiceSM > ssmq_;

   //  The parent SSM, if this SSM is a modifier.
   //
   ServiceSM* parentSsm_;

   //  The events currently owned by the SSM.
   //
   Q1Way< Event > eventq_[Event::Location_N];
};
}
#endif
