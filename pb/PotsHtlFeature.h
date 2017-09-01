//==============================================================================
//
//  PotsHtlFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSHTLFEATURE_H_INCLUDED
#define POTSHTLFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "PotsFeatureProfile.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsHtlFeature : public PotsFeature
{
   friend class Singleton< PotsHtlFeature >;
private:
   PotsHtlFeature();
   ~PotsHtlFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};

//------------------------------------------------------------------------------

class PotsHtlFeatureProfile : public PotsFeatureProfile
{
public:
   explicit PotsHtlFeatureProfile(Address::DN dn);
   ~PotsHtlFeatureProfile();
   Address::DN GetDN() const { return dn_; }
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   Address::DN dn_;
};
}
#endif
