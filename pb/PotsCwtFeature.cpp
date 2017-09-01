//==============================================================================
//
//  PotsCwtFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsCwtFeature.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatureProfile.h"
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

//==============================================================================

fixed_string PotsCwtAbbrName = "cwt";
fixed_string PotsCwtFullName = "Call Waiting";

PotsCwtAttrs::PotsCwtAttrs() : CliText(PotsCwtFullName, PotsCwtAbbrName) { }

//------------------------------------------------------------------------------

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
   Debug::ft(PotsCwtFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsCwtFeature::Attrs() const { return new PotsCwtAttrs; }

//------------------------------------------------------------------------------

fn_name PotsCwtFeature_Subscribe = "PotsCwtFeature.Subscribe";

PotsFeatureProfile* PotsCwtFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsCwtFeature_Subscribe);

   cli.EndOfInput(false);
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
   Debug::ft(PotsCwtFeatureProfile_dtor);
}
}