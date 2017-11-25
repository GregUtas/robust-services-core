//==============================================================================
//
//  Tones.h
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
#ifndef TONES_H_INCLUDED
#define TONES_H_INCLUDED

#include "Circuit.h"
#include <cstddef>
#include <cstdint>
#include "NbTypes.h"
#include "RegCell.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  Subclass for tone circuits.
//
class Tone : public Circuit
{
   friend class Registry< Tone >;
public:
   //  Type for identifying a tone.
   //
   typedef uint8_t Id;

   //  Identifiers for various tones.
   //
   static const Id Silence         = 1;
   static const Id Dial            = 2;
   static const Id StutteredDial   = 3;
   static const Id Confirmation    = 4;
   static const Id Ringback        = 5;
   static const Id Busy            = 6;
   static const Id CallWaiting     = 7;
   static const Id Reorder         = 8;
   static const Id ReceiverOffHook = 9;
   static const Id Held            = 10;
   static const Id MaxId           = 10;

   static const Id Media           = UINT8_MAX;  // not connected to a tone

   //  Returns the tone's identifier.
   //
   Id Tid() const { return Id(tid_.GetId()); }

   //  Returns the offset to tid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   explicit Tone(Id tid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~Tone();
private:
   //  Overridden to prohibit copying.
   //
   Tone(const Tone& that);
   void operator=(const Tone& that);

   //  The tone's identifier.
   //
   RegCell tid_;
};

//------------------------------------------------------------------------------

class ToneSilent : public Tone
{
   friend class Singleton< ToneSilent >;
private:
   ToneSilent();
   ~ToneSilent();
   virtual std::string Name() const override;
};

class ToneDial : public Tone
{
   friend class Singleton< ToneDial >;
private:
   ToneDial();
   ~ToneDial();
   virtual std::string Name() const override;
};

class ToneStutteredDial : public Tone
{
   friend class Singleton< ToneStutteredDial >;
private:
   ToneStutteredDial();
   ~ToneStutteredDial();
   virtual std::string Name() const override;
};

class ToneConfirmation : public Tone
{
   friend class Singleton< ToneConfirmation >;
private:
   ToneConfirmation();
   ~ToneConfirmation();
   virtual std::string Name() const override;
};

class ToneRingback : public Tone
{
   friend class Singleton< ToneRingback >;
private:
   ToneRingback();
   ~ToneRingback();
   virtual std::string Name() const override;
};

class ToneBusy : public Tone
{
   friend class Singleton< ToneBusy >;
private:
   ToneBusy();
   ~ToneBusy();
   virtual std::string Name() const override;
};

class ToneCallWaiting : public Tone
{
   friend class Singleton< ToneCallWaiting >;
private:
   ToneCallWaiting();
   ~ToneCallWaiting();
   virtual std::string Name() const override;
};

class ToneReorder : public Tone
{
   friend class Singleton< ToneReorder >;
private:
   ToneReorder();
   ~ToneReorder();
   virtual std::string Name() const override;
};

class ToneReceiverOffHook : public Tone
{
   friend class Singleton< ToneReceiverOffHook >;
private:
   ToneReceiverOffHook();
   ~ToneReceiverOffHook();
   virtual std::string Name() const override;
};

class ToneHeld : public Tone
{
   friend class Singleton< ToneHeld >;
private:
   ToneHeld();
   ~ToneHeld();
   virtual std::string Name() const override;
};
}
#endif
