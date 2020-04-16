//==============================================================================
//
//  SysHeap.h
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
#ifndef SYSHEAP_H_INCLUDED
#define SYSHEAP_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include <iosfwd>
#include "SysDecls.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: heap.
//
class SysHeap : public Permanent
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~SysHeap();

   //  Deleted to prohibit copying.
   //
   SysHeap(const SysHeap& that) = delete;
   SysHeap& operator=(const SysHeap& that) = delete;

   //  Allocates SIZE bytes.
   //
   void* Alloc(size_t size);

   //  Frees the memory segment at ADDR, which is SIZE bytes long.
   //
   void Free(void* addr, size_t size);

   //  Validates the heap.  If ADDR is not nullptr, only the memory
   //  segment alleged to be at ADDR is validated.
   //
   bool Validate(const void* addr) const;

   //  Applies ATTRS to the heap.  The heap must have been
   //  constructed with a fixed size.
   //
   bool SetPermissions(MemoryProtection attrs);

   //  Returns the type of memory that the heap manages.
   //
   MemoryType Type() const { return type_; }

   //  Returns the heap's size.
   //
   size_t Size() const { return size_; }

   //  Returns the heap's current memory protection.
   //
   MemoryProtection Protection() const { return attrs_; }

   //  Returns the address of the heap itself.
   //
   const void* Addr() const;

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

   //  Returns true if the heap has a fixed size.
   //
   bool IsFixedSize() const;

   //  Displays all heaps allocated by this process.
   //
   static void DisplayHeaps(std::ostream& stream);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate memory from the default heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);

   //  Overridden to return memory to the default heap.
   //
   static void operator delete(void* addr);
   static void operator delete[](void* addr);

   //  Deleted to prevent allocation on another heap.
   //
   static void* operator new(size_t size, MemoryType type) = delete;
   static void* operator new[](size_t size, MemoryType type) = delete;
protected:
   //  Creates a heap for memory of TYPE.  If SIZE is 0, the heap's
   //  size can expand, else it is limited to SIZE bytes.  If TYPE
   //  is MemPermanent, this creates a wrapper for the default heap.
   //  Protected because this class is virtual.
   //
   SysHeap(MemoryType type, size_t size);
private:
   //  The type of memory provided by the heap.
   //
   const MemoryType type_;

   //  The native handle to the underlying heap.
   //
   SysHeap_t heap_;

   //  The size of the heap.
   //
   const size_t size_;

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
};
}
#endif
