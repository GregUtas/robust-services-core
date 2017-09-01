//==============================================================================
//
//  PotsCfbFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCFBFEATURE_H_INCLUDED
#define POTSCFBFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "PotsFeatures.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfbFeature : public PotsFeature
{
   friend class Singleton< PotsCfbFeature >;
private:
   PotsCfbFeature();
   ~PotsCfbFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};

//------------------------------------------------------------------------------

class PotsCfbFeatureProfile : public DnRouteFeatureProfile
{
public:
   explicit PotsCfbFeatureProfile(Address::DN dn);
   ~PotsCfbFeatureProfile();
private:
   virtual bool Activate(PotsProfile& profile, CliThread& cli) override;
};
}
#endif
