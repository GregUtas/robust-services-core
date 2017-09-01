//==============================================================================
//
//  ForceTransitionEvent.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SbEvents.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name ForceTransitionEvent_ctor = "ForceTransitionEvent.ctor";

ForceTransitionEvent::ForceTransitionEvent
   (ServiceSM& owner, const EventHandler& handler) :
   Event(ForceTransition, &owner),
   handler_(&handler)
{
   Debug::ft(ForceTransitionEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name ForceTransitionEvent_dtor = "ForceTransitionEvent.dtor";

ForceTransitionEvent::~ForceTransitionEvent()
{
   Debug::ft(ForceTransitionEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name ForceTransitionEvent_BuildSap = "ForceTransitionEvent.BuildSap";

Event* ForceTransitionEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(ForceTransitionEvent_BuildSap);

   //  Modifiers cannot analyze or intercept the Force Transition event.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

void ForceTransitionEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Event::Display(stream, prefix, options);

   stream << prefix << "handler : " << handler_ << CRLF;
}

//------------------------------------------------------------------------------

void ForceTransitionEvent::Patch(sel_t selector, void* arguments)
{
   Event::Patch(selector, arguments);
}
}
