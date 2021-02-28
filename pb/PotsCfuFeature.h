//==============================================================================
//
//  PotsCfuFeature.h
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
#ifndef POTSCFUFEATURE_H_INCLUDED
#define POTSCFUFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "PotsFeatures.h"
#include "BcAddress.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfuFeature : public PotsFeature
{
   friend class Singleton< PotsCfuFeature >;

   PotsCfuFeature();
   ~PotsCfuFeature();
   CliText* Attrs() const override;
   PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};

//------------------------------------------------------------------------------

class PotsCfuFeatureProfile : public DnRouteFeatureProfile
{
public:
   explicit PotsCfuFeatureProfile(Address::DN dn);
   ~PotsCfuFeatureProfile();
private:
   bool Activate(PotsProfile& profile, CliThread& cli) override;
};
}
#endif
