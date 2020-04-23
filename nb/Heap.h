//==============================================================================
//
//  Heap.h
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
#ifndef HEAP_H_INCLUDED
#define HEAP_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for heaps.
//
class Heap : public Permanent
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Heap();

   //  Deleted to prohibit copying.
   //
   Heap(const Heap& that) = delete;
   Heap& operator=(const Heap& that) = delete;

   //  Returns the address of the heap itself.
   //
   virtual void* Addr() const = 0;

   //  Returns the heap's size.
   //
   virtual size_t Size() const = 0;

   //  Returns the type of memory that the heap manages.
   //
   virtual MemoryType Type() const = 0;

   //  Allocates SIZE bytes.
   //
   virtual void* Alloc(size_t size) = 0;

   //  Frees the memory segment at ADDR.
   //
   virtual void Free(void* addr) = 0;

   //  Returns the size of the block at ADDR.
   //
   virtual size_t BlockSize(const void* addr) const = 0;

   //  Validates the heap.  If ADDR is not nullptr, only the memory
   //  segment alleged to be at ADDR is validated.
   //
   virtual bool Validate(const void* addr) const = 0;

   //  Returns true if the heap supports write-protection.
   //
   virtual bool CanBeProtected() const { return true; }

   //  Applies ATTRS to the heap.  The heap must have a fixed size.
   //  Returns 0 on success.  On failure, returns a system-specific
   //  error code or initiates a restart if the operation actually
   //  failed when it was attempted.
   //
   virtual int SetPermissions(MemoryProtection attrs);

   //  Returns the heap's current memory protection.
   //
   MemoryProtection GetAttrs() const { return attrs_; }

   //  Invoked when the heap's memory protection has changed.
   //  Returns 0.
   //
   int SetAttrs(MemoryProtection attrs);

   //  Invoked when SIZE bytes of memory were requested.  OK is
   //  set if allocation succeeded.
   //
   void Requested(size_t size, bool ok = true);

   //  Invoked when a SIZE bytes of memory have been freed.
   //
   void Freed(size_t size);

   //  Returns the number of bytes currently allocated from the heap.
   //
   size_t BytesInUse() const { return inUse_; }

   //  Returns the maximum number of bytes allocated from the heap.
   //
   size_t MaxBytesInUse() const { return maxInUse_; }

   //  Returns the number of successful calls to Alloc().
   //
   size_t AllocCount() const { return allocs_; }

   //  Returns the number of unsuccessful calls to Alloc().
   //
   size_t FailCount() const { return fails_; }

   //  Returns the number of times that Free() released memory.
   //
   size_t FreeCount() const { return frees_; }

   //  Returns the number of times that Free() released memory.
   //
   size_t ChangeCount() const { return changes_; }

   //  Returns true if the heap has a fixed size.
   //
   bool IsFixedSize() const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate a heap object on the default C++ heap.
   //
   static void* operator new(size_t size);

   //  Overridden to return a heap object to the default C++ heap.
   //
   static void operator delete(void* addr);

   //  Deleted to prevent the allocation of an array of heaps.
   //
   static void* operator new[](size_t size) = delete;
protected:
   //  Protected because this class is virtual.
   //
   Heap();
private:
   //  The heap's current memory protection attributes.
   //
   MemoryProtection attrs_;

   //  The number of bytes currently allocated on the heap.
   //
   size_t inUse_;

   //  The number of successful calls to Alloc().
   //
   size_t allocs_;

   //  The number of unsuccessful calls to Alloc().
   //
   size_t fails_;

   //  The number of times that Free() released memory.
   //
   size_t frees_;

   //  The maximum number of bytes allocated on the heap.
   //
   size_t maxInUse_;

   //  The number of times the heap's memory protection was changed.
   //
   size_t changes_;
};
}
#endif
