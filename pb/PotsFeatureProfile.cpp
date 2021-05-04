//==============================================================================
//
//  PotsFeatureProfile.cpp
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
#include "PotsFeatureProfile.h"
#include <cstdint>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "PotsFeatureRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsFeatureProfile::PotsFeatureProfile(PotsFeature::Id fid) : fid_(fid)
{
   Debug::ft("PotsFeatureProfile.ctor");
}

//------------------------------------------------------------------------------

PotsFeatureProfile::~PotsFeatureProfile()
{
   Debug::ftnt("PotsFeatureProfile.dtor");
}

//------------------------------------------------------------------------------

fn_name PotsFeatureProfile_Activate = "PotsFeatureProfile.Activate";

bool PotsFeatureProfile::Activate(PotsProfile& profile, CliThread& cli)
{
   Debug::ft(PotsFeatureProfile_Activate);

   Debug::SwLog(PotsFeatureProfile_Activate, strOver(this), fid_);
   return false;
}

//------------------------------------------------------------------------------

fn_name PotsFeatureProfile_Deactivate = "PotsFeatureProfile.Deactivate";

bool PotsFeatureProfile::Deactivate(PotsProfile& profile)
{
   Debug::ft(PotsFeatureProfile_Deactivate);

   Debug::SwLog(PotsFeatureProfile_Deactivate, strOver(this), fid_);
   return false;
}

//------------------------------------------------------------------------------

void PotsFeatureProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   auto ftr = Singleton< PotsFeatureRegistry >::Instance()->Feature(fid_);

   stream << prefix << "fid  : " << int(fid_);
   stream << " (" << ftr->AbbrName() << ')' << CRLF;
   stream << prefix << "link : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

ptrdiff_t PotsFeatureProfile::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const PotsFeatureProfile* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

bool PotsFeatureProfile::Unsubscribe(PotsProfile& profile)
{
   Debug::ft("PotsFeatureProfile.Unsubscribe");

   return true;
}
}
