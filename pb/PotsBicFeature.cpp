//==============================================================================
//
//  PotsBicFeature.cpp
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
#include "PotsBicFeature.h"
#include "CliText.h"
#include "PotsFeatureProfile.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatures.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBicFeatureProfile : public PotsFeatureProfile
{
public:
   PotsBicFeatureProfile();
   ~PotsBicFeatureProfile();
};

class PotsBicAttrs : public CliText
{
public: PotsBicAttrs();
};

//==============================================================================

fn_name PotsBicFeatureProfile_ctor = "PotsBicFeatureProfile.ctor";

PotsBicFeatureProfile::PotsBicFeatureProfile() : PotsFeatureProfile(BIC)
{
   Debug::ft(PotsBicFeatureProfile_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsBicFeatureProfile_dtor = "PotsBicFeatureProfile.dtor";

PotsBicFeatureProfile::~PotsBicFeatureProfile()
{
   Debug::ftnt(PotsBicFeatureProfile_dtor);
}

//==============================================================================

fixed_string PotsBicAbbrName = "bic";
fixed_string PotsBicFullName = "Barring of Incoming Calls";

PotsBicAttrs::PotsBicAttrs() : CliText(PotsBicFullName, PotsBicAbbrName) { }

//------------------------------------------------------------------------------

fn_name PotsBicFeature_ctor = "PotsBicFeature.ctor";

PotsBicFeature::PotsBicFeature() :
   PotsFeature(BIC, false, PotsBicAbbrName, PotsBicFullName)
{
   Debug::ft(PotsBicFeature_ctor);

   SetIncompatible(CFU);
   SetIncompatible(CFB);
   SetIncompatible(CFN);
   SetIncompatible(CWT);
}

//------------------------------------------------------------------------------

fn_name PotsBicFeature_dtor = "PotsBicFeature.dtor";

PotsBicFeature::~PotsBicFeature()
{
   Debug::ftnt(PotsBicFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsBicFeature::Attrs() const { return new PotsBicAttrs; }

//------------------------------------------------------------------------------

fn_name PotsBicFeature_Subscribe = "PotsBicFeature.Subscribe";

PotsFeatureProfile* PotsBicFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsBicFeature_Subscribe);

   if(!cli.EndOfInput()) return nullptr;
   return new PotsBicFeatureProfile;
}
}
