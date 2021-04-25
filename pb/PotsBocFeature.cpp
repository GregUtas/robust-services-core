//==============================================================================
//
//  PotsBocFeature.cpp
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
#include "PotsBocFeature.h"
#include "PotsFeatureProfile.h"
#include "CliText.h"
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

//==============================================================================

fixed_string PotsBocAbbrName = "boc";
fixed_string PotsBocFullName = "Barring of Outgoing Calls";

PotsBocFeature::PotsBocFeature() :
   PotsFeature(BOC, false, PotsBocAbbrName, PotsBocFullName)
{
   Debug::ft("PotsBocFeature.ctor");

   SetIncompatible(HTL);
   SetIncompatible(WML);
   SetIncompatible(CFU);
   SetIncompatible(CFB);
   SetIncompatible(CFN);
   SetIncompatible(TWC);
   SetIncompatible(CXF);
}

//------------------------------------------------------------------------------

PotsBocFeature::~PotsBocFeature()
{
   Debug::ftnt("PotsBocFeature.dtor");
}

//------------------------------------------------------------------------------

CliText* PotsBocFeature::Attrs() const
{
   return new CliText(PotsBocFullName, PotsBocAbbrName);
}

//------------------------------------------------------------------------------

PotsFeatureProfile* PotsBocFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft("PotsBocFeature.Subscribe");

   if(!cli.EndOfInput()) return nullptr;
   return new PotsBocFeatureProfile;
}

//==============================================================================

PotsBocFeatureProfile::PotsBocFeatureProfile() : PotsFeatureProfile(BOC)
{
   Debug::ft("PotsBocFeatureProfile.ctor");
}

//------------------------------------------------------------------------------

PotsBocFeatureProfile::~PotsBocFeatureProfile()
{
   Debug::ftnt("PotsBocFeatureProfile.dtor");
}
}
