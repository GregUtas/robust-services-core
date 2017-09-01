//==============================================================================
//
//  MediaFailureEvent.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MediaFailureEvent.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
fn_name MediaFailureEvent_ctor = "MediaFailureEvent.ctor";

MediaFailureEvent::MediaFailureEvent(ServiceSM& owner, MediaEndpt& mep) :
   Event(MediaFailure, &owner),
   mep_(&mep)
{
   Debug::ft(MediaFailureEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name MediaFailureEvent_dtor = "MediaFailureEvent.dtor";

MediaFailureEvent::~MediaFailureEvent()
{
   Debug::ft(MediaFailureEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name MediaFailureEvent_BuildSap = "MediaFailureEvent.BuildSap";

Event* MediaFailureEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(MediaFailureEvent_BuildSap);

   //  Modifiers receive the Media Failure event in its original form.
   //
   return this;
}

//------------------------------------------------------------------------------

fn_name MediaFailureEvent_BuildSnp = "MediaFailureEvent.BuildSnp";

Event* MediaFailureEvent::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(MediaFailureEvent_BuildSnp);

   //  Notification is not provided after a Media Failure event.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

void MediaFailureEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Event::Display(stream, prefix, options);

   stream << prefix << "mep : " << mep_ << CRLF;
}
}
