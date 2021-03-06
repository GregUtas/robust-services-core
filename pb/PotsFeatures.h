//==============================================================================
//
//  PotsFeatures.h
//
//  Copyright (C) 2013-2020  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef POTSFEATURES_H_INCLUDED
#define POTSFEATURES_H_INCLUDED

#include "PotsFeatureProfile.h"
#include "BcAddress.h"
#include "PotsFeature.h"

using namespace CallBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Identifiers for POTS features.
//
constexpr PotsFeature::Id SUS = 1;   // Suspended service
constexpr PotsFeature::Id BOC = 2;   // Barring of Outcoming Calls
constexpr PotsFeature::Id HTL = 3;   // Hot Line
constexpr PotsFeature::Id WML = 4;   // Warm Line
constexpr PotsFeature::Id BIC = 5;   // Barring of Incoming Calls
constexpr PotsFeature::Id CFU = 6;   // Call Forwarding Unconditional
constexpr PotsFeature::Id CFB = 7;   // Call Forwarding Busy
constexpr PotsFeature::Id CFN = 8;   // Call Forwarding Don't Answer
constexpr PotsFeature::Id CWT = 9;   // Call Waiting
constexpr PotsFeature::Id TWC = 10;  // Three-Way Calling
constexpr PotsFeature::Id CXF = 11;  // Call Transfer

//------------------------------------------------------------------------------

class DnRouteFeatureProfile : public PotsFeatureProfile
{
public:
   Address::DN GetDN() const { return dn_; }
   void SetDN(Address::DN dn);
   bool IsActive() const { return on_; }
   void SetActive(bool on);
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   DnRouteFeatureProfile(PotsFeature::Id fid, Address::DN dn);
   virtual ~DnRouteFeatureProfile();
   bool Activate(PotsProfile& profile, CliThread& cli) override;
   bool Deactivate(PotsProfile& profile) override;
private:
   Address::DN dn_;
   bool on_;
};
}
#endif
