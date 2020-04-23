//==============================================================================
//
//  Persistent.cpp
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
#include "Persistent.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Persistent_ctor = "Persistent.ctor";

Persistent::Persistent()
{
   Debug::ft(Persistent_ctor);
}

//------------------------------------------------------------------------------

fn_name Persistent_delete1 = "Persistent.operator delete";

void Persistent::operator delete(void* addr)
{
   Debug::ft(Persistent_delete1);

   Memory::Free(addr, MemPersistent);
}

//------------------------------------------------------------------------------

fn_name Persistent_delete2 = "Persistent.operator delete[]";

void Persistent::operator delete[](void* addr)
{
   Debug::ft(Persistent_delete2);

   Memory::Free(addr, MemPersistent);
}

//------------------------------------------------------------------------------

fn_name Persistent_new1 = "Persistent.operator new";

void* Persistent::operator new(size_t size)
{
   Debug::ft(Persistent_new1);

   return Memory::Alloc(size, MemPersistent);
}

//------------------------------------------------------------------------------

fn_name Persistent_new2 = "Persistent.operator new[]";

void* Persistent::operator new[](size_t size)
{
   Debug::ft(Persistent_new2);

   return Memory::Alloc(size, MemPersistent);
}

//------------------------------------------------------------------------------

void Persistent::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}