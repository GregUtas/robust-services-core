//==============================================================================
//
//  PotsWmlFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSWMLFEATURE_H_INCLUDED
#define POTSWMLFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "BcAddress.h"
#include "Clock.h"
#include "NbTypes.h"
#include "PotsFeatures.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsWmlFeature : public PotsFeature
{
   friend class Singleton< PotsWmlFeature >;
private:
   PotsWmlFeature();
   ~PotsWmlFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};

//------------------------------------------------------------------------------

class PotsWmlFeatureProfile : public DnRouteFeatureProfile
{
public:
   PotsWmlFeatureProfile(Address::DN dn, secs_t timeout);
   ~PotsWmlFeatureProfile();
   secs_t Timeout() const { return timeout_; }
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   virtual bool Activate(PotsProfile& profile, CliThread& cli) override;

   secs_t timeout_;
};
}
#endif
