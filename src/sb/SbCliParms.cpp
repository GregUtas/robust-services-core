//==============================================================================
//
//  SbCliParms.cpp
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
#include "SbCliParms.h"
#include "Event.h"
#include "EventHandler.h"
#include "Factory.h"
#include "Parameter.h"
#include "Protocol.h"
#include "Service.h"
#include "Signal.h"
#include "State.h"
#include "Trigger.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fixed_string BadParameterExpl    = "Failed to build message: ";
fixed_string MessageNotFound     = "Failed to find message.";
fixed_string NoContextExpl       = "No context was found.";
fixed_string NoContextsExpl      = "There were no contexts to display.";
fixed_string NoEventExpl         = "There is no event with that identifier.";
fixed_string NoEventsExpl        = "This service has no events.";
fixed_string NoFactoryExpl       = "There is no factory with that identifier.";
fixed_string NoFactoryProtocol   = "That factory does not have a registered protocol.";
fixed_string NoHandlerExpl       = "There is no event handler with that identifier.";
fixed_string NoHandlersExpl      = "This service has no event handlers.";
fixed_string NoInvPoolExpl       = "There is no invoker pool in that faction.";
fixed_string NoMepsExpl          = "There were no MEPs to display.";
fixed_string NoMessagesExpl      = "There were no messages to display.";
fixed_string NoMsgsPortsExpl     = "There were no message ports to display.";
fixed_string NoParameterDisplay  = "This parameter does not support symbolic display.";
fixed_string NoParameterExpl     = "There is no parameter with that identifier.";
fixed_string NoParametersExpl    = "This protocol has no parameters.";
fixed_string NoProtocolDisplay   = "This protocol does not support symbolic display.";
fixed_string NoProtocolExpl      = "There is no protocol with that identifier.";
fixed_string NoPsmsExpl          = "There were no PSMs to display.";
fixed_string NoServiceExpl       = "There is no service with that identifier.";
fixed_string NoSignalExpl        = "There is no signal with that identifier.";
fixed_string NoSignalsExpl       = "This protocol has no signals.";
fixed_string NoSsmsExpl          = "There were no SSMs to display.";
fixed_string NoStateExpl         = "There is no state with that identifier.";
fixed_string NoStatesExpl        = "This service has no states.";
fixed_string NoTimersExpl        = "There were no timers to display.";
fixed_string NoTriggerExpl       = "There is no trigger with that identifier.";
fixed_string NoTriggersExpl      = "This service has no triggers.";
fixed_string ParameterNotAdded   = "Failed to add parameter.";
fixed_string SendFailure         = "Failed to send message.";
fixed_string SkippedFirstExpl    = "First signal=";
fixed_string SkippedMessagesExpl = "Warning: messages skipped=";

//------------------------------------------------------------------------------

fixed_string EventIdOptExpl = "EventId (default=all)";

EventIdOptParm::EventIdOptParm() :
   CliIntParm(EventIdOptExpl, 0, Event::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string FactoryIdMandExpl = "FactoryId";

FactoryIdMandParm::FactoryIdMandParm() :
   CliIntParm(FactoryIdMandExpl, 0, Factory::MaxId) { }

fixed_string FactoryIdOptExpl = "FactoryId (default=all)";

FactoryIdOptParm::FactoryIdOptParm() :
   CliIntParm(FactoryIdOptExpl, 0, Factory::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string HandlerIdOptExpl = "EventHandlerId (default=all)";

HandlerIdOptParm::HandlerIdOptParm() :
   CliIntParm(HandlerIdOptExpl, 0, EventHandler::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string ParameterIdOptExpl = "ParameterId (default=all)";

ParameterIdOptParm::ParameterIdOptParm() :
   CliIntParm(ParameterIdOptExpl, 0, Parameter::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string ProtocolIdMandExpl = "ProtocolId";

ProtocolIdMandParm::ProtocolIdMandParm() :
   CliIntParm(ProtocolIdMandExpl, 0, Protocol::MaxId) { }

fixed_string ProtocolIdOptExpl = "ProtocolId (default=all)";

ProtocolIdOptParm::ProtocolIdOptParm() :
   CliIntParm(ProtocolIdOptExpl, 0, Protocol::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string ServiceIdMandExpl = "ServiceId";

ServiceIdMandParm::ServiceIdMandParm() :
   CliIntParm(ServiceIdMandExpl, 0, Service::MaxId) { }

fixed_string ServiceIdOptExpl = "ServiceId (default=all)";

ServiceIdOptParm::ServiceIdOptParm() :
   CliIntParm(ServiceIdOptExpl, 0, Service::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string SignalIdMandExpl = "SignalId";

SignalIdMandParm::SignalIdMandParm() :
   CliIntParm(SignalIdMandExpl, 0, Signal::MaxId) { }

fixed_string SignalIdOptExpl = "SignalId (default=all)";

SignalIdOptParm::SignalIdOptParm() :
   CliIntParm(SignalIdOptExpl, 0, Signal::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string StateIdOptExpl = "State::Id (default=all)";

StateIdOptParm::StateIdOptParm() :
   CliIntParm(StateIdOptExpl, 0, State::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string TriggerIdOptExpl = "TriggerId (default=all)";

TriggerIdOptParm::TriggerIdOptParm() :
   CliIntParm(TriggerIdOptExpl, 0, Trigger::MaxId, true) { }
}
