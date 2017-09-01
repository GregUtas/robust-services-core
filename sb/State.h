//==============================================================================
//
//  State.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include "Event.h"
#include "RegCell.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Subclassed by a Service to define states for its state machine.  Each
//  subclass must be a singleton.
//
class State : public Protected
{
   friend class Registry< State >;
public:
   //  Allows "Id" to refer to a state identifier in this class hierarchy.
   //
   typedef StateId Id;

   //> Highest valid state identifier;
   //
   static const Id MaxId = 63;

   //  Returns the state's identifier.
   //
   Id Stid() const { return Id(stid_.GetId()); }

   //  Returns the service against which the state is registered.
   //
   ServiceId Sid() const { return sid_; }

   //  Returns the event handler registered against event EID.
   //
   EventHandlerId GetHandler(EventId eid) const;

   //  Returns the message analyzer registered against PID.
   //
   EventHandlerId MsgAnalyzer(ServicePortId pid) const;

   //  Returns the offset to stid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the corresponding member variables and initializes all other
   //  fields to default values.  Registers the state against SID and
   //  registers system-defined event handlers with the state.  Protected
   //  because this class is virtual.
   //
   State(ServiceId sid, Id stid);

   //  Removes the state from its service.  Protected because subclasses
   //  should be singletons.
   //
   virtual ~State();

   //  Registers the event handler associated with EHID so that it is invoked
   //  when the service is in this state and the internal event identified by
   //  EID is raised.  Invoked by a subclass constructor.
   //
   bool BindEventHandler(EventHandlerId ehid, EventId eid);

   //  Registers the message analyzer associated with EHID so that it is
   //  invoked when the service is in this state and a message arrives on
   //  a PSM that the service identifies by PID.  Invoked by a subclass
   //  constructor.
   //
   bool BindMsgAnalyzer(EventHandlerId ehid, ServicePortId pid);
private:
   //  Overridden to prohibit copying.
   //
   State(const State& that);
   void operator=(const State& that);

   //  The state's identifier.
   //
   RegCell stid_;

   //  The service to which this state belongs.
   //
   ServiceId sid_;

   //  Registry for the state's event handlers.
   //
   EventHandlerId handlers_[Event::MaxId + 1];

   //  Registry for the state's message analyzers.
   //
   EventHandlerId msgAnalyzers_[MaxServicePortId + 1];
};
}
#endif
