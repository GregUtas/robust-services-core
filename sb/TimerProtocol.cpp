//==============================================================================
//
//  TimerProtocol.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "TimerProtocol.h"
#include <ostream>
#include "Debug.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
TimeoutInfo::TimeoutInfo() :
   owner(nullptr),
   tid(0)
{
   Debug::ft("TimeoutInfo.ctor");
}

//------------------------------------------------------------------------------

void TimeoutInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "owner : " << owner << CRLF;
   stream << prefix << "tid   : " << tid << CRLF;
}

//==============================================================================

TimerProtocol::TimerProtocol() : TlvProtocol(TimerProtocolId, NIL_ID)
{
   Debug::ft("TimerProtocol.ctor");

   //  Create the timeout signal and parameter.
   //
   Singleton< TimeoutSignal >::Instance();
   Singleton< TimeoutParameter >::Instance();
}

//------------------------------------------------------------------------------

TimerProtocol::~TimerProtocol()
{
   Debug::ftnt("TimerProtocol.dtor");
}

//==============================================================================

TimeoutSignal::TimeoutSignal() : Signal(TimerProtocolId, Timeout)
{
   Debug::ft("TimeoutSignal.ctor");
}

//------------------------------------------------------------------------------

TimeoutSignal::~TimeoutSignal()
{
   Debug::ftnt("TimeoutSignal.dtor");
}

//==============================================================================

TimeoutParameter::TimeoutParameter() : TlvParameter(TimerProtocolId, Timeout)
{
   Debug::ft("TimeoutParameter.ctor");

   BindUsage(Signal::Timeout, Mandatory);
}

//------------------------------------------------------------------------------

TimeoutParameter::~TimeoutParameter()
{
   Debug::ftnt("TimeoutParameter.dtor");
}

//------------------------------------------------------------------------------

void TimeoutParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   auto toi = reinterpret_cast< const TimeoutInfo* >(bytes);
   toi->Display(stream, prefix);
}
}
