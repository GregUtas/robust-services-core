//==============================================================================
//
//  PotsCxfFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsCxfFeature.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatureProfile.h"
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

class PotsCxfAttrs : public CliText
{
public: PotsCxfAttrs();
};

fixed_string PotsCxfAbbrName = "cxf";
fixed_string PotsCxfFullName = "Call Transfer";

PotsCxfAttrs::PotsCxfAttrs() : CliText(PotsCxfFullName, PotsCxfAbbrName) { }

fn_name PotsCxfFeature_ctor = "PotsCxfFeature.ctor";

PotsCxfFeature::PotsCxfFeature() :
   PotsFeature(CXF, false, PotsCxfAbbrName, PotsCxfFullName)
{
   Debug::ft(PotsCxfFeature_ctor);

   SetIncompatible(BOC);
   SetIncompatible(HTL);
}

fn_name PotsCxfFeature_dtor = "PotsCxfFeature.dtor";

PotsCxfFeature::~PotsCxfFeature()
{
   Debug::ft(PotsCxfFeature_dtor);
}

CliText* PotsCxfFeature::Attrs() const { return new PotsCxfAttrs; }

fn_name PotsCxfFeature_Subscribe = "PotsCxfFeature.Subscribe";

PotsFeatureProfile* PotsCxfFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsCxfFeature_Subscribe);

   cli.EndOfInput(false);
   return new PotsCxfFeatureProfile;
}

fn_name PotsCxfFeatureProfile_ctor = "PotsCxfFeatureProfile.ctor";

PotsCxfFeatureProfile::PotsCxfFeatureProfile() : PotsFeatureProfile(CXF)
{
   Debug::ft(PotsCxfFeatureProfile_ctor);
}

fn_name PotsCxfFeatureProfile_dtor = "PotsCxfFeatureProfile.dtor";

PotsCxfFeatureProfile::~PotsCxfFeatureProfile()
{
   Debug::ft(PotsCxfFeatureProfile_dtor);
}
}