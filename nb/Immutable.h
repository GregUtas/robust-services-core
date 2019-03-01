//==============================================================================
//
//  Immutable.h
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
#ifndef IMMUTABLE_H_INCLUDED
#define IMMUTABLE_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that is write-protected
//  at run-time and that survives all restarts.  Subclasses typically contain
//  data that can only be recreated by rebooting.
//
class Immutable : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Immutable() = default;

   //  Overridden to return the type of memory used by subclasses.
   //
   MemoryType MemType() const override { return MemImm; }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate memory from the immutable heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
protected:
   //  Protected because this class is virtual.
   //
   Immutable();
};
}
#endif
