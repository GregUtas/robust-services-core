//==============================================================================
//
//  PotsFeatures.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "PotsFeatures.h"
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "PotsCliParms.h"
#include "PotsProfile.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
DnRouteFeatureProfile::DnRouteFeatureProfile
   (PotsFeature::Id fid, Address::DN dn) :
   PotsFeatureProfile(fid),
   dn_(dn),
   on_(PotsProfile::IsValidDN(dn))
{
   Debug::ft("DnRouteFeatureProfile.ctor");
}

//------------------------------------------------------------------------------

DnRouteFeatureProfile::~DnRouteFeatureProfile()
{
   Debug::ftnt("DnRouteFeatureProfile.dtor");
}

//------------------------------------------------------------------------------

bool DnRouteFeatureProfile::Activate(const PotsProfile& profile, CliThread& cli)
{
   Debug::ft("DnRouteFeatureProfile.Activate");

   FunctionGuard guard(Guard_MemUnprotect);

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

//------------------------------------------------------------------------------

bool DnRouteFeatureProfile::Deactivate(PotsProfile& profile)
{
   Debug::ft("DnRouteFeatureProfile.Deactivate");

   FunctionGuard guard(Guard_MemUnprotect);
   on_ = false;
   return true;
}

//------------------------------------------------------------------------------

void DnRouteFeatureProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PotsFeatureProfile::Display(stream, prefix, options);

   stream << prefix << "dn : " << dn_ << CRLF;
   stream << prefix << "on : " << on_ << CRLF;
}

//------------------------------------------------------------------------------

void DnRouteFeatureProfile::SetActive(bool on)
{
   FunctionGuard guard(Guard_MemUnprotect);
   on_ = on;
}

//------------------------------------------------------------------------------

void DnRouteFeatureProfile::SetDN(Address::DN dn)
{
   FunctionGuard guard(Guard_MemUnprotect);
   dn_ = dn;
}
}
