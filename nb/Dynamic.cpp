//==============================================================================
//
//  Dynamic.cpp
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
#include "Dynamic.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Dynamic_ctor = "Dynamic.ctor";

Dynamic::Dynamic()
{
   Debug::ft(Dynamic_ctor);
}

//------------------------------------------------------------------------------

fn_name Dynamic_delete1 = "Dynamic.operator delete";

void Dynamic::operator delete(void* addr)
{
   Debug::ft(Dynamic_delete1);

   Memory::Free(addr, MemDynamic);
}

//------------------------------------------------------------------------------

fn_name Dynamic_delete2 = "Dynamic.operator delete[]";

void Dynamic::operator delete[](void* addr)
{
   Debug::ft(Dynamic_delete2);

   Memory::Free(addr, MemDynamic);
}

//------------------------------------------------------------------------------

fn_name Dynamic_new1 = "Dynamic.operator new";

void* Dynamic::operator new(size_t size)
{
   Debug::ft(Dynamic_new1);

   return Memory::Alloc(size, MemDynamic);
}

//------------------------------------------------------------------------------

fn_name Dynamic_new2 = "Dynamic.operator new[]";

void* Dynamic::operator new[](size_t size)
{
   Debug::ft(Dynamic_new2);

   return Memory::Alloc(size, MemDynamic);
}

//------------------------------------------------------------------------------

void Dynamic::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}