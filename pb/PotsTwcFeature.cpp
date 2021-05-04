//==============================================================================
//
//  PotsTwcFeature.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "PotsTwcFeature.h"
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

//==============================================================================

fixed_string PotsTwcAbbrName = "twc";
fixed_string PotsTwcFullName = "Three-Way Calling";

PotsTwcFeature::PotsTwcFeature() :
   PotsFeature(TWC, false, PotsTwcAbbrName, PotsTwcFullName)
{
   Debug::ft("PotsTwcFeature.ctor");

   SetIncompatible(BOC);
   SetIncompatible(HTL);
}

//------------------------------------------------------------------------------

PotsTwcFeature::~PotsTwcFeature()
{
   Debug::ftnt("PotsTwcFeature.dtor");
}

//------------------------------------------------------------------------------

CliText* PotsTwcFeature::Attrs() const
{
   return new CliText(PotsTwcFullName, PotsTwcAbbrName);
}

//------------------------------------------------------------------------------

PotsFeatureProfile* PotsTwcFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft("PotsTwcFeature.Subscribe");

   if(!cli.EndOfInput()) return nullptr;

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsTwcFeatureProfile;
}

//==============================================================================

PotsTwcFeatureProfile::PotsTwcFeatureProfile() : PotsFeatureProfile(TWC)
{
   Debug::ft("PotsTwcFeatureProfile.ctor");
}

//------------------------------------------------------------------------------

PotsTwcFeatureProfile::~PotsTwcFeatureProfile()
{
   Debug::ftnt("PotsTwcFeatureProfile.dtor");
}
}
