//==============================================================================
//
//  PotsCfuFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCFUFEATURE_H_INCLUDED
#define POTSCFUFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "PotsFeatures.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfuFeature : public PotsFeature
{
   friend class Singleton< PotsCfuFeature >;
private:
   PotsCfuFeature();
   ~PotsCfuFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};

//------------------------------------------------------------------------------

class PotsCfuFeatureProfile : public DnRouteFeatureProfile
{
public:
   explicit PotsCfuFeatureProfile(Address::DN dn);
   ~PotsCfuFeatureProfile();
private:
   virtual bool Activate(PotsProfile& profile, CliThread& cli) override;
};
}
#endif
