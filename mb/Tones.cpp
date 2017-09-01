//==============================================================================
//
//  Tones.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Tones.h"
#include <iosfwd>
#include <sstream>
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
   std::ostringstream name;
   name << "Busy tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneCallWaiting::ToneCallWaiting() : Tone(CallWaiting) { }

ToneCallWaiting::~ToneCallWaiting() { }

string ToneCallWaiting::Name() const
{
   std::ostringstream name;
   name << "Call waiting tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneConfirmation::ToneConfirmation() : Tone(Confirmation) { }

ToneConfirmation::~ToneConfirmation() { }

string ToneConfirmation::Name() const
{
   std::ostringstream name;
   name << "Confirmation tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneDial::ToneDial() : Tone(Dial) { }

ToneDial::~ToneDial() { }

string ToneDial::Name() const
{
   std::ostringstream name;
   name << "Dial tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneHeld::ToneHeld() : Tone(Held) { }

ToneHeld::~ToneHeld() { }

string ToneHeld::Name() const
{
   std::ostringstream name;
   name << "Held tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneReceiverOffHook::ToneReceiverOffHook() : Tone(ReceiverOffHook) { }

ToneReceiverOffHook::~ToneReceiverOffHook() { }

string ToneReceiverOffHook::Name() const
{
   std::ostringstream name;
   name << "Receiver off-hook tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneReorder::ToneReorder() : Tone(Reorder) { }

ToneReorder::~ToneReorder() { }

string ToneReorder::Name() const
{
   std::ostringstream name;
   name << "Reorder tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneRingback::ToneRingback() : Tone(Ringback) { }

ToneRingback::~ToneRingback() { }

string ToneRingback::Name() const
{
   std::ostringstream name;
   name << "Ringback tone";
   return name.str();
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
   std::ostringstream name;
   name << "Silent tone";
   return name.str();
}

//------------------------------------------------------------------------------

ToneStutteredDial::ToneStutteredDial() : Tone(StutteredDial) { }

ToneStutteredDial::~ToneStutteredDial() { }

string ToneStutteredDial::Name() const
{
   std::ostringstream name;
   name << "Stuttered dial tone";
   return name.str();
}
}
