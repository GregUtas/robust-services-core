//==============================================================================
//
//  PotsTwcFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsTwcFeature.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "PotsFeatureProfile.h"
#include "PotsFeatures.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
//p TWC/CXF Design:
//  o bind Initiator against local answer and remote alerting/answer SNP to
//    report flash
//  o need to support flash + access code (recall dial tone)
//  o only 3WC subscribed: flash 1 conferences; flash 2 drops add-on; onhook
//    before conference recalls; onhook after conference releases both
//  o only CXF subscribed: flash flipflops; onhook transfers or recalls
//  o allow conferencing and transferring during alerting
//  o block conferencing and transferring treatments: flash is ignored;
//    onhook causes recall
//  o CXF needs proxy in some OBC (SC, OA) and all XBC states (AC, RS, LS)
//  o CXF on original call must not relay SUS/RES to UPSM
//  o CXF on consultation call must not relay EOS/ALT/ANM to UPSM
//  o CXF on consultation call must not relay SUS/RES to UPSM unless UPSM is
//    in AnmSent state (its initial state when the original call is a TBC)
//
class PotsTwcFeatureProfile : public PotsFeatureProfile
{
public:
   PotsTwcFeatureProfile();
   ~PotsTwcFeatureProfile();
};

class PotsTwcAttrs : public CliText
{
public: PotsTwcAttrs();
};

fixed_string PotsTwcAbbrName = "twc";
fixed_string PotsTwcFullName = "Three-Way Calling";

PotsTwcAttrs::PotsTwcAttrs() : CliText(PotsTwcFullName, PotsTwcAbbrName) { }

fn_name PotsTwcFeature_ctor = "PotsTwcFeature.ctor";

PotsTwcFeature::PotsTwcFeature() :
   PotsFeature(TWC, false, PotsTwcAbbrName, PotsTwcFullName)
{
   Debug::ft(PotsTwcFeature_ctor);

   SetIncompatible(BOC);
   SetIncompatible(HTL);
}

fn_name PotsTwcFeature_dtor = "PotsTwcFeature.dtor";

PotsTwcFeature::~PotsTwcFeature()
{
   Debug::ft(PotsTwcFeature_dtor);
}

CliText* PotsTwcFeature::Attrs() const { return new PotsTwcAttrs; }

fn_name PotsTwcFeature_Subscribe = "PotsTwcFeature.Subscribe";

PotsFeatureProfile* PotsTwcFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsTwcFeature_Subscribe);

   cli.EndOfInput(false);
   return new PotsTwcFeatureProfile;
}

fn_name PotsTwcFeatureProfile_ctor = "PotsTwcFeatureProfile.ctor";

PotsTwcFeatureProfile::PotsTwcFeatureProfile() : PotsFeatureProfile(TWC)
{
   Debug::ft(PotsTwcFeatureProfile_ctor);
}

fn_name PotsTwcFeatureProfile_dtor = "PotsTwcFeatureProfile.dtor";

PotsTwcFeatureProfile::~PotsTwcFeatureProfile()
{
   Debug::ft(PotsTwcFeatureProfile_dtor);
}
}