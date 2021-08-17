//==============================================================================
//
//  SbEvents.h
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
#ifndef SBEVENTS_H_INCLUDED
#define SBEVENTS_H_INCLUDED

#include "Event.h"
#include "EventHandler.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A PSM's ProcessIcMsg function raises this event to instruct the root
//  SSM to analyze the incoming message.
//
class AnalyzeMsgEvent : public Event
{
public:
   //  MSG is the message to be analyzed.  Set by PSMs.
   //
   explicit AnalyzeMsgEvent(Message& msg);

   //  Not subclassed.
   //
   ~AnalyzeMsgEvent();

   //  Returns the message to be analyzed.
   //
   Message* Msg() const { return msg_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  The Analyze Message event is passed to modifiers as is, so this
   //  returns the event itself.
   //
   Event* BuildSap(ServiceSM& owner, TriggerId tid) override;

   //  The Analyze Message event does not have an SNP.  Notification is
   //  not provided after message analysis, so this returns nullptr.
   //
   Event* BuildSnp(ServiceSM& owner, TriggerId tid) override;

   //  The message to be analyzed.
   //
   Message* const msg_;
};

//------------------------------------------------------------------------------
//
//  The framework raises this event to inform modifiers that an ancestor
//  is about to process an event.
//
class AnalyzeSapEvent : public Event
{
   friend class Event;
   friend class ServiceSM;
public:
   //  Frees any saved message associated with the event.  Not subclassed.
   //
   ~AnalyzeSapEvent();

   //  Returns the ancestor's current state.
   //
   StateId CurrState() const { return currState_; }

   //  Returns the event that the ancestor is about to process.
   //
   Event* CurrEvent() const { return currEvent_; }

   //  Returns the trigger associated with the SAP.
   //
   TriggerId GetTrigger() const { return trigger_; }

   //  Saves the event's context when a modifier returns EventHandler::Suspend
   //  o after being created by an Initiator that reacted to an SAP, or
   //  o when already active and reacting to a subsequent SAP.
   //  The modifier invokes this function on its parent's current SAP event.
   //  It must then save a pointer to the SAP so that it can eventually invoke
   //  RestoreContext (for EventHandler::Resume) or FreeContext (when forcing
   //  its parent to take another path).  Returns true on success.
   //
   bool SaveContext();

   //  Restores the event's context when its processing should resume.  The
   //  appropriate return code for resuming the processing of the returned
   //  event is provided; the return event is nullptr on error conditions.
   //  Processing resumes
   //  o with the next Initiator in the trigger's queue, if the modifier
   //    invoked SaveContext when it was initiated, or
   //  o with the next SSM in the SSMQ, if the modifier invoked SaveContext
   //    in reaction to a subsequent SAP.
   //
   Event* RestoreContext(EventHandler::Rc& rc);

   //  Purges the event's context when its processing should not resume.
   //  If freeMsg is true, the saved message is also deleted; otherwise,
   //  it is restored as the context message.
   //
   void FreeContext(bool freeMsg);

   //  Returns the message that was saved when the processing of this
   //  event was interrupted.
   //
   Message* SavedMsg() const { return savedMsg_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to support asynchronous modifier requests.
   //
   void Free() override;

   //  Overridden to support asynchronous modifier requests.
   //
   Event* Restore(EventHandler::Rc& rc) override;

   //  Overridden to support asynchronous modifier requests.
   //
   bool Save() override;
private:
   //  Uses OWNER to initialize the base class event and the other arguments
   //  to initialize the Analyze SAP event.  Private to restrict creation.
   //
   AnalyzeSapEvent(ServiceSM& owner, StateId currState,
      Event& currEvent, TriggerId tid);

   //  Returns the SSM that is processing this event.
   //
   ServiceSM* CurrSsm() const { return currSsm_; }

   //  Returns the initiator that is processing this event.
   //
   const Initiator* CurrInitiator() const { return currInit_; }

   //  Overridden to return the event itself, because an Analyze SAP event
   //  is passed to modifiers (of modifiers) in its original form.
   //
   Event* BuildSap(ServiceSM& owner, TriggerId tid) override;

   //  Overridden to return nullptr, because notification (SNP processing)
   //  is not supported after SAP processing.
   //
   Event* BuildSnp(ServiceSM& owner, TriggerId tid) override;

   //  Overridden to capture the underlying event associated with the SAP.
   //
   void Capture
      (ServiceId sid, const State& state, EventHandler::Rc rc) const override;

   //  Overridden to remember the initiator that is processing this event.
   //
   void SetCurrInitiator(const Initiator* init) override { currInit_ = init; }

   //  Overridden to remember the SSM that is processing this event.
   //
   void SetCurrSsm(ServiceSM* ssm) override { currSsm_ = ssm; }

   //  The ancestor's current state.
   //
   const StateId currState_;

   //  The event about to be processed by the ancestor.
   //
   Event* const currEvent_;

   //  The trigger associated with the SAP, if any.
   //
   const TriggerId trigger_;

   //  The SSM that is currently analyzing the SAP.
   //
   ServiceSM* currSsm_;

   //  The initiator that is currently analyzing the SAP.
   //
   const Initiator* currInit_;

   //  The context message if the SAP was saved.
   //
   Message* savedMsg_;
};

//------------------------------------------------------------------------------
//
//  The framework raises this event to inform modifiers that an ancestor
//  has just finished processing an event.
//
class AnalyzeSnpEvent : public Event
{
   friend class Event;
public:
   //  Returns the ancestor's current state.
   //
   StateId CurrState() const { return currState_; }

   //  Returns the ancestor's next state.
   //
   StateId NextState() const { return nextState_; }

   //  Returns the event just processed by the ancestor.
   //
   Event* CurrEvent() const { return currEvent_; }

   //  Returns the trigger associated with the SNP.
   //
   TriggerId GetTrigger() const { return trigger_; }

   //  Not subclassed.
   //
   ~AnalyzeSnpEvent();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Uses OWNER to initialize the base class event and the other arguments
   //  to initialize the Analyze SNP event.  Private to restrict creation.
   //
   AnalyzeSnpEvent(ServiceSM& owner, StateId currState,
      StateId nextState, Event& currEvent, TriggerId tid);

   //  Overridden to return nullptr, because SAP analysis is not provided
   //  before SNP processing.
   //
   Event* BuildSap(ServiceSM& owner, TriggerId tid) override;

   //  Overridden to return the event itself, because an Analyze SNP event
   //  is passed to modifiers (of modifiers) in its original form.
   //
   Event* BuildSnp(ServiceSM& owner, TriggerId tid) override;

   //  Overridden to capture the underlying event associated with the SNP.
   //
   void Capture
      (ServiceId sid, const State& state, EventHandler::Rc rc) const override;

   //  The ancestor's current state.
   //
   const StateId currState_;

   //  The ancestor's next state.
   //
   const StateId nextState_;

   //  The event just processed by the ancestor.
   //
   Event* const currEvent_;

   //  The trigger associated with the SNP, if any.
   //
   const TriggerId trigger_;
};

//------------------------------------------------------------------------------
//
//  A modifier raises this event to "warp" an ancestor's state machine to
//  a new state.  The modifier supplies an event handler that runs in the
//  ancestor's context, which allows other modifiers to observe the state
//  transition.  Modifiers may subclass this event to add parameters that
//  the event handler needs to perform the state transition.  The event's
//  identifier must remain fixed, however.
//
class ForceTransitionEvent : public Event
{
public:
   //  Uses OWNER (which must be the ancestor's SSM) to initialize the
   //  base class event and HANDLER to initialize the Force Transition
   //  event.
   //
   ForceTransitionEvent(ServiceSM& owner, const EventHandler& handler);

   //  Virtual to allow subclassing.
   //
   virtual ~ForceTransitionEvent();

   //  Returns the event handler that the modifier supplied.
   //
   const EventHandler* Handler() const { return handler_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to return nullptr, because a Force Transition event cannot
   //  be analyzed or intercepted.
   //
   Event* BuildSap(ServiceSM& owner, TriggerId tid) override;

   //  The event handler supplied by the modifier.
   //
   const EventHandler* const handler_;
};

//------------------------------------------------------------------------------
//
//  This event is raised
//  o within SessionBase, when an initiator requests the creation of its
//    SSM by returning EventHandler::Initiate, or
//  o by an SSM, when it receives a message that contains a service
//    activation parameter.  In this case, the event may be a request
//    to initiate (create) a modifier, or it may be a subsequent event
//    which is destined for a modifier that already exists.
//  The event is passed to existing modifiers (ProcessInitReq) and the target
//  modifier's (ProcessInitAck or ProcessInitNack).  Modifiers may subclass
//  this event to add their own parameters.
//
class InitiationReqEvent : public Event
{
public:
   //  Uses OWNER (which must be the parent's SSM) and LOC to initialize
   //  the base class event and the other arguments to initialize the
   //  initiation event.
   //
   InitiationReqEvent(ServiceSM& owner, ServiceId modifier,
      bool init = true, Message* msg = nullptr, ServiceSM* rcvr = nullptr,
      Location loc = Active);

   //  Virtual to allow subclassing.
   //
   virtual ~InitiationReqEvent();

   //  Returns the modifier that will receive the event.
   //
   ServiceId GetModifier() const { return modifier_; }

   //  Returns true if the event is an initiation request, and false
   //  if it is a subsequent event.
   //
   bool IsInitiation() const { return initiation_; }

   //  If the event was raised by an Initiator reacting to an SAP event,
   //  this returns that SAP event.  This function is typically used so
   //  that the modifier can invoke SaveContext on the SAP event.
   //
   AnalyzeSapEvent* GetSapEvent() const { return sapEvent_; }

   //  If the event was raised because of a service invocation parameter,
   //  this returns the message that contains that parameter.
   //
   Message* GetMessage() const { return message_; }

   //  If this is not an initiation request, this returns the modifier
   //  that will receive it.
   //
   ServiceSM* GetReceiver() const { return receiver_; }

   //  Denies an initiation request.  When the modifier's SSM is created,
   //  its ProcessInitNack function will be invoked.
   //
   void DenyRequest();

   //  Records whether the event is currently being screened by siblings.
   //
   void SetScreening(bool screening);

   //  Returns true if the event is currently being screened by siblings.
   //
   bool IsBeingScreened() const { return screening_; }

   //  Returns true if the initiation request was denied by a sibling.
   //
   bool WasDenied() const { return denied_; }

   //  Sets the modifier that will receive the event.
   //
   void SetReceiver(ServiceSM* receiver);

   //  Sets the SAP event that caused an Initiator to raise this event.
   //
   void SetSapEvent(AnalyzeSapEvent& sapEvent);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to return the event itself, because an initiation request
   //  event is passed to modifiers (of modifiers) in its original form.
   //
   Event* BuildSap(ServiceSM& owner, TriggerId tid) override;

   //  Overridden to return nullptr, because notification (SNP processing)
   //  is not supported after an initiation request event is processed.
   //
   Event* BuildSnp(ServiceSM& owner, TriggerId tid) override;

   //  Overridden to capture the service associated with the event.
   //
   void Capture
      (ServiceId sid, const State& state, EventHandler::Rc rc) const override;

   //  The modifier that will process the event.
   //
   const ServiceId modifier_;

   //  Set if the event is an initiation request.
   //
   const bool initiation_;

   //  Set if the request has been denied.
   //
   bool denied_;

   //  Set if modifier_'s siblings are currently screening the event.
   //
   bool screening_;

   //  The SAP event (if any) that caused an Initiator to raise this event.
   //
   AnalyzeSapEvent* sapEvent_;

   //  If the event was raised because of a service invocation parameter,
   //  the message that contains that parameter.
   //
   Message* const message_;

   //  If the event is not an initiation request, the modifier that will
   //  process it.
   //
   ServiceSM* receiver_;
};
}
#endif
