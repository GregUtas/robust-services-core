//==============================================================================
//
//  ToneRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
public:
   //  Returns the port associated with the tone identified by TID.
   //
   static Switch::PortId ToneToPort(Tone::Id tid);

   //  Registers TONE.
   //
   bool BindTone(Tone& tone);

   //  Deregisters TONE.
   //
   void UnbindTone(Tone& tone);

   //  Returns the tone registered against TID.
   //
   Tone* GetTone(Tone::Id tid) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   ToneRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ToneRegistry();

   //  Registry for singleton subclasses of Tone.
   //
   Registry< Tone > tones_;
};
}
#endif
