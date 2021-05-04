//==============================================================================
//
//  AnalyzeSnpEvent.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
AnalyzeSnpEvent::AnalyzeSnpEvent(ServiceSM& owner, StateId currState,
   StateId nextState, Event& currEvent, TriggerId tid) :
   Event(AnalyzeSnp, &owner),
   currState_(currState),
   nextState_(nextState),
   currEvent_(&currEvent),
   trigger_(tid)
{
   Debug::ft("AnalyzeSnpEvent.ctor");
}

//------------------------------------------------------------------------------

AnalyzeSnpEvent::~AnalyzeSnpEvent()
{
   Debug::ftnt("AnalyzeSnpEvent.dtor");
}

//------------------------------------------------------------------------------

Event* AnalyzeSnpEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("AnalyzeSnpEvent.BuildSap");

   //  Analysis is not provided before notification.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

Event* AnalyzeSnpEvent::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("AnalyzeSnpEvent.BuildSnp");

   //  Second-order modifiers receive the Analyze SNP event in its
   //  original form.
   //
   return this;
}

//------------------------------------------------------------------------------

void AnalyzeSnpEvent::Capture
   (ServiceId sid, const State& state, EventHandler::Rc rc) const
{
   auto rec = new SxpTrace(sid, state, *this, rc);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void AnalyzeSnpEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Event::Display(stream, prefix, options);

   stream << prefix << "currState : " << int(currState_) << CRLF;
   stream << prefix << "nextState : " << int(nextState_) << CRLF;
   stream << prefix << "currEvent : " << currEvent_ << CRLF;
   stream << prefix << "trigger   : " << int(trigger_) << CRLF;
}

//------------------------------------------------------------------------------

void AnalyzeSnpEvent::Patch(sel_t selector, void* arguments)
{
   Event::Patch(selector, arguments);
}
}
