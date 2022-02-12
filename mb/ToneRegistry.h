//==============================================================================
//
//  ToneRegistry.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef TONEREGISTRY_H_INCLUDED
#define TONEREGISTRY_H_INCLUDED

#include "Dynamic.h"
#include "NbTypes.h"
#include "Registry.h"
#include "Switch.h"
#include "Tones.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  Registry for tones.
//
class ToneRegistry : public Dynamic
{
   friend class Singleton< ToneRegistry >;
   friend class Tone;
public:
   //  Deleted to prohibit copying.
   //
   ToneRegistry(const ToneRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ToneRegistry& operator=(const ToneRegistry& that) = delete;

   //  Returns the port associated with the tone identified by TID.
   //
   static Switch::PortId ToneToPort(Tone::Id tid);

   //  Returns the tone registered against TID.
   //
   Tone* GetTone(Tone::Id tid) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this is a singleton.
   //
   ToneRegistry();

   //  Private because this is a singleton.
   //
   ~ToneRegistry();

   //  Registers TONE.
   //
   bool BindTone(Tone& tone);

   //  Deregisters TONE.
   //
   void UnbindTone(Tone& tone);

   //  Registry for singleton subclasses of Tone.
   //
   Registry< Tone > tones_;
};
}
#endif
