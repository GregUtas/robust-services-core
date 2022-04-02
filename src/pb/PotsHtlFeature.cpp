//==============================================================================
//
//  PotsHtlFeature.cpp
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
#include "PotsHtlFeature.h"
#include "CliText.h"
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "PotsCliParms.h"
#include "PotsFeatures.h"
#include "PotsProfileRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsHtlAttrs : public CliText
{
public: PotsHtlAttrs();
};

//------------------------------------------------------------------------------

fixed_string PotsHtlAbbrName = "htl";
fixed_string PotsHtlFullName = "Hot Line";

PotsHtlAttrs::PotsHtlAttrs() : CliText(PotsHtlFullName, PotsHtlAbbrName)
{
   BindParm(*new DnMandParm);
}

//==============================================================================

PotsHtlFeature::PotsHtlFeature() :
   PotsFeature(HTL, false, PotsHtlAbbrName, PotsHtlFullName)
{
   Debug::ft("PotsHtlFeature.ctor");

   SetIncompatible(BOC);
   SetIncompatible(WML);
   SetIncompatible(TWC);
   SetIncompatible(CXF);
}

//------------------------------------------------------------------------------

PotsHtlFeature::~PotsHtlFeature()
{
   Debug::ftnt("PotsHtlFeature.dtor");
}

//------------------------------------------------------------------------------

CliText* PotsHtlFeature::Attrs() const { return new PotsHtlAttrs; }

//------------------------------------------------------------------------------

PotsFeatureProfile* PotsHtlFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft("PotsHtlFeature.Subscribe");

   word dn;

   if(!cli.Command()->GetIntParm(dn, cli)) return nullptr;
   if(!cli.EndOfInput()) return nullptr;

   if(Singleton< PotsProfileRegistry >::Instance()->Profile(dn) == nullptr)
      *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsHtlFeatureProfile(dn);
}

//==============================================================================

PotsHtlFeatureProfile::PotsHtlFeatureProfile(Address::DN dn) :
   PotsFeatureProfile(HTL),
   dn_(dn)
{
   Debug::ft("PotsHtlFeatureProfile.ctor");
}

//------------------------------------------------------------------------------

PotsHtlFeatureProfile::~PotsHtlFeatureProfile()
{
   Debug::ftnt("PotsHtlFeatureProfile.dtor");
}

//------------------------------------------------------------------------------

void PotsHtlFeatureProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PotsFeatureProfile::Display(stream, prefix, options);

   stream << prefix << "dn : " << dn_ << CRLF;
}
}
