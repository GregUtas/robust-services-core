//==============================================================================
//
//  PotsCxfFeature.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "PotsCxfFeature.h"
#include "PotsFeatureProfile.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "PotsFeatures.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCxfFeatureProfile : public PotsFeatureProfile
{
public:
   PotsCxfFeatureProfile();
   ~PotsCxfFeatureProfile();
};

//==============================================================================

fixed_string PotsCxfAbbrName = "cxf";
fixed_string PotsCxfFullName = "Call Transfer";

PotsCxfFeature::PotsCxfFeature() :
   PotsFeature(CXF, false, PotsCxfAbbrName, PotsCxfFullName)
{
   Debug::ft("PotsCxfFeature.ctor");

   SetIncompatible(BOC);
   SetIncompatible(HTL);
}

//------------------------------------------------------------------------------

PotsCxfFeature::~PotsCxfFeature()
{
   Debug::ftnt("PotsCxfFeature.dtor");
}

//------------------------------------------------------------------------------

CliText* PotsCxfFeature::Attrs() const
{
   return new CliText(PotsCxfFullName, PotsCxfAbbrName);
}

//------------------------------------------------------------------------------

PotsFeatureProfile* PotsCxfFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft("PotsCxfFeature.Subscribe");

   if(!cli.EndOfInput()) return nullptr;

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsCxfFeatureProfile;
}

//==============================================================================

PotsCxfFeatureProfile::PotsCxfFeatureProfile() : PotsFeatureProfile(CXF)
{
   Debug::ft("PotsCxfFeatureProfile.ctor");
}

//------------------------------------------------------------------------------

PotsCxfFeatureProfile::~PotsCxfFeatureProfile()
{
   Debug::ftnt("PotsCxfFeatureProfile.dtor");
}
}
