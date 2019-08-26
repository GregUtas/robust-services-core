//==============================================================================
//
//  EventHandler.h
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
#ifndef EVENTHANDLER_H_INCLUDED
#define EVENTHANDLER_H_INCLUDED

#include "Protected.h"
#include <cstdint>
#include <iosfwd>
#include "SbTypes.h"

namespace NodeBase
{
   template< class T > class Registry;
}

namespace SessionBase
{
   class ServiceSM;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Each subclass defines an event handler for a service's state machine.
//  Event handlers include message analyzers, and each subclass must be a
//  singleton.
//
class EventHandler : public NodeBase::Protected
{
   friend class NodeBase::Registry< EventHandler >;
public:
   //  Allows "Id" to refer to an event handler identifier in this class
   //  hierarchy.
   //
   typedef EventHandlerId Id;

   //> Highest valid event handler identifier.
   //
   static const Id MaxId = UINT8_MAX;

   //  Event handler identifiers used within the SessionBase framework.  See
   //  SbHandlers.h for the event handlers associated with these.  The event
   //  handlers associated with these identifiers are automatically registered
   //  against each service and cannot be overwritten.
   //
   static const Id AnalyzeMsg      = 1;
   static const Id AnalyzeSap      = 2;
   static const Id AnalyzeSnp      = 3;
   static const Id ForceTransition = 4;
   static const Id InitiationReq   = 5;

   //  Predefined identifier for an event handler that handles media
   //  failures for a service that controls media streams.
   //
   static const Id MediaFailure = 6;

   //  Applications must start to number their event handlers from here.
   //
   static const Id NextId = 11;

   //  Returns true if it is valid for an application to register an
   //  event handler against EHID.
   //
   static bool AppCanRegister(Id ehid)
   {
      return ((ehid >= MediaFailure) && (ehid <= MaxId));
   }

   //  Returns true if it is valid for an application to register the
   //  event handler associated with EHID in one of its states.
   //
   static bool AppCanUse(Id ehid)
   {
      return ((ehid >= NextId) && (ehid <= MaxId));
   }

   //  Return codes (event routing instructions) from event handlers.
   //
   enum Rc
   {
      Suspend,    // end of transaction
      Continue,   // process another event within the same service
      Pass,       // pass the event to the next modifier, else to parent
      Initiate,   // request the initiation of a modifier
      Revert,     // return to parent with a new event
      Resume,     // return to parent with an event whose processing was
                  // suspended but which should now continue
      Rc_N        // number of event handler return codes
   };

   //  The actual event handler logic.  SSM is the state machine in which the
   //  event handler is running, and currEvent is the event to be handled.  The
   //  event handler sets nextEvent to the next event (if any) to be handled.
   //
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const;
protected:
   //  Protected because this class is virtual.
   //
   EventHandler();

   //  Protected because subclasses should be singletons.
   //
   virtual ~EventHandler();
};

//  Inserts a string for RC into STREAM.
//
std::ostream& operator<<(std::ostream& stream, EventHandler::Rc rc);
}
#endif
