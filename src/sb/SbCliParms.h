//==============================================================================
//
//  SbCliParms.h
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
#ifndef SBCLIPARMS_H_INCLUDED
#define SBCLIPARMS_H_INCLUDED

#include "CliIntParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Strings used by commands in the SessionBase increment.
//
extern NodeBase::fixed_string BadParameterExpl;
extern NodeBase::fixed_string MessageNotFound;
extern NodeBase::fixed_string NoContextExpl;
extern NodeBase::fixed_string NoContextsExpl;
extern NodeBase::fixed_string NoEventExpl;
extern NodeBase::fixed_string NoEventsExpl;
extern NodeBase::fixed_string NoFactoryExpl;
extern NodeBase::fixed_string NoFactoryProtocol;
extern NodeBase::fixed_string NoHandlerExpl;
extern NodeBase::fixed_string NoHandlersExpl;
extern NodeBase::fixed_string NoInvPoolExpl;
extern NodeBase::fixed_string NoMepsExpl;
extern NodeBase::fixed_string NoMessagesExpl;
extern NodeBase::fixed_string NoMsgsPortsExpl;
extern NodeBase::fixed_string NoParameterDisplay;
extern NodeBase::fixed_string NoParameterExpl;
extern NodeBase::fixed_string NoParametersExpl;
extern NodeBase::fixed_string NoProtocolDisplay;
extern NodeBase::fixed_string NoProtocolExpl;
extern NodeBase::fixed_string NoPsmsExpl;
extern NodeBase::fixed_string NoServiceExpl;
extern NodeBase::fixed_string NoSignalExpl;
extern NodeBase::fixed_string NoSignalsExpl;
extern NodeBase::fixed_string NoSsmsExpl;
extern NodeBase::fixed_string NoStateExpl;
extern NodeBase::fixed_string NoStatesExpl;
extern NodeBase::fixed_string NoTimersExpl;
extern NodeBase::fixed_string NoTriggerExpl;
extern NodeBase::fixed_string NoTriggersExpl;
extern NodeBase::fixed_string ParameterNotAdded;
extern NodeBase::fixed_string SendFailure;
extern NodeBase::fixed_string SkippedFirstExpl;
extern NodeBase::fixed_string SkippedMessagesExpl;

//------------------------------------------------------------------------------
//
//  Parameter for an EventId.
//
class EventIdOptParm : public NodeBase::CliIntParm
{
public: EventIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a FactoryId.
//
class FactoryIdMandParm : public NodeBase::CliIntParm
{
public: FactoryIdMandParm();
};

class FactoryIdOptParm : public NodeBase::CliIntParm
{
public: FactoryIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an EventHandlerId.
//
class HandlerIdOptParm : public NodeBase::CliIntParm
{
public: HandlerIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a ParameterId.
//
class ParameterIdOptParm : public NodeBase::CliIntParm
{
public: ParameterIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a ProtocolId.
//
class ProtocolIdMandParm : public NodeBase::CliIntParm
{
public: ProtocolIdMandParm();
};

class ProtocolIdOptParm : public NodeBase::CliIntParm
{
public: ProtocolIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a ServiceId.
//
class ServiceIdMandParm : public NodeBase::CliIntParm
{
public: ServiceIdMandParm();
};

class ServiceIdOptParm : public NodeBase::CliIntParm
{
public: ServiceIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a SignalId.
//
class SignalIdMandParm : public NodeBase::CliIntParm
{
public: SignalIdMandParm();
};

class SignalIdOptParm : public NodeBase::CliIntParm
{
public: SignalIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a State::Id.
//
class StateIdOptParm : public NodeBase::CliIntParm
{
public: StateIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a TriggerId.
//
class TriggerIdMandParm : public NodeBase::CliIntParm
{
public: TriggerIdMandParm();
};

class TriggerIdOptParm : public NodeBase::CliIntParm
{
public: TriggerIdOptParm();
};
}
#endif
