//==============================================================================
//
//  PotsCwtFeature.cpp
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
#include "PotsCwtFeature.h"
#include "CliText.h"
#include "PotsFeatureProfile.h"
#include "CliThread.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "PotsFeatures.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCwtFeatureProfile : public PotsFeatureProfile
{
public:
   PotsCwtFeatureProfile();
   ~PotsCwtFeatureProfile();
};

class PotsCwtAttrs : public CliText
{
public: PotsCwtAttrs();
};

//------------------------------------------------------------------------------

fixed_string PotsCwtAbbrName = "cwt";
fixed_string PotsCwtFullName = "Call Waiting";

PotsCwtAttrs::PotsCwtAttrs() : CliText(PotsCwtFullName, PotsCwtAbbrName) { }

//==============================================================================

fn_name PotsCwtFeature_ctor = "PotsCwtFeature.ctor";

PotsCwtFeature::PotsCwtFeature() :
   PotsFeature(CWT, false, PotsCwtAbbrName, PotsCwtFullName)
{
   Debug::ft(PotsCwtFeature_ctor);

   SetIncompatible(BIC);
}

//------------------------------------------------------------------------------

fn_name PotsCwtFeature_dtor = "PotsCwtFeature.dtor";

PotsCwtFeature::~PotsCwtFeature()
{
   Debug::ftnt(PotsCwtFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsCwtFeature::Attrs() const { return new PotsCwtAttrs; }

//------------------------------------------------------------------------------

fn_name PotsCwtFeature_Subscribe = "PotsCwtFeature.Subscribe";

PotsFeatureProfile* PotsCwtFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsCwtFeature_Subscribe);

   if(!cli.EndOfInput()) return nullptr;

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsCwtFeatureProfile;
}

//==============================================================================

fn_name PotsCwtFeatureProfile_ctor = "PotsCwtFeatureProfile.ctor";

PotsCwtFeatureProfile::PotsCwtFeatureProfile() : PotsFeatureProfile(CWT)
{
   Debug::ft(PotsCwtFeatureProfile_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtFeatureProfile_dtor = "PotsCwtFeatureProfile.dtor";

PotsCwtFeatureProfile::~PotsCwtFeatureProfile()
{
   Debug::ftnt(PotsCwtFeatureProfile_dtor);
}
}
