//==============================================================================
//
//  EventHandler.cpp
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
#include "EventHandler.h"
#include <ostream>
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "Event.h"
#include "ServiceSM.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;

//------------------------------------------------------------------------------

namespace SessionBase
{
EventHandler::EventHandler()
{
   Debug::ft("EventHandler.ctor");
}

//------------------------------------------------------------------------------

fn_name EventHandler_dtor = "EventHandler.dtor";

EventHandler::~EventHandler()
{
   Debug::ftnt(EventHandler_dtor);

   Debug::SwLog(EventHandler_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

EventHandler::Rc EventHandler::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("EventHandler.ProcessEvent");

   //  An event handler must override this function if it can be invoked.
   //
   Context::Kill
      (strOver(this), pack3(ssm.Sid(), ssm.CurrState(), currEvent.Eid()));
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
