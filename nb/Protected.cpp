//==============================================================================
//
//  Protected.cpp
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
#include "Protected.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Protected_ctor = "Protected.ctor";

Protected::Protected()
{
   Debug::ft(Protected_ctor);
}

//------------------------------------------------------------------------------

fn_name Protected_delete1 = "Protected.operator delete";

void Protected::operator delete(void* addr)
{
   Debug::ftnt(Protected_delete1);

   Memory::Free(addr, MemProtected);
}

//------------------------------------------------------------------------------

fn_name Protected_delete2 = "Protected.operator delete[]";

void Protected::operator delete[](void* addr)
{
   Debug::ftnt(Protected_delete2);

   Memory::Free(addr, MemProtected);
}

//------------------------------------------------------------------------------

fn_name Protected_delete3 = "Protected.operator delete(place)";

void Protected::operator delete(void* addr, void* place) noexcept
{
   Debug::ftnt(Protected_delete3);
}

//------------------------------------------------------------------------------

fn_name Protected_delete4 = "Protected.operator delete[](place)";

void Protected::operator delete[](void* addr, void* place) noexcept
{
   Debug::ftnt(Protected_delete4);
}

//------------------------------------------------------------------------------

fn_name Protected_new1 = "Protected.operator new";

void* Protected::operator new(size_t size)
{
   Debug::ft(Protected_new1);

   return Memory::Alloc(size, MemProtected);
}

//------------------------------------------------------------------------------

fn_name Protected_new2 = "Protected.operator new[]";

void* Protected::operator new[](size_t size)
{
   Debug::ft(Protected_new2);

   return Memory::Alloc(size, MemProtected);
}

//------------------------------------------------------------------------------

fn_name Protected_new3 = "Protected.operator new(place)";

void* Protected::operator new(size_t size, void* place)
{
   Debug::ft(Protected_new3);

   return place;
}

//------------------------------------------------------------------------------

fn_name Protected_new4 = "Protected.operator new[](place)";

void* Protected::operator new[](size_t size, void* place)
{
   Debug::ft(Protected_new4);

   return place;
}

//------------------------------------------------------------------------------

void Protected::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}