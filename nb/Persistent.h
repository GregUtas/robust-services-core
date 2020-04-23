//==============================================================================
//
//  Persistent.h
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
#ifndef PERSISTENT_H_INCLUDED
#define PERSISTENT_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that survives both warm
//  and cold restarts.  Subclasses usually contain data that is associated with
//  data subclassed from Protected, but which changes too frequently to be
//  write-protected.
//
class Persistent : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Persistent() = default;

   //  Overridden to return the type of memory used by subclasses.
   //
   MemoryType MemType() const override { return MemPersistent; }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to use the persistent heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
   static void operator delete(void* addr);
   static void operator delete[](void* addr);
protected:
   //  Protected because this class is virtual.
   //
   Persistent();
};
}
#endif