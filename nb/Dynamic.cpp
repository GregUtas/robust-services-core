//==============================================================================
//
//  Dynamic.cpp
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
#include "Dynamic.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
Dynamic::Dynamic()
{
   Debug::ft("Dynamic.ctor");
}

//------------------------------------------------------------------------------

void Dynamic::operator delete(void* addr)
{
   Debug::ftnt("Dynamic.operator delete");

   Memory::Free(addr, MemDynamic);
}

//------------------------------------------------------------------------------

void Dynamic::operator delete[](void* addr)
{
   Debug::ftnt("Dynamic.operator delete[]");

   Memory::Free(addr, MemDynamic);
}

//------------------------------------------------------------------------------

void Dynamic::operator delete(void* addr, void* place) noexcept
{
   Debug::ftnt("Dynamic.operator delete(place)");
}

//------------------------------------------------------------------------------

void Dynamic::operator delete[](void* addr, void* place) noexcept
{
   Debug::ftnt("Dynamic.operator delete[](place)");
}

//------------------------------------------------------------------------------

void* Dynamic::operator new(size_t size)
{
   Debug::ft("Dynamic.operator new");

   return Memory::Alloc(size, MemDynamic);
}

//------------------------------------------------------------------------------

void* Dynamic::operator new[](size_t size)
{
   Debug::ft("Dynamic.operator new[]");

   return Memory::Alloc(size, MemDynamic);
}

//------------------------------------------------------------------------------

void* Dynamic::operator new(size_t size, void* place)
{
   Debug::ft("Dynamic.operator new(place)");

   return place;
}

//------------------------------------------------------------------------------

void* Dynamic::operator new[](size_t size, void* place)
{
   Debug::ft("Dynamic.operator new[](place)");

   return place;
}

//------------------------------------------------------------------------------

void Dynamic::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
