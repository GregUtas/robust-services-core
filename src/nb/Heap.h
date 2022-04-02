//==============================================================================
//
//  Heap.h
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
#ifndef HEAP_H_INCLUDED
#define HEAP_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include <iosfwd>
#include <map>
#include <utility>
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

   //  Deleted to prohibit copy assignment.
   //
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

   //  Returns the size of the block at ADDR if it is currently in
   //  use by an application.  Returns 0 if the block is on the heap.
   //
   virtual size_t BlockToSize(const void* addr) const = 0;

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

   //  Enables or disables tracing of allocated blocks.
   //
   void SetTrace(bool enabled);

   //  Clears the set of allocated blocks.
   //
   void ResetTrace();

   //  Displays the addresses of allocated blocks.
   //
   void DisplayBlocks(std::ostream& stream) const;

   //  Overridden to allocate a heap object on the default C++ heap.
   //
   static void* operator new(size_t size);

   //  Overridden to return a heap object to the default C++ heap.
   //
   static void operator delete(void* addr);

   //  Deleted to prevent the allocation of an array of heaps.
   //
   static void* operator new[](size_t size) = delete;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because this class is virtual.
   //
   Heap();

   //  Invoked before returning ADDR for a request of SIZE bytes.
   //
   void Requested(size_t size, void* addr);

   //  Invoked before freeing ADDR, a block that is SIZE bytes long.
   //
   void Freeing(void* addr, size_t size);
private:
   //  Invoked when the heap's memory protection has changed.
   //  Returns 0.
   //
   int SetAttrs(MemoryProtection attrs);

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

   //  Set if in-use blocks are being traced.
   //
   bool trace_;

   //  A trace entry, which contains a block's address and its size.  The
   //  size is negative when a block whose allocation was not recorded is
   //  freed.
   //
   typedef std::pair< void*, word > TraceEntry;

   //  Tracks allocated blocks to assist with detecting memory leaks.
   //
   std::map< void*, word > blocks_;
};
}
#endif
