//==============================================================================
//
//  PotsProfileRegistry.cpp
//
//  Copyright (C) 2017  Greg Utas
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
#include "PotsProfileRegistry.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "PotsCircuit.h"
#include "PotsProfile.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsProfileRegistry_ctor = "PotsProfileRegistry.ctor";

PotsProfileRegistry::PotsProfileRegistry()
{
   Debug::ft(PotsProfileRegistry_ctor);

   auto max = Address::LastDN - Address::FirstDN + 1;
   profiles_.Init(max, PotsProfile::CellDiff(), MemProtected);
}

//------------------------------------------------------------------------------

fn_name PotsProfileRegistry_dtor = "PotsProfileRegistry.dtor";

PotsProfileRegistry::~PotsProfileRegistry()
{
   Debug::ft(PotsProfileRegistry_dtor);

   Debug::SwLog(PotsProfileRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name PotsProfileRegistry_BindProfile = "PotsProfileRegistry.BindProfile";

bool PotsProfileRegistry::BindProfile(PotsProfile& profile)
{
   Debug::ft(PotsProfileRegistry_BindProfile);

   return profiles_.Insert(profile);
}

//------------------------------------------------------------------------------

void PotsProfileRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "profiles [Address::DN]" << CRLF;
   profiles_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name PotsProfileRegistry_FirstProfile = "PotsProfileRegistry.FirstProfile";

PotsProfile* PotsProfileRegistry::FirstProfile(Address::DN dn) const
{
   Debug::ft(PotsProfileRegistry_FirstProfile);

   if(!PotsProfile::IsValidDN(dn)) return nullptr;

   id_t id = Address::DNToIndex(dn);

   return profiles_.First(id);
}

//------------------------------------------------------------------------------

fn_name PotsProfileRegistry_NextProfile = "PotsProfileRegistry.NextProfile";

PotsProfile* PotsProfileRegistry::NextProfile(const PotsProfile& profile) const
{
   Debug::ft(PotsProfileRegistry_NextProfile);

   auto dn = profile.GetDN();

   if(!PotsProfile::IsValidDN(dn)) return nullptr;

   id_t id = Address::DNToIndex(dn);

   return profiles_.Next(id);
}

//------------------------------------------------------------------------------

PotsProfile* PotsProfileRegistry::Profile(Address::DN dn) const
{
   if(!PotsProfile::IsValidDN(dn)) return nullptr;
   return profiles_.At(Address::DNToIndex(dn));
}

//------------------------------------------------------------------------------

fn_name PotsProfileRegistry_Shutdown = "PotsProfileRegistry.Shutdown";

void PotsProfileRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(PotsProfileRegistry_Shutdown);

   PotsCircuit::ResetStateCounts(level);

   for(auto p = profiles_.Last(); p != nullptr; profiles_.Prev(p))
   {
      p->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

fn_name PotsProfileRegistry_Startup = "PotsProfileRegistry.Startup";

void PotsProfileRegistry::Startup(RestartLevel level)
{
   Debug::ft(PotsProfileRegistry_Startup);

   for(auto p = profiles_.First(); p != nullptr; profiles_.Next(p))
   {
      p->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name PotsProfileRegistry_UnbindProfile = "PotsProfileRegistry.UnbindProfile";

void PotsProfileRegistry::UnbindProfile(PotsProfile& profile)
{
   Debug::ft(PotsProfileRegistry_UnbindProfile);

   profiles_.Erase(profile);
}
}
