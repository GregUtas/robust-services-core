//==============================================================================
//
//  PotsBocFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsBocFeature.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatureProfile.h"
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