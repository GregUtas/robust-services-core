//==============================================================================
//
//  Memory.h
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
#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <new>
#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   class Heap;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Memory management.
//
namespace Memory
{
   //  Rounds up SIZE bytes to a multiple of log2align bytes.
   //
   size_t Align(size_t size, size_t log2align = BYTES_PER_WORD_LOG2);

   //  Rounds up SIZE bytes to a word multiple.  The result is in words.
   //
   size_t Words(size_t size);

   //  Copies SIZE bytes of memory, starting at SOURCE, to DEST.
   //
   void Copy(void* dest, const void* source, size_t size);

   //  Initializes SIZE bytes of memory to VALUE, starting at DEST.
   //
   void Set(void* dest, byte_t value, size_t size);

   //  Allocates a memory segment of SIZE of the specified TYPE.  The
   //  first version throws an AllocationException on failure, whereas
   //  the second version returns nullptr.
   //
   void* Alloc(size_t size, MemoryType type);
   void* Alloc(size_t size, MemoryType type, const std::nothrow_t&);

   //  Deallocates the memory segment returned by Alloc.
   //
   void Free(void* addr, MemoryType type);

   //  Extends the segment at ADDR so that it can hold SIZE bytes.  If
   //  there is insufficient space for the additional bytes, a new segment
   //  of sufficient length is allocated, the existing bytes are copied to
   //  it, and the segment at ADDR is freed.  Returns ADDR if in-place
   //  extension succeeds, nullptr if extension fails, or another value
   //  if a new segment was allocated.
   //
   void* Realloc(void* addr, size_t size, MemoryType type);

   //  Returns the heap (if any) associated with TYPE.
   //
   Heap* AccessHeap(MemoryType type);

   //  Protects the heap for TYPE.
   //
   bool Protect(MemoryType type);

   //  Unprotects the heap for TYPE.
   //
   bool Unprotect(MemoryType type);

   //  Validates ADDR, which should be of TYPE.  If ADDR is nullptr, the
   //  entire heap for TYPE is validated.  Returns
   //  o 1 if the heap was validated
   //  o 0 if the heap was corrupt
   //  o -1 if the heap does not exist
   //
   int Validate(MemoryType type, const void* addr);

   //  Returns the type of memory associated with the heap at ADDR.
   //  Returns MemNull if no heap begins at ADDR.
   //
   MemoryType AddrToType(const void* addr);

   //  Displays the system's heaps in STREAM, using PREFIX at the start
   //  of each line.
   //
   void DisplayHeaps(std::ostream& stream, const std::string& prefix);

   //  Frees the appropriate heap(s) during a restart.
   //
   void Shutdown();
}
}
#endif
