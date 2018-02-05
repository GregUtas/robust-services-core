//==============================================================================
//
//  PotsBocFeature.cpp
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
#include "PotsBocFeature.h"
#include "CliText.h"
#include "PotsFeatureProfile.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatures.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBocFeatureProfile : public PotsFeatureProfile
{
public:
   PotsBocFeatureProfile();
   ~PotsBocFeatureProfile();
};

class PotsBocAttrs : public CliText
{
public: PotsBocAttrs();
};

//==============================================================================

fixed_string PotsBocAbbrName = "boc";
fixed_string PotsBocFullName = "Barring of Outgoing Calls";

PotsBocAttrs::PotsBocAttrs() : CliText(PotsBocFullName, PotsBocAbbrName) { }

//------------------------------------------------------------------------------

fn_name PotsBocFeature_ctor = "PotsBocFeature.ctor";

PotsBocFeature::PotsBocFeature() :
   PotsFeature(BOC, false, PotsBocAbbrName, PotsBocFullName)
{
   Debug::ft(PotsBocFeature_ctor);

   SetIncompatible(HTL);
   SetIncompatible(WML);
   SetIncompatible(CFU);
   SetIncompatible(CFB);
   SetIncompatible(CFN);
   SetIncompatible(TWC);
   SetIncompatible(CXF);
}

//------------------------------------------------------------------------------

fn_name PotsBocFeature_dtor = "PotsBocFeature.dtor";

PotsBocFeature::~PotsBocFeature()
{
   Debug::ft(PotsBocFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsBocFeature::Attrs() const { return new PotsBocAttrs; }

//------------------------------------------------------------------------------

fn_name PotsBocFeature_Subscribe = "PotsBocFeature.Subscribe";

PotsFeatureProfile* PotsBocFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsBocFeature_Subscribe);

   cli.EndOfInput(false);
   return new PotsBocFeatureProfile;
}

//==============================================================================

fn_name PotsBocFeatureProfile_ctor = "PotsBocFeatureProfile.ctor";

PotsBocFeatureProfile::PotsBocFeatureProfile() : PotsFeatureProfile(BOC)
{
   Debug::ft(PotsBocFeatureProfile_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsBocFeatureProfile_dtor = "PotsBocFeatureProfile.dtor";

PotsBocFeatureProfile::~PotsBocFeatureProfile()
{
   Debug::ft(PotsBocFeatureProfile_dtor);
}
}
