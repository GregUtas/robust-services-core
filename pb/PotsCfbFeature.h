//==============================================================================
//
//  PotsCfbFeature.h
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
#ifndef POTSCFBFEATURE_H_INCLUDED
#define POTSCFBFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "PotsFeatures.h"

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
