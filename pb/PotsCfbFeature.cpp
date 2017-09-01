//==============================================================================
//
//  PotsCfbFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsCfbFeature.h"
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "PotsCliParms.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfbAttrs : public CliText
{
public: PotsCfbAttrs();
};

//==============================================================================

fixed_string PotsCfbAbbrName = "cfb";
fixed_string PotsCfbFullName = "Call Forwarding Busy";

PotsCfbAttrs::PotsCfbAttrs() : CliText(PotsCfbFullName, PotsCfbAbbrName)
{
   BindParm(*new DnOptParm);
}

//------------------------------------------------------------------------------

fn_name PotsCfbFeature_ctor = "PotsCfbFeature.ctor";

PotsCfbFeature::PotsCfbFeature() :
   PotsFeature(CFB, true, PotsCfbAbbrName, PotsCfbFullName)
{
   Debug::ft(PotsCfbFeature_ctor);

   SetIncompatible(BOC);
   SetIncompatible(BIC);
}

//------------------------------------------------------------------------------

fn_name PotsCfbFeature_dtor = "PotsCfbFeature.dtor";

PotsCfbFeature::~PotsCfbFeature()
{
   Debug::ft(PotsCfbFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsCfbFeature::Attrs() const { return new PotsCfbAttrs; }

//------------------------------------------------------------------------------

fn_name PotsCfbFeature_Subscribe = "PotsCfbFeature.Subscribe";

PotsFeatureProfile* PotsCfbFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsCfbFeature_Subscribe);

   word dn;

   if(cli.Command()->GetIntParmRc(dn, cli) == CliParm::Ok)
   {
      cli.EndOfInput(false);
      auto reg = Singleton< PotsProfileRegistry >::Instance();

      if(reg->Profile(dn) == nullptr)
      {
         *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;
      }
   }
   else
   {
      dn = PotsProfile::NilDN;
      *cli.obuf << spaces(2) << NoDestinationWarning << CRLF;
   }

   return new PotsCfbFeatureProfile(dn);
}

//==============================================================================

fn_name PotsCfbFeatureProfile_ctor = "PotsCfbFeatureProfile.ctor";

PotsCfbFeatureProfile::PotsCfbFeatureProfile(Address::DN dn) :
   DnRouteFeatureProfile(CFB, dn)
{
   Debug::ft(PotsCfbFeatureProfile_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCfbFeatureProfile_dtor = "PotsCfbFeatureProfile.dtor";

PotsCfbFeatureProfile::~PotsCfbFeatureProfile()
{
   Debug::ft(PotsCfbFeatureProfile_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfbFeatureProfile_Activate = "PotsCfbFeatureProfile.Activate";

bool PotsCfbFeatureProfile::Activate(PotsProfile& profile, CliThread& cli)
{
   Debug::ft(PotsCfbFeatureProfile_Activate);

   if(DnRouteFeatureProfile::Activate(profile, cli))
   {
      cli.EndOfInput(false);
      auto reg = Singleton< PotsProfileRegistry >::Instance();

      if(reg->Profile(GetDN()) == nullptr)
      {
         *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;
      }

      return true;
   }

   return false;
}
}
