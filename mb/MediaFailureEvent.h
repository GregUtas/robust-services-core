//==============================================================================
//
//  MediaFailureEvent.h
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
#ifndef MEDIAFAILUREEVENT_H_INCLUDED
#define MEDIAFAILUREEVENT_H_INCLUDED

#include "Event.h"
#include "EventHandler.h"

namespace MediaBase
{
   class MediaEndpt;
}

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  A PSM or message analyzer raises this event when a media failure occurs.
//
class MediaFailureEvent : public Event
{
public:
   //  Uses OWNER (which must be a root SSM) to initialize the base class
   //  event and the other arguments to initialize the Media Failure event.
   //
   MediaFailureEvent(ServiceSM& owner, MediaEndpt& mep);

   //  Virtual to allow subclassing.
   //
   virtual ~MediaFailureEvent();

   //  Returns the MEP on which the media failure occurred.
   //
   MediaEndpt* Mep() const { return mep_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Overridden to return the event itself: a Media Failure event is
   //  passed to modifiers in its original form.
   //
   Event* BuildSap(ServiceSM& owner, TriggerId tid) override;

   //  Overridden to return nullptr: notification is not provided after
   //  a media failure.
   //
   Event* BuildSnp(ServiceSM& owner, TriggerId tid) override;

   //  The MEP on which the media failure occurred.
   //
   MediaEndpt* const mep_;
};
}
#endif
