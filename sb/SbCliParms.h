//==============================================================================
//
//  SbCliParms.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBCLIPARMS_H_INCLUDED
#define SBCLIPARMS_H_INCLUDED

#include "CliIntParm.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Strings used by commands in the SessionBase increment.
//
extern fixed_string BadParameterExpl;
extern fixed_string MessageNotFound;
extern fixed_string NoContextExpl;
extern fixed_string NoContextsExpl;
extern fixed_string NoEventExpl;
extern fixed_string NoEventsExpl;
extern fixed_string NoFactoryExpl;
extern fixed_string NoFactoryProtocol;
extern fixed_string NoHandlerExpl;
extern fixed_string NoHandlersExpl;
extern fixed_string NoInvPoolExpl;
extern fixed_string NoMepsExpl;
extern fixed_string NoMessagesExpl;
extern fixed_string NoMsgsPortsExpl;
extern fixed_string NoParameterDisplay;
extern fixed_string NoParameterExpl;
extern fixed_string NoParametersExpl;
extern fixed_string NoProtocolDisplay;
extern fixed_string NoProtocolExpl;
extern fixed_string NoPsmsExpl;
extern fixed_string NoServiceExpl;
extern fixed_string NoSignalExpl;
extern fixed_string NoSignalsExpl;
extern fixed_string NoSsmsExpl;
extern fixed_string NoStateExpl;
extern fixed_string NoStatesExpl;
extern fixed_string NoTimersExpl;
extern fixed_string NoTriggerExpl;
extern fixed_string NoTriggersExpl;
extern fixed_string ParameterNotAdded;
extern fixed_string SendFailure;
extern fixed_string SkippedFirstExpl;
extern fixed_string SkippedMessagesExpl;

//------------------------------------------------------------------------------
//
//  Parameter for an EventId.
//
class EventIdOptParm : public CliIntParm
{
public: EventIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a FactoryId.
//
class FactoryIdMandParm : public CliIntParm
{
public: FactoryIdMandParm();
};

class FactoryIdOptParm : public CliIntParm
{
public: FactoryIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an EventHandlerId.
//
class HandlerIdOptParm : public CliIntParm
{
public: HandlerIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a ParameterId.
//
class ParameterIdOptParm : public CliIntParm
{
public: ParameterIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a ProtocolId.
//
class ProtocolIdMandParm : public CliIntParm
{
public: ProtocolIdMandParm();
};

class ProtocolIdOptParm : public CliIntParm
{
public: ProtocolIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a ServiceId.
//
class ServiceIdMandParm : public CliIntParm
{
public: ServiceIdMandParm();
};

class ServiceIdOptParm : public CliIntParm
{
public: ServiceIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a SignalId.
//
class SignalIdMandParm : public CliIntParm
{
public: SignalIdMandParm();
};

class SignalIdOptParm : public CliIntParm
{
public: SignalIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a State::Id.
//
class StateIdOptParm : public CliIntParm
{
public: StateIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a TriggerId.
//
class TriggerIdOptParm : public CliIntParm
{
public: TriggerIdOptParm();
};
}
#endif
