//==============================================================================
//
//  PotsCxfFeature.cpp
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
#include "PotsCxfFeature.h"
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
class PotsCxfFeatureProfile : public PotsFeatureProfile
{
public:
   PotsCxfFeatureProfile();
   ~PotsCxfFeatureProfile();
};

//------------------------------------------------------------------------------

class PotsCxfAttrs : public CliText
{
public: PotsCxfAttrs();
};

fixed_string PotsCxfAbbrName = "cxf";
fixed_string PotsCxfFullName = "Call Transfer";

PotsCxfAttrs::PotsCxfAttrs() : CliText(PotsCxfFullName, PotsCxfAbbrName) { }

//==============================================================================

fn_name PotsCxfFeature_ctor = "PotsCxfFeature.ctor";

PotsCxfFeature::PotsCxfFeature() :
   PotsFeature(CXF, false, PotsCxfAbbrName, PotsCxfFullName)
{
   Debug::ft(PotsCxfFeature_ctor);

   SetIncompatible(BOC);
   SetIncompatible(HTL);
}

//------------------------------------------------------------------------------

fn_name PotsCxfFeature_dtor = "PotsCxfFeature.dtor";

PotsCxfFeature::~PotsCxfFeature()
{
   Debug::ft(PotsCxfFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsCxfFeature::Attrs() const { return new PotsCxfAttrs; }

//------------------------------------------------------------------------------

fn_name PotsCxfFeature_Subscribe = "PotsCxfFeature.Subscribe";

PotsFeatureProfile* PotsCxfFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsCxfFeature_Subscribe);

   cli.EndOfInput(false);

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsCxfFeatureProfile;
}

//==============================================================================

fn_name PotsCxfFeatureProfile_ctor = "PotsCxfFeatureProfile.ctor";

PotsCxfFeatureProfile::PotsCxfFeatureProfile() : PotsFeatureProfile(CXF)
{
   Debug::ft(PotsCxfFeatureProfile_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCxfFeatureProfile_dtor = "PotsCxfFeatureProfile.dtor";

PotsCxfFeatureProfile::~PotsCxfFeatureProfile()
{
   Debug::ft(PotsCxfFeatureProfile_dtor);
}
}
