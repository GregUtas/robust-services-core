//==============================================================================
//
//  MediaFailureEvent.cpp
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
   Debug::ftnt(MediaFailureEvent_dtor);
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
