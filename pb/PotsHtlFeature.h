//==============================================================================
//
//  PotsHtlFeature.h
//
//  Copyright (C) 2017  Greg Utas
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
#ifndef POTSHTLFEATURE_H_INCLUDED
#define POTSHTLFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "PotsFeatureProfile.h"

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
