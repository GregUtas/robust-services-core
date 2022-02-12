//==============================================================================
//
//  CliRegistry.cpp
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
#include "CliRegistry.h"
#include <cstddef>
#include <ostream>
#include "CliIncrement.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum number of increments that can register.
//
constexpr size_t MaxIncrements = 30;

//------------------------------------------------------------------------------

CliRegistry::CliRegistry()
{
   Debug::ft("CliRegistry.ctor");

   increments_.Init(MaxIncrements, CliIncrement::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name CliRegistry_dtor = "CliRegistry.dtor";

CliRegistry::~CliRegistry()
{
   Debug::ftnt(CliRegistry_dtor);

   Debug::SwLog(CliRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool CliRegistry::BindIncrement(CliIncrement& incr)
{
   Debug::ft("CliRegistry.BindIncrement");

   return increments_.Insert(incr);
}

//------------------------------------------------------------------------------

void CliRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "increments : " << CRLF;
   increments_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

CliIncrement* CliRegistry::FindIncrement(const string& name) const
{
   Debug::ft("CliRegistry.FindIncrement");

   //  Look for the increment that is known by NAME.
   //
   for(auto i = increments_.First(); i != nullptr; increments_.Next(i))
   {
      if(i->Name() == name) return i;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CliRegistry::ListIncrements(ostream& stream) const
{
   Debug::ft("CliRegistry.ListIncrements");

   //  Output a brief description of each increment.
   //
   for(auto i = increments_.First(); i != nullptr; increments_.Next(i))
   {
      i->Explain(stream, 0);
   }
}

//------------------------------------------------------------------------------

void CliRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void CliRegistry::UnbindIncrement(CliIncrement& incr)
{
   Debug::ftnt("CliRegistry.UnbindIncrement");

   increments_.Erase(incr);
}
}
