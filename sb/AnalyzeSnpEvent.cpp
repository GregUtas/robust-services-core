//==============================================================================
//
//  AnalyzeSnpEvent.cpp
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
#include "SbEvents.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SbTrace.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name AnalyzeSnpEvent_ctor = "AnalyzeSnpEvent.ctor";

AnalyzeSnpEvent::AnalyzeSnpEvent(ServiceSM& owner, StateId currState,
   StateId nextState, Event& currEvent, TriggerId tid) :
   Event(AnalyzeSnp, &owner),
   currState_(currState),
   nextState_(nextState),
   currEvent_(&currEvent),
   trigger_(tid)
{
   Debug::ft(AnalyzeSnpEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name AnalyzeSnpEvent_dtor = "AnalyzeSnpEvent.dtor";

AnalyzeSnpEvent::~AnalyzeSnpEvent()
{
   Debug::ft(AnalyzeSnpEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name AnalyzeSnpEvent_BuildSap = "AnalyzeSnpEvent.BuildSap";

Event* AnalyzeSnpEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(AnalyzeSnpEvent_BuildSap);

   //  Analysis is not provided before notification.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name AnalyzeSnpEvent_BuildSnp = "AnalyzeSnpEvent.BuildSnp";

Event* AnalyzeSnpEvent::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(AnalyzeSnpEvent_BuildSnp);

   //  Second-order modifiers receive the Analyze SNP event in its
   //  original form.
   //
   return this;
}

//------------------------------------------------------------------------------

void AnalyzeSnpEvent::Capture
   (ServiceId sid, const State& state, EventHandler::Rc rc) const
{
   new SxpTrace(sid, state, *this, rc);
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
