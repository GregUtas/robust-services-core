//==============================================================================
//
//  PotsFeatures.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsFeatures.h"
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "PotsCliParms.h"
#include "PotsProfile.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name DnRouteFeatureProfile_ctor = "DnRouteFeatureProfile.ctor";

DnRouteFeatureProfile::DnRouteFeatureProfile
   (PotsFeature::Id fid, Address::DN dn) :
   PotsFeatureProfile(fid),
   dn_(dn),
   on_(PotsProfile::IsValidDN(dn))
{
   Debug::ft(DnRouteFeatureProfile_ctor);
}

fn_name DnRouteFeatureProfile_dtor = "DnRouteFeatureProfile.dtor";

DnRouteFeatureProfile::~DnRouteFeatureProfile()
{
   Debug::ft(DnRouteFeatureProfile_dtor);
}

fn_name DnRouteFeatureProfile_Activate = "DnRouteFeatureProfile.Activate";

bool DnRouteFeatureProfile::Activate(PotsProfile& profile, CliThread& cli)
{
   Debug::ft(DnRouteFeatureProfile_Activate);

   word dn;

   if(cli.Command()->GetIntParmRc(dn, cli) == CliParm::Ok)
   {
      dn_ = dn;
   }
   else if(!PotsProfile::IsValidDN(dn_))
   {
      *cli.obuf << spaces(2) << InvalidDestination << CRLF;
      return false;
   }

   on_ = true;
   return true;
}

fn_name DnRouteFeatureProfile_Deactivate = "DnRouteFeatureProfile.Deactivate";

bool DnRouteFeatureProfile::Deactivate(PotsProfile& profile)
{
   Debug::ft(DnRouteFeatureProfile_Deactivate);

   on_ = false;
   return true;
}

void DnRouteFeatureProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PotsFeatureProfile::Display(stream, prefix, options);

   stream << prefix << "dn : " << dn_ << CRLF;
   stream << prefix << "on : " << on_ << CRLF;
}
}
