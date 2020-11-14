//==============================================================================
//
//  Permanent.cpp
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
#include "Permanent.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
Permanent::Permanent()
{
   Debug::ft("Permanent.ctor");
}

//------------------------------------------------------------------------------

void Permanent::operator delete(void* addr)
{
   Debug::ftnt("Permanent.operator delete");

   Memory::Free(addr, MemPermanent);
}

//------------------------------------------------------------------------------

void Permanent::operator delete[](void* addr)
{
   Debug::ftnt("Permanent.operator delete[]");

   Memory::Free(addr, MemPermanent);
}

//------------------------------------------------------------------------------

void Permanent::operator delete(void* addr, void* place) noexcept
{
   Debug::ftnt("Permanent.operator delete(place)");
}

//------------------------------------------------------------------------------

void Permanent::operator delete[](void* addr, void* place) noexcept
{
   Debug::ftnt("Permanent.operator delete[](place)");
}

//------------------------------------------------------------------------------

void* Permanent::operator new(size_t size)
{
   Debug::ft("Permanent.operator new");

   return Memory::Alloc(size, MemPermanent);
}

//------------------------------------------------------------------------------

void* Permanent::operator new[](size_t size)
{
   Debug::ft("Permanent.operator new[]");

   return Memory::Alloc(size, MemPermanent);
}

//------------------------------------------------------------------------------

void* Permanent::operator new(size_t size, void* place)
{
   Debug::ft("Permanent.operator new(place)");

   return place;
}

//------------------------------------------------------------------------------

void* Permanent::operator new[](size_t size, void* place)
{
   Debug::ft("Permanent.operator new[](place)");

   return place;
}

//------------------------------------------------------------------------------

void Permanent::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
