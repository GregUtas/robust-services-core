//==============================================================================
//
//  PotsFeatures.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSFEATURES_H_INCLUDED
#define POTSFEATURES_H_INCLUDED

#include "PotsFeatureProfile.h"
#include "BcAddress.h"
#include "PotsFeature.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Identifiers for POTS features.
//
enum PotsFeatureIds
{
   SUS = 1,   // Suspended service
   BOC = 2,   // Barring of Outcoming Calls
   HTL = 3,   // Hot Line
   WML = 4,   // Warm Line
   BIC = 5,   // Barring of Incoming Calls
   CFU = 6,   // Call Forwarding Unconditional
   CFB = 7,   // Call Forwarding Busy
   CFN = 8,   // Call Forwarding Don't Answer
   CWT = 9,   // Call Waiting
   TWC = 10,  // Three-Way Calling
   CXF = 11   // Call Transfer
};

//------------------------------------------------------------------------------

class DnRouteFeatureProfile : public PotsFeatureProfile
{
public:
   Address::DN GetDN() const { return dn_; }
   void SetDN(Address::DN dn) { dn_ = dn; }
   bool IsActive() const { return on_; }
   void SetActive(bool on) { on_ = on; }
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   DnRouteFeatureProfile(PotsFeature::Id fid, Address::DN dn);
   virtual ~DnRouteFeatureProfile();
   virtual bool Activate(PotsProfile& profile, CliThread& cli) override;
   virtual bool Deactivate(PotsProfile& profile) override;
private:
   Address::DN dn_;
   bool on_;
};
}
#endif
