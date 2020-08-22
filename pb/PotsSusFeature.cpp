//==============================================================================
//
//  PotsSusFeature.cpp
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
#include "PotsSusFeature.h"
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
class PotsSusFeatureProfile : public PotsFeatureProfile
{
public:
   PotsSusFeatureProfile();
   ~PotsSusFeatureProfile();
};

class PotsSusAttrs : public CliText
{
public: PotsSusAttrs();
};

//------------------------------------------------------------------------------

fixed_string PotsSusAbbrName = "sus";
fixed_string PotsSusFullName = "Suspended Service";

PotsSusAttrs::PotsSusAttrs() : CliText(PotsSusFullName, PotsSusAbbrName) { }

//==============================================================================

fn_name PotsSusFeature_ctor = "PotsSusFeature.ctor";

PotsSusFeature::PotsSusFeature() :
   PotsFeature(SUS, false, PotsSusAbbrName, PotsSusFullName)
{
   Debug::ft(PotsSusFeature_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsSusFeature_dtor = "PotsSusFeature.dtor";

PotsSusFeature::~PotsSusFeature()
{
   Debug::ftnt(PotsSusFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsSusFeature::Attrs() const { return new PotsSusAttrs; }

//------------------------------------------------------------------------------

fn_name PotsSusFeature_Subscribe = "PotsSusFeature.Subscribe";

PotsFeatureProfile* PotsSusFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsSusFeature_Subscribe);

   if(!cli.EndOfInput()) return nullptr;

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsSusFeatureProfile;
}

//==============================================================================

fn_name PotsSusFeatureProfile_ctor = "PotsSusFeatureProfile.ctor";

PotsSusFeatureProfile::PotsSusFeatureProfile() : PotsFeatureProfile(SUS)
{
   Debug::ft(PotsSusFeatureProfile_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsSusFeatureProfile_dtor = "PotsSusFeatureProfile.dtor";

PotsSusFeatureProfile::~PotsSusFeatureProfile()
{
   Debug::ftnt(PotsSusFeatureProfile_dtor);
}
}
