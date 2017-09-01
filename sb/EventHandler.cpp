//==============================================================================
//
//  EventHandler.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "EventHandler.h"
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "Event.h"
#include "ServiceSM.h"
#include "SysTypes.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name EventHandler_ctor = "EventHandler.ctor";

EventHandler::EventHandler()
{
   Debug::ft(EventHandler_ctor);
}

//------------------------------------------------------------------------------

fn_name EventHandler_dtor = "EventHandler.dtor";

EventHandler::~EventHandler()
{
   Debug::ft(EventHandler_dtor);
}

//------------------------------------------------------------------------------

fn_name EventHandler_ProcessEvent = "EventHandler.ProcessEvent";

EventHandler::Rc EventHandler::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(EventHandler_ProcessEvent);

   //  An event handler must override this function if it can be invoked.
   //
   Context::Kill(EventHandler_ProcessEvent,
      pack2(ssm.Sid(), ssm.CurrState()), currEvent.Eid());
   return Suspend;
}

//------------------------------------------------------------------------------

fixed_string EventHandlerRcStrings[EventHandler::Rc_N + 1] =
{
   "suspend ",
   "continue",
   "pass    ",
   "initiate",
   "revert  ",
   "resume  ",
   ERROR_STR
};

ostream& operator<<(ostream& stream, EventHandler::Rc rc)
{
   if((rc >= 0) && (rc < EventHandler::Rc_N))
      stream << EventHandlerRcStrings[rc];
   else
      stream << EventHandlerRcStrings[EventHandler::Rc_N];
   return stream;
}
}