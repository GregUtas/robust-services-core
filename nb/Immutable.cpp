//==============================================================================
//
//  Immutable.cpp
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
#include "Immutable.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Immutable_ctor = "Immutable.ctor";

Immutable::Immutable()
{
   Debug::ft(Immutable_ctor);
}

//------------------------------------------------------------------------------

fn_name Immutable_new1 = "Immutable.operator new";

void* Immutable::operator new(size_t size)
{
   Debug::ft(Immutable_new1);

   return Memory::Alloc(size, MemImm);
}

//------------------------------------------------------------------------------

fn_name Immutable_new2 = "Immutable.operator new[]";

void* Immutable::operator new[](size_t size)
{
   Debug::ft(Immutable_new2);

   return Memory::Alloc(size, MemImm);
}

//------------------------------------------------------------------------------

void Immutable::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}