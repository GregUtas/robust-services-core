//==============================================================================
//
//  PotsCxfFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCXFFEATURE_H_INCLUDED
#define POTSCXFFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCxfFeature : public PotsFeature
{
   friend class Singleton< PotsCxfFeature >;
private:
   PotsCxfFeature();
   ~PotsCxfFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};
}
#endif
