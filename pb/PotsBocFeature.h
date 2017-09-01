//==============================================================================
//
//  PotsBocFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSBOCFEATURE_H_INCLUDED
#define POTSBOCFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBocFeature : public PotsFeature
{
   friend class Singleton< PotsBocFeature >;
private:
   PotsBocFeature();
   ~PotsBocFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};
}
#endif
