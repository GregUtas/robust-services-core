//==============================================================================
//
//  CliCookie.cpp
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
#include "CliCookie.h"
#include <cstddef>
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CliCookie_ctor = "CliCookie.ctor";

CliCookie::CliCookie()
{
   Debug::ft(CliCookie_ctor);
}

//------------------------------------------------------------------------------

fn_name CliCookie_dtor = "CliCookie.dtor";

CliCookie::~CliCookie()
{
   Debug::ftnt(CliCookie_dtor);
}

//------------------------------------------------------------------------------

fn_name CliCookie_Advance = "CliCookie.Advance";

void CliCookie::Advance()
{
   Debug::ft(CliCookie_Advance);

   //  Advance to the next parameter at the current level in the tree.
   //
   ++index_.back();
}

//------------------------------------------------------------------------------

fn_name CliCookie_Ascend = "CliCookie.Ascend";

void CliCookie::Ascend()
{
   Debug::ft(CliCookie_Ascend);

   //  There are no more parameters at the current level, so back up
   //  and look for the next parameter at the previous level.
   //
   index_.pop_back();
   ++index_.back();
}

//------------------------------------------------------------------------------

fn_name CliCookie_Descend1 = "CliCookie.Descend";

void CliCookie::Descend()
{
   Debug::ft(CliCookie_Descend1);

   //  Look for the first parameter at the next level.
   //
   index_.push_back(1);
}

//------------------------------------------------------------------------------

fn_name CliCookie_Descend2 = "CliCookie.Descend(index)";

void CliCookie::Descend(uint32_t index)
{
   Debug::ft(CliCookie_Descend2);

   //  Record INDEX as the offset where a parameter was found at the next
   //  level, and then look for the first parameter at the subsequent level.
   //
   index_.push_back(index);
   index_.push_back(1);
}

//------------------------------------------------------------------------------

void CliCookie::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "index : { ";

   auto last = index_.size() - 1;

   for(size_t i = 0; i <= last; ++i)
   {
      stream << index_.at(i);
      if(i != last) stream << ", ";
   }

   stream << " }" << CRLF;
}

//------------------------------------------------------------------------------

uint32_t CliCookie::Index(uint32_t depth) const
{
   if(depth < index_.size()) return index_.at(depth);
   return 0;
}

//------------------------------------------------------------------------------

fn_name CliCookie_Initialize = "CliCookie.Initialize";

void CliCookie::Initialize()
{
   Debug::ft(CliCookie_Initialize);

   //  Look for the first parameter at level 0.
   //
   index_.clear();
   index_.push_back(1);
}

//------------------------------------------------------------------------------

void CliCookie::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
