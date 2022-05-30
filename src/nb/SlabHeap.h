//==============================================================================
//
//  SlabHeap.h
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
#ifndef SLABHEAP_H_INCLUDED
#define SLABHEAP_H_INCLUDED

#include "Heap.h"
#include <cstddef>
#include <memory>
#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   class SlabPriv;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A heap that grows by adding large blocks of memory (slabs).  This heap is
//  slower, and has more per-block overhead, than BuddyHeap.  It is therefore
//  intended for applications that allocate large blocks of memory that they
//  never, or rarely, free.
//
class SlabHeap : public Heap
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~SlabHeap();

   //  Overridden to return the heap's address.
   //
   void* Addr() const override { return nullptr; }  //* to be deleted

   //  Overridden to allocate SIZE bytes.
   //
   void* Alloc(size_t size) override;

   //  Overridden to return the size of the block at ADDR if it is
   //  currently allocated.
   //
   size_t BlockToSize(const void* addr) const override;

   //  Overridden to return the number of currently available bytes.
   //
   size_t CurrAvail() const override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to free the memory segment at ADDR.
   //
   void Free(void* addr) override;

   //  Overridden to return the number of bytes of management overhead.
   //
   size_t Overhead() const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to return total number of in-use and available bytes.
   //
   size_t Size() const override;

   //  Overridden to return the type of memory that the heap manages.
   //
   MemoryType Type() const override;

   //  Overridden to validate the entire heap or the block at ADDR.
   //
   bool Validate(const void* addr) const override;
protected:
   //  Creates a heap for memory of TYPE.  Protected because this class
   //  is virtual.
   //
   explicit SlabHeap(MemoryType type);
private:
   //  The heap management information.
   //
   std::unique_ptr<SlabPriv> priv_;
};
}
#endif
