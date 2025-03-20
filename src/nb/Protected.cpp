//==============================================================================
//
//  Protected.cpp
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
#include "Protected.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
Protected::Protected()
{
   Debug::ft("Protected.ctor");
}

//------------------------------------------------------------------------------

void Protected::operator delete(void* addr)
{
   Debug::ftnt("Protected.operator delete");

   Memory::Free(addr, MemProtected);
}

//------------------------------------------------------------------------------

void Protected::operator delete[](void* addr)
{
   Debug::ftnt("Protected.operator delete[]");

   Memory::Free(addr, MemProtected);
}

//------------------------------------------------------------------------------

void Protected::operator delete(void* addr, void* place) noexcept
{
   Debug::ftnt("Protected.operator delete(place)");
}

//------------------------------------------------------------------------------

void Protected::operator delete[](void* addr, void* place) noexcept
{
   Debug::ftnt("Protected.operator delete[](place)");
}

//------------------------------------------------------------------------------

void* Protected::operator new(size_t size)
{
   Debug::ft("Protected.operator new");

   return Memory::Alloc(size, MemProtected);
}

//------------------------------------------------------------------------------

void* Protected::operator new[](size_t size)
{
   Debug::ft("Protected.operator new[]");

   return Memory::Alloc(size, MemProtected);
}

//------------------------------------------------------------------------------

void* Protected::operator new(size_t size, void* place)
{
   Debug::ft("Protected.operator new(place)");

   return place;
}

//------------------------------------------------------------------------------

void* Protected::operator new[](size_t size, void* place)
{
   Debug::ft("Protected.operator new[](place)");

   return place;
}

//------------------------------------------------------------------------------

void Protected::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
