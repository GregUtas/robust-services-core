//==============================================================================
//
//  NbHeap.h
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
#ifndef NBHEAP_H_INCLUDED
#define NBHEAP_H_INCLUDED

#include "Heap.h"
#include <cstddef>
#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   struct HeapBlock;
   struct HeapPriv;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  RSC's heap implementation, which is implemented using buddy allocation.
//  It exists because Windows heap implementation runs into trouble if one
//  of its heaps is write-protected.  A heap implementation that supports
//  this capability is required.
//
class NbHeap : public Heap
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~NbHeap();

   //  Overridden to return the heap's address.
   //
   void* Addr() const override { return heap_; }

   //  Overridden to return the heap's size.
   //
   size_t Size() const override { return size_; }

   //  Overridden to return the type of memory that the heap manages.
   //
   MemoryType Type() const override { return type_; }

   //  Overridden to allocate SIZE bytes.
   //
   void* Alloc(size_t size) override;

   //  Overridden to free the memory segment at ADDR.
   //
   void Free(void* addr) override;

   //  Overridden to return the size of the block at ADDR if it is
   //  currently allocated.
   //
   size_t BlockToSize(const void* addr) const override;

   //  Overridden to validate the heap or the block at ADDR.
   //
   bool Validate(const void* addr) const override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  The type for a level within the heap.  Blocks at the same level have
   //  the same size.  Blocks at level N-1 are twice the size of blocks at
   //  level N.
   //
   typedef int level_t;

   //  The type for a block's index.  The state of each block is maintained
   //  in separate heap management data.  If the heap spans N blocks of its
   //  minimum size, the blocks are numbered 0...2N-1, with block#0 being the
   //  one that spans the entire heap, block#1 and #2 being its children, and
   //  so on.
   //
   typedef size_t index_t;
protected:
   //  Creates a heap for memory of TYPE.  Variable-length heaps (SIZE=) are
   //  not currently supported.  The heap is limited to SIZE bytes, which is
   //  rounded up to a multiple of the smallest block size.  Protected because
   //  this class is virtual.
   //
   NbHeap(MemoryType type, size_t size);
private:
   //  The state of a block.
   //
   enum BlockState
   {
      Merged,     // merged with sibling: look at parent block
      Split,      // split from sibling: look at child block
      Allocated,  // in use by application software
      Available,  // on heap's free queue
      Invalid     // used to denote an invalid block address
   };

   //  Puts BLOCK on the free queue at LEVEL when initializing the heap.
   //
   void ReleaseBlock(HeapBlock* block, level_t level);

   //  Marks BLOCK as off-limits when initializing the heap.  This is done
   //  for blocks that overlay heap management data.
   //
   void ReserveBlock(const HeapBlock* block);

   //  ReleaseBlock or ReserveBlock has just been invoked on the block
   //  identified by INDEX.  Update the state of its ancestors to Split.
   //
   void SplitAncestors(index_t block);

   //  Puts BLOCK, which is associated with INDEX, on LEVEL's free queue
   //  and initializes it.
   //
   void EnqBlock(HeapBlock* block, index_t index, level_t level);

   //  Sets the block that is identified by INDEX to STATE.
   //
   void SetState(index_t index, BlockState state);

   //  Returns the state of the block that is identified by INDEX.
   //
   BlockState GetState(index_t index) const;

   //  Allocates a block at LEVEL.  If it is larger than SIZE, it is split
   //  into two siblings, with one being requeued and the other returned.
   //
   HeapBlock* AllocBlock(level_t level, size_t size);

   //  Dequeues a block at LEVEL, validates it, and marks it as allocated.
   //  Returns nullptr if no blocks are available at LEVEL.
   //
   HeapBlock* Dequeue(level_t level);

   //  Frees BLOCK by returning it to LEVEL.  If BLOCK's sibling is not in
   //  use, merges the two blocks and returns them to LEVEL-1.
   //
   void FreeBlock(HeapBlock* block, level_t level);

   //  Enqueues BLOCK at LEVEL, initializes it, and returns nullptr.  But
   //  if BLOCK's sibling is free, exqueues it, validates it, and returns
   //  a pointer to it.
   //
   HeapBlock* Enqueue(HeapBlock* block, level_t level);

   //  Returns true if ADDR
   //  o is a legal block address regardless of its current state, or
   //  o if HEADER is set, if ADDR the address of a free queue header.
   //
   bool AddrIsValid(const void* addr, bool header) const;

   //  Validates the block at INDEX and LEVEL and returns its state.
   //  If the block is corrupt, returns Invalid or initiates a restart
   //  if RESTART is set.
   //
   BlockState ValidateBlock(index_t index, level_t level, bool restart) const;

   //  Invoked when heap corruption is detected.  REASON specifies the type
   //  of corruption, and RESTART is set to initiate a restart.
   //
   BlockState Corrupt(int reason, bool restart) const;

   //  Returns the index that accesses BLOCK's state within heap management
   //  data.  The block's LEVEL, which also corresponds to its size, must
   //  be provided because blocks of different sizes have the same address
   //  depending on how they are currently merged with, or split from, their
   //  siblings.
   //
   index_t BlockToIndex(const HeapBlock* block, level_t level) const;

   //  Returns the block associated with INDEX.
   //
   HeapBlock* IndexToBlock(index_t index, level_t level) const;

   //  The heap, which begins with its management information.
   //
   HeapPriv* heap_;

   //  The heap's size.
   //
   size_t size_;

   //  The type of memory that the heap manages.
   //
   const MemoryType type_;

   //  The name for the heap's lock.
   //
   std::string lockName_;
};
}
#endif
