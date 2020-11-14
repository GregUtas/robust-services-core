//==============================================================================
//
//  Immutable.cpp
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
#include "Immutable.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
Immutable::Immutable()
{
   Debug::ft("Immutable.ctor");
}

//------------------------------------------------------------------------------

void Immutable::operator delete(void* addr)
{
   Debug::ftnt("Immutable.operator delete");

   Memory::Free(addr, MemImmutable);
}

//------------------------------------------------------------------------------

void Immutable::operator delete[](void* addr)
{
   Debug::ftnt("Immutable.operator delete[]");

   Memory::Free(addr, MemImmutable);
}

//------------------------------------------------------------------------------

void Immutable::operator delete(void* addr, void* place) noexcept
{
   Debug::ftnt("Immutable.operator delete(place)");
}

//------------------------------------------------------------------------------

void Immutable::operator delete[](void* addr, void* place) noexcept
{
   Debug::ftnt("Immutable.operator delete[](place)");
}

//------------------------------------------------------------------------------

void* Immutable::operator new(size_t size)
{
   Debug::ft("Immutable.operator new");

   return Memory::Alloc(size, MemImmutable);
}

//------------------------------------------------------------------------------

void* Immutable::operator new[](size_t size)
{
   Debug::ft("Immutable.operator new[]");

   return Memory::Alloc(size, MemImmutable);
}

//------------------------------------------------------------------------------

void* Immutable::operator new(size_t size, void* place)
{
   Debug::ft("Immutable.operator new(place)");

   return place;
}

//------------------------------------------------------------------------------

void* Immutable::operator new[](size_t size, void* place)
{
   Debug::ft("Immutable.operator new[](place)");

   return place;
}

//------------------------------------------------------------------------------

void Immutable::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
