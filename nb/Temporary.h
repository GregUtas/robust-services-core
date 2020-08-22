//==============================================================================
//
//  Temporary.h
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
#ifndef TEMPORARY_H_INCLUDED
#define TEMPORARY_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that is destroyed during
//  all restarts.  Subclasses contain data that does not need to preserved over
//  a restart or that can easily be recreated.
//
class Temporary : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Temporary() = default;

   //  Overridden to return the type of memory used by subclasses.
   //
   MemoryType MemType() const override { return MemTemporary; }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to use the temporary heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
   static void operator delete(void* addr);
   static void operator delete[](void* addr);

   //  Placement new and delete.
   //
   static void* operator new(size_t size, void* place);
   static void* operator new[](size_t size, void* place);
   static void operator delete(void* addr, void* place) noexcept;
   static void operator delete[](void* addr, void* place) noexcept;
protected:
   //  Protected because this class is virtual.
   //
   Temporary();
};
}
#endif
