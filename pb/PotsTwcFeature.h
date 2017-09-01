//==============================================================================
//
//  PotsTwcFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSTWCFEATURE_H_INCLUDED
#define POTSTWCFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsTwcFeature : public PotsFeature
{
   friend class Singleton< PotsTwcFeature >;
private:
   PotsTwcFeature();
   ~PotsTwcFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};
}
#endif
