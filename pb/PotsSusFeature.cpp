//==============================================================================
//
//  PotsSusFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsSusFeature.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatureProfile.h"
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

//==============================================================================

fixed_string PotsSusAbbrName = "sus";
fixed_string PotsSusFullName = "Suspended Service";

PotsSusAttrs::PotsSusAttrs() : CliText(PotsSusFullName, PotsSusAbbrName) { }

//------------------------------------------------------------------------------

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
   Debug::ft(PotsSusFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsSusFeature::Attrs() const { return new PotsSusAttrs; }

//------------------------------------------------------------------------------

fn_name PotsSusFeature_Subscribe = "PotsSusFeature.Subscribe";

PotsFeatureProfile* PotsSusFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsSusFeature_Subscribe);

   cli.EndOfInput(false);
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
   Debug::ft(PotsSusFeatureProfile_dtor);
}
}