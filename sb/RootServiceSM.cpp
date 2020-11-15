//==============================================================================
//
//  RootServiceSM.cpp
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
#include "RootServiceSM.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "EventHandler.h"
#include "SbEvents.h"
#include "SsmContext.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
RootServiceSM::RootServiceSM(ServiceId sid) :
   ServiceSM(sid),
   ctx_(nullptr)
{
   Debug::ft("RootServiceSM.ctor");

   //  Register the SSM with its context.
   //
   ctx_ = static_cast< SsmContext* >(Context::RunningContext());

   if(ctx_ != nullptr) ctx_->SetRoot(this);
}

//------------------------------------------------------------------------------

RootServiceSM::~RootServiceSM()
{
   Debug::ftnt("RootServiceSM.dtor");

   //  Deregister the SSM from its context.
   //
   if(ctx_ != nullptr) ctx_->SetRoot(nullptr);
}

//------------------------------------------------------------------------------

void RootServiceSM::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ServiceSM::Display(stream, prefix, options);

   stream << prefix << "ctx : " << ctx_ << CRLF;
}

//------------------------------------------------------------------------------

void RootServiceSM::Patch(sel_t selector, void* arguments)
{
   ServiceSM::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

EventHandler::Rc RootServiceSM::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("RootServiceSM.ProcessInitAck");

   Context::Kill(strOver(this), Sid());
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc RootServiceSM::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("RootServiceSM.ProcessInitNack");

   Context::Kill(strOver(this), Sid());
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc RootServiceSM::ProcessSap(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("RootServiceSM.ProcessSap");

   Context::Kill(strOver(this), Sid());
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc RootServiceSM::ProcessSip(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("RootServiceSM.ProcessSip");

   Context::Kill(strOver(this), Sid());
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc RootServiceSM::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("RootServiceSM.ProcessSnp");

   Context::Kill(strOver(this), Sid());
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

Event* RootServiceSM::RaiseProtocolError(ProtocolSM& psm, ProtocolSM::Error err)
{
   Debug::ft("RootServiceSM.RaiseProtocolError");

   return new AnalyzeMsgEvent(*Context::ContextMsg());
}
}
