//==============================================================================
//
//  AnalyzeMsgEvent.cpp
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
#include "SbEvents.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Message.h"
#include "ProtocolSM.h"
#include "RootServiceSM.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
AnalyzeMsgEvent::AnalyzeMsgEvent(Message& msg) :
   Event(AnalyzeMsg, msg.Psm()->RootSsm()),
   msg_(&msg)
{
   Debug::ft("AnalyzeMsgEvent.ctor");
}

//------------------------------------------------------------------------------

AnalyzeMsgEvent::~AnalyzeMsgEvent()
{
   Debug::ftnt("AnalyzeMsgEvent.dtor");
}

//------------------------------------------------------------------------------

Event* AnalyzeMsgEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("AnalyzeMsgEvent.BuildSap");

   //  Modifiers receive the Analyze Message event in its original form.
   //
   return this;
}

//------------------------------------------------------------------------------

Event* AnalyzeMsgEvent::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("AnalyzeMsgEvent.BuildSnp");

   //  Notification is not provided after message analysis.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

void AnalyzeMsgEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Event::Display(stream, prefix, options);

   stream << prefix << "msg : " << msg_ << CRLF;
}

//------------------------------------------------------------------------------

void AnalyzeMsgEvent::Patch(sel_t selector, void* arguments)
{
   Event::Patch(selector, arguments);
}
}
