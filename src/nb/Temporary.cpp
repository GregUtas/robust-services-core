//==============================================================================
//
//  Temporary.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "Temporary.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
Temporary::Temporary()
{
   Debug::ft("Temporary.ctor");
}

//------------------------------------------------------------------------------

void Temporary::operator delete(void* addr)
{
   Debug::ftnt("Temporary.operator delete");

   Memory::Free(addr, MemTemporary);
}

//------------------------------------------------------------------------------

void Temporary::operator delete[](void* addr)
{
   Debug::ftnt("Temporary.operator delete[]");

   Memory::Free(addr, MemTemporary);
}

//------------------------------------------------------------------------------

void Temporary::operator delete(void* addr, void* place) noexcept
{
   Debug::ftnt("Temporary.operator delete(place)");
}

//------------------------------------------------------------------------------

void Temporary::operator delete[](void* addr, void* place) noexcept
{
   Debug::ftnt("Temporary.operator delete[](place)");
}

//------------------------------------------------------------------------------

void* Temporary::operator new(size_t size)
{
   Debug::ft("Temporary.operator new");

   return Memory::Alloc(size, MemTemporary);
}

//------------------------------------------------------------------------------

void* Temporary::operator new[](size_t size)
{
   Debug::ft("Temporary.operator new[]");

   return Memory::Alloc(size, MemTemporary);
}

//------------------------------------------------------------------------------

void* Temporary::operator new(size_t size, void* place)
{
   Debug::ft("Temporary.operator new(place)");

   return place;
}

//------------------------------------------------------------------------------

void* Temporary::operator new[](size_t size, void* place)
{
   Debug::ft("Temporary.operator new[](place)");

   return place;
}

//------------------------------------------------------------------------------

void Temporary::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
