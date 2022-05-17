//==============================================================================
//
//  SysMemory.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef SYSMEMORY_H_INCLUDED
#define SYSMEMORY_H_INCLUDED

#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: memory functions.
//
namespace SysMemory
{
   //  Allocates SIZE memory, applying ATTRS to it.  Returns the address
   //  where the segment begins.  If ADDR is provided, the segment begins
   //  at that location on success.
   //
   void* Alloc(void* addr, size_t size, MemoryProtection attrs = MemReadWrite);

   //  Frees the memory at ADDR, which was previously returned by Alloc.
   //  SIZE should be the size of segment requested from Alloc.
   //
   bool Free(void* addr, size_t size);

   //  Disables paging for ADDR[0 to SIZE-1].
   //
   bool Lock(void* addr, size_t size);

   //  Enables paging for ADDR[0 to SIZE-1].  All pages in that range must
   //  currently be locked.
   //
   bool Unlock(void* addr, size_t size);

   //  Applies ATTRS to ADDR[0 to SIZE-1].  Returns 0 on success, else a
   //  system-specific failure code.
   //
   int Protect(void* addr, size_t size, MemoryProtection attrs);
}
}
#endif
