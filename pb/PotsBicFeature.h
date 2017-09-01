//==============================================================================
//
//  PotsBicFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSBICFEATURE_H_INCLUDED
#define POTSBICFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBicFeature : public PotsFeature
{
   friend class Singleton< PotsBicFeature >;
private:
   PotsBicFeature();
   ~PotsBicFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};
}
#endif
