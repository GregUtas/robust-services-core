//==============================================================================
//
//  PotsCfuFeature.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "PotsCfuFeature.h"
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
class PotsCfuAttrs : public CliText
{
public: PotsCfuAttrs();
};

//------------------------------------------------------------------------------

fixed_string PotsCfuAbbrName = "cfu";
fixed_string PotsCfuFullName = "Call Forwarding Unconditional";

PotsCfuAttrs::PotsCfuAttrs() : CliText(PotsCfuFullName, PotsCfuAbbrName)
{
   BindParm(*new DnOptParm);
}

//==============================================================================

PotsCfuFeature::PotsCfuFeature() :
   PotsFeature(CFU, true, PotsCfuAbbrName, PotsCfuFullName)
{
   Debug::ft("PotsCfuFeature.ctor");

   SetIncompatible(BOC);
   SetIncompatible(BIC);
}

//------------------------------------------------------------------------------

PotsCfuFeature::~PotsCfuFeature()
{
   Debug::ftnt("PotsCfuFeature.dtor");
}

//------------------------------------------------------------------------------

CliText* PotsCfuFeature::Attrs() const { return new PotsCfuAttrs; }

//------------------------------------------------------------------------------

PotsFeatureProfile* PotsCfuFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft("PotsCfuFeature.Subscribe");

   word dn;

   if(cli.Command()->GetIntParmRc(dn, cli) == CliParm::Ok)
   {
      if(!cli.EndOfInput()) return nullptr;
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

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsCfuFeatureProfile(dn);
}

//==============================================================================

PotsCfuFeatureProfile::PotsCfuFeatureProfile(Address::DN dn) :
   DnRouteFeatureProfile(CFU, dn)
{
   Debug::ft("PotsCfuFeatureProfile.ctor");

   SetActive(false);
}

//------------------------------------------------------------------------------

PotsCfuFeatureProfile::~PotsCfuFeatureProfile()
{
   Debug::ftnt("PotsCfuFeatureProfile.dtor");
}

//------------------------------------------------------------------------------

bool PotsCfuFeatureProfile::Activate(PotsProfile& profile, CliThread& cli)
{
   Debug::ft("PotsCfuFeatureProfile.Activate");

   if(DnRouteFeatureProfile::Activate(profile, cli))
   {
      if(!cli.EndOfInput()) return false;
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
