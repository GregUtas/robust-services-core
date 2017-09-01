//==============================================================================
//
//  PotsCwtFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCWTFEATURE_H_INCLUDED
#define POTSCWTFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCwtFeature : public PotsFeature
{
   friend class Singleton< PotsCwtFeature >;
private:
   PotsCwtFeature();
   ~PotsCwtFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};
}
#endif
