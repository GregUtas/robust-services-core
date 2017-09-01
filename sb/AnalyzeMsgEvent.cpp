//==============================================================================
//
//  AnalyzeMsgEvent.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SbEvents.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Message.h"
#include "ProtocolSM.h"
#include "RootServiceSM.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name AnalyzeMsgEvent_ctor = "AnalyzeMsgEvent.ctor";

AnalyzeMsgEvent::AnalyzeMsgEvent(Message& msg) :
   Event(AnalyzeMsg, msg.Psm()->RootSsm()),
   msg_(&msg)
{
   Debug::ft(AnalyzeMsgEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name AnalyzeMsgEvent_dtor = "AnalyzeMsgEvent.dtor";

AnalyzeMsgEvent::~AnalyzeMsgEvent()
{
   Debug::ft(AnalyzeMsgEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name AnalyzeMsgEvent_BuildSap = "AnalyzeMsgEvent.BuildSap";

Event* AnalyzeMsgEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(AnalyzeMsgEvent_BuildSap);

   //  Modifiers receive the Analyze Message event in its original form.
   //
   return this;
}

//------------------------------------------------------------------------------

fn_name AnalyzeMsgEvent_BuildSnp = "AnalyzeMsgEvent.BuildSnp";

Event* AnalyzeMsgEvent::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(AnalyzeMsgEvent_BuildSnp);

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
