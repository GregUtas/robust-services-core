//==============================================================================
//
//  TimerProtocol.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "TimerProtocol.h"
#include <ostream>
#include "Debug.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name TimeoutInfo_ctor= "TimeoutInfo.ctor";

TimeoutInfo::TimeoutInfo() :
   owner(nullptr),
   tid(0)
{
   Debug::ft(TimeoutInfo_ctor);
}

//------------------------------------------------------------------------------

void TimeoutInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "owner : " << owner << CRLF;
   stream << prefix << "tid   : " << tid << CRLF;
}

//==============================================================================

fn_name TimerProtocol_ctor = "TimerProtocol.ctor";

TimerProtocol::TimerProtocol() : TlvProtocol(TimerProtocolId, NIL_ID)
{
   Debug::ft(TimerProtocol_ctor);

   //  Create the timeout signal and parameter.
   //
   Singleton< TimeoutSignal >::Instance();
   Singleton< TimeoutParameter >::Instance();
}

//------------------------------------------------------------------------------

fn_name TimerProtocol_dtor = "TimerProtocol.dtor";

TimerProtocol::~TimerProtocol()
{
   Debug::ft(TimerProtocol_dtor);
}

//==============================================================================

fn_name TimeoutSignal_ctor = "TimeoutSignal.ctor";

TimeoutSignal::TimeoutSignal() : Signal(TimerProtocolId, Timeout)
{
   Debug::ft(TimeoutSignal_ctor);
}

//------------------------------------------------------------------------------

fn_name TimeoutSignal_dtor = "TimeoutSignal.dtor";

TimeoutSignal::~TimeoutSignal()
{
   Debug::ft(TimeoutSignal_dtor);
}

//==============================================================================

fn_name TimeoutParameter_ctor = "TimeoutParameter.ctor";

TimeoutParameter::TimeoutParameter() : TlvParameter(TimerProtocolId, Timeout)
{
   Debug::ft(TimeoutParameter_ctor);

   BindUsage(Signal::Timeout, Mandatory);
}

//------------------------------------------------------------------------------

fn_name TimeoutParameter_dtor = "TimeoutParameter.dtor";

TimeoutParameter::~TimeoutParameter()
{
   Debug::ft(TimeoutParameter_dtor);
}

//------------------------------------------------------------------------------

void TimeoutParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   auto toi = reinterpret_cast< const TimeoutInfo* >(bytes);
   toi->Display(stream, prefix);
}
}
