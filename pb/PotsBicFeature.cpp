//==============================================================================
//
//  PotsBicFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsBicFeature.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatureProfile.h"
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
   Debug::ft(PotsBicFeatureProfile_dtor);
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
   Debug::ft(PotsBicFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsBicFeature::Attrs() const { return new PotsBicAttrs; }

//------------------------------------------------------------------------------

fn_name PotsBicFeature_Subscribe = "PotsBicFeature.Subscribe";

PotsFeatureProfile* PotsBicFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsBicFeature_Subscribe);

   cli.EndOfInput(false);
   return new PotsBicFeatureProfile;
}
}