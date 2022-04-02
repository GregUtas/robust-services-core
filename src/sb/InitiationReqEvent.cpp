//==============================================================================
//
//  InitiationReqEvent.cpp
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
#include "SbTrace.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
InitiationReqEvent::InitiationReqEvent(ServiceSM& owner, ServiceId modifier,
   bool init, Message* msg, ServiceSM* rcvr, Location loc) :
   Event(InitiationReq, &owner, loc),
   modifier_(modifier),
   initiation_(init),
   denied_(false),
   screening_(true),
   sapEvent_(nullptr),
   message_(msg),
   receiver_(rcvr)
{
   Debug::ft("InitiationReqEvent.ctor");
}

//------------------------------------------------------------------------------

InitiationReqEvent::~InitiationReqEvent()
{
   Debug::ftnt("InitiationReqEvent.dtor");
}

//------------------------------------------------------------------------------

Event* InitiationReqEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("InitiationReqEvent.BuildSap");

   //  Modifiers receive the initiation request event in its original form.
   //
   return this;
}

//------------------------------------------------------------------------------

Event* InitiationReqEvent::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("InitiationReqEvent.BuildSnp");

   //  Notification is not provided after a modifier is initiated.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

void InitiationReqEvent::Capture
   (ServiceId sid, const State& state, EventHandler::Rc rc) const
{
   auto rec = new SipTrace(sid, state, *this, rc);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void InitiationReqEvent::DenyRequest()
{
   Debug::ft("InitiationReqEvent.DenyRequest");

   denied_ = true;
}

//------------------------------------------------------------------------------

void InitiationReqEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Event::Display(stream, prefix, options);

   stream << prefix << "modifier   : " << int(modifier_) << CRLF;
   stream << prefix << "initiation : " << initiation_ << CRLF;
   stream << prefix << "denied     : " << denied_ << CRLF;
   stream << prefix << "screening  : " << screening_ << CRLF;
   stream << prefix << "sapEvent   : " << sapEvent_ << CRLF;
   stream << prefix << "message    : " << message_ << CRLF;
   stream << prefix << "receiver   : " << receiver_ << CRLF;
}

//------------------------------------------------------------------------------

void InitiationReqEvent::Patch(sel_t selector, void* arguments)
{
   Event::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void InitiationReqEvent::SetReceiver(ServiceSM* receiver)
{
   Debug::ft("InitiationReqEvent.SetReceiver");

   receiver_ = receiver;
}

//------------------------------------------------------------------------------

void InitiationReqEvent::SetSapEvent(AnalyzeSapEvent& sapEvent)
{
   Debug::ft("InitiationReqEvent.SetSapEvent");

   sapEvent_ = &sapEvent;
}

//------------------------------------------------------------------------------

void InitiationReqEvent::SetScreening(bool screening)
{
   Debug::ft("InitiationReqEvent.SetScreening");

   screening_ = screening;
}
}
