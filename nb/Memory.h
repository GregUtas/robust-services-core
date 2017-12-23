//==============================================================================
//
//  Memory.h
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
#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <cstddef>
#include "SysTypes.h"

namespace NodeBase
{
   class SysHeap;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Memory management.
//
class Memory
{
public:
   //  Rounds up SIZE bytes to a multiple of LOG2ALIGN bytes.
   //
   static size_t Align(size_t size, size_t log2align = BYTES_PER_WORD_LOG2);

   //  Rounds up nBytes to a word multiple.  The result is in words.
   //
   static size_t Words(size_t nBytes);

   //  Copies nBytes of memory, starting at SOURCE, to DEST.
   //
   static void Copy(void* dest, const void* source, size_t nBytes);

   //  Initializes nBytes of memory to VALUE, starting at DEST.
   //
   static void Set(void* dest, byte_t value, size_t nBytes);

   //  Allocates a memory segment of nBytes of the specified TYPE.  If
   //  EX is true, an AllocationException is thrown on failure.
   //
   static void* Alloc(size_t nBytes, MemoryType type, bool ex = true);

   //  Deallocates the memory segment returned by Alloc.
   //
   static void Free(const void* addr);

   //  Extends the segment at ADDR so that it can hold nBytes.  If there
   //  is insufficient space for the additional bytes, a new segment of
   //  sufficient length is allocated, the existing bytes are copied to
   //  it, and the segment at ADDR is freed.  Returns ADDR if in-place
   //  extension succeeds, nullptr if extension fails, or another value
   //  if a new segment was allocated.
   //
   static void* Realloc(void* addr, size_t nBytes);

   //  Verifies ADDR, which should be of TYPE.  If ADDR is nullptr, the
   //  entire heap for TYPE is verified.
   //
   static bool Verify(MemoryType type, void* addr);

   //  Returns the type of memory used by the object located at ADDR.
   //
   static MemoryType Type(const void* addr);

   //  Returns the heap (if any) associated with TYPE.
   //
   static const SysHeap* Heap(MemoryType type);

   //  Frees the appropriate heap(s) during a restart.
   //
   static void Shutdown(RestartLevel level);
private:
   //  Private because this class only has static members.
   //
   Memory();

   //  Returns the heap for TYPE.  If it doesn't exist, it is created.
   //
   static SysHeap* EnsureHeap(MemoryType type);

   //  Returns the heap (if any) associated with TYPE.
   //
   static SysHeap* AccessHeap(MemoryType type);
};
}
#endif
