//==============================================================================
//
//  AnalyzeSnpEvent.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
