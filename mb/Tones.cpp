//==============================================================================
//
//  Tones.cpp
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
#include "Tones.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Singleton.h"
#include "Switch.h"
#include "SysTypes.h"
#include "ToneRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
fn_name Tone_ctor = "Tone.ctor";

Tone::Tone(Id tid)
{
   Debug::ft(Tone_ctor);

   tid_.SetId(tid);
   Singleton< ToneRegistry >::Instance()->BindTone(*this);
}

//------------------------------------------------------------------------------

fn_name Tone_dtor = "Tone.dtor";

Tone::~Tone()
{
   Debug::ft(Tone_dtor);

   Singleton< ToneRegistry >::Instance()->UnbindTone(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Tone::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Tone* >(&local);
   return ptrdiff(&fake->tid_, fake);
}

//------------------------------------------------------------------------------

void Tone::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Circuit::Display(stream, prefix, options);

   stream << prefix << "tid : " << tid_.to_str() << CRLF;
}

//==============================================================================

ToneBusy::ToneBusy() : Tone(Busy) { }

ToneBusy::~ToneBusy() { }

string ToneBusy::Name() const
{
   return "Busy tone";
}

//------------------------------------------------------------------------------

ToneCallWaiting::ToneCallWaiting() : Tone(CallWaiting) { }

ToneCallWaiting::~ToneCallWaiting() { }

string ToneCallWaiting::Name() const
{
   return "Call waiting tone";
}

//------------------------------------------------------------------------------

ToneConfirmation::ToneConfirmation() : Tone(Confirmation) { }

ToneConfirmation::~ToneConfirmation() { }

string ToneConfirmation::Name() const
{
   return "Confirmation tone";
}

//------------------------------------------------------------------------------

ToneDial::ToneDial() : Tone(Dial) { }

ToneDial::~ToneDial() { }

string ToneDial::Name() const
{
   return "Dial tone";
}

//------------------------------------------------------------------------------

ToneHeld::ToneHeld() : Tone(Held) { }

ToneHeld::~ToneHeld() { }

string ToneHeld::Name() const
{
   return "Held tone";
}

//------------------------------------------------------------------------------

ToneReceiverOffHook::ToneReceiverOffHook() : Tone(ReceiverOffHook) { }

ToneReceiverOffHook::~ToneReceiverOffHook() { }

string ToneReceiverOffHook::Name() const
{
   return "Receiver off-hook tone";
}

//------------------------------------------------------------------------------

ToneReorder::ToneReorder() : Tone(Reorder) { }

ToneReorder::~ToneReorder() { }

string ToneReorder::Name() const
{
   return "Reorder tone";
}

//------------------------------------------------------------------------------

ToneRingback::ToneRingback() : Tone(Ringback) { }

ToneRingback::~ToneRingback() { }

string ToneRingback::Name() const
{
   return "Ringback tone";
}

//------------------------------------------------------------------------------

fn_name ToneSilent_ctor = "ToneSilent.ctor";

ToneSilent::ToneSilent() : Tone(Silence)
{
   Debug::ft(ToneSilent_ctor);

   auto p = TsPort();

   if(p != Switch::SilentPort)
   {
      Debug::SwErr(ToneSilent_ctor, p, 0);
   }
}

ToneSilent::~ToneSilent() { }

string ToneSilent::Name() const
{
   return "Silent tone";
}

//------------------------------------------------------------------------------

ToneStutteredDial::ToneStutteredDial() : Tone(StutteredDial) { }

ToneStutteredDial::~ToneStutteredDial() { }

string ToneStutteredDial::Name() const
{
   return "Stuttered dial tone";
}
}
