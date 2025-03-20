//==============================================================================
//
//  PotsCfbFeature.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "PotsCfbFeature.h"
#include "CliText.h"
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
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

//------------------------------------------------------------------------------

fixed_string PotsCfbAbbrName = "cfb";
fixed_string PotsCfbFullName = "Call Forwarding Busy";

PotsCfbAttrs::PotsCfbAttrs() : CliText(PotsCfbFullName, PotsCfbAbbrName)
{
   BindParm(*new DnOptParm);
}

//==============================================================================

PotsCfbFeature::PotsCfbFeature() :
   PotsFeature(CFB, true, PotsCfbAbbrName, PotsCfbFullName)
{
   Debug::ft("PotsCfbFeature.ctor");

   SetIncompatible(BOC);
   SetIncompatible(BIC);
}

//------------------------------------------------------------------------------

PotsCfbFeature::~PotsCfbFeature()
{
   Debug::ftnt("PotsCfbFeature.dtor");
}

//------------------------------------------------------------------------------

CliText* PotsCfbFeature::Attrs() const { return new PotsCfbAttrs; }

//------------------------------------------------------------------------------

PotsFeatureProfile* PotsCfbFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft("PotsCfbFeature.Subscribe");

   word dn;

   if(cli.Command()->GetIntParmRc(dn, cli) == CliParm::Ok)
   {
      if(!cli.EndOfInput()) return nullptr;
      auto reg = Singleton<PotsProfileRegistry>::Instance();

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

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsCfbFeatureProfile(dn);
}

//==============================================================================

PotsCfbFeatureProfile::PotsCfbFeatureProfile(Address::DN dn) :
   DnRouteFeatureProfile(CFB, dn)
{
   Debug::ft("PotsCfbFeatureProfile.ctor");
}

//------------------------------------------------------------------------------

PotsCfbFeatureProfile::~PotsCfbFeatureProfile()
{
   Debug::ftnt("PotsCfbFeatureProfile.dtor");
}

//------------------------------------------------------------------------------

bool PotsCfbFeatureProfile::Activate(const PotsProfile& profile, CliThread& cli)
{
   Debug::ft("PotsCfbFeatureProfile.Activate");

   if(DnRouteFeatureProfile::Activate(profile, cli))
   {
      if(!cli.EndOfInput()) return false;
      auto reg = Singleton<PotsProfileRegistry>::Instance();

      if(reg->Profile(GetDN()) == nullptr)
      {
         *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;
      }

      return true;
   }

   return false;
}
}
