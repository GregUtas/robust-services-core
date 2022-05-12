//==============================================================================
//
//  PotsWmlFeature.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef POTSWMLFEATURE_H_INCLUDED
#define POTSWMLFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "PotsFeatures.h"
#include <cstdint>
#include "BcAddress.h"
#include "NbTypes.h"

using namespace CallBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsWmlFeature : public PotsFeature
{
   friend class Singleton< PotsWmlFeature >;

   PotsWmlFeature();
   ~PotsWmlFeature();
   CliText* Attrs() const override;
   PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};

//------------------------------------------------------------------------------

class PotsWmlFeatureProfile : public DnRouteFeatureProfile
{
public:
   PotsWmlFeatureProfile(Address::DN dn, uint32_t timeout);
   ~PotsWmlFeatureProfile();
   uint32_t Timeout() const { return timeout_; }
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   bool Activate(const PotsProfile& profile, CliThread& cli) override;

   uint32_t timeout_;
};
}
#endif
