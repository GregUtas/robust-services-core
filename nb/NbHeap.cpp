//==============================================================================
//
//  NbHeap.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "NbHeap.h"
#include <cstdint>
#include <ios>
#include <iosfwd>
#include <memory>
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "Formatters.h"
#include "MutexGuard.h"
#include "NbTypes.h"
#include "Q2Link.h"
#include "Q2Way.h"
#include "Restart.h"
#include "SysMemory.h"
#include "SysMutex.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A block managed by the heap.
//
struct HeapBlock
{
   //  The block's link when it is on the heap's free queue.
   //
   Q2Link link;

   //  Set to a pre-defined pattern to detect trampling.
   //
   size_t fence[2];

   //  The fence pattern for blocks on the free queue.
   //
   static const size_t FencePattern =
      (BYTES_PER_WORD == 4 ? 0xaaaaaaaa : 0xaaaaaaaaaaaaaaaa);

   HeapBlock()
   {
      fence[0] = FencePattern;
      fence[1] = FencePattern;
   }

   //  Displays member variables.  This has the same signature as
   //  Base.Display so that Q2Way can invoke it.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const;
};

//------------------------------------------------------------------------------

void HeapBlock::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "link : " << CRLF;
   link.Display(stream, prefix + spaces(2));
}

//==============================================================================
//
//  The minimum size of a block allocated from the heap.
//
constexpr size_t MinBlockSize = sizeof(HeapBlock);

//  Log2 of the minimum block size.  HeapBlock contains 4 words (pointers), so
//  multiply by number of bytes in a word by 4 by adding 2 to its log2 value.
//
constexpr size_t MinBlockSizeLog2 = BYTES_PER_WORD_LOG2 + 2;

//  The number of block sizes.  Each block size is a power of 2, which means
//  that the largest heap could be MinBlockSize * 2^31 = MinBlockSize * 2GB.
//  The largest block that could be allocated is MinBlockSize * 1GB, because
//  heap management information uses some space at the beginning of the heap.
//
constexpr NbHeap::level_t NumLevels = 32;
constexpr NbHeap::level_t LastLevel = NumLevels - 1;

//  Types of heap corruption that can be detected.
//
enum HeapCorruptionReason
{
   FenceInvalid,         // block's fence pattern trampled
   PrevInvalid,          // block's prev pointer invalid
   NextInvalid,          // block's next pointer invalid
   PrevNextInvalid,      // previous block's next pointer invalid
   NextPrevInvalid,      // next block's prev pointer invalid
   ParentStateInvalid,   // unexpected state for parent
   SiblingStateInvalid,  // unexpected state for sibling
   ChildStateInvalid,    // unexpected state for child
   ExqFailure            // failed to exqueue sibling
};

//------------------------------------------------------------------------------
//
//  Returns the index of the first child associated with INDEX.
//  The second child's index follows immediately.
//
static NbHeap::index_t IndexToChild(NbHeap::index_t index)
{
   return (index << 1) + 1;
}

//------------------------------------------------------------------------------
//
//  Returns the index of the parent associated with INDEX.
//
static NbHeap::index_t IndexToParent(NbHeap::index_t index)
{
   return (index - 1) >> 1;
}

//------------------------------------------------------------------------------
//
//  Returns the index of the sibling associated with INDEX.
//
static NbHeap::index_t IndexToSibling(NbHeap::index_t index)
{
   return ((index & 0x01) == 0 ? index - 1 : index + 1);
}

//------------------------------------------------------------------------------
//
//  Returns log2 of the size of a block at LEVEL.  Blocks at LastLevel have a
//  size (log2) of MinBlockSizeLog2, and the size of a block doubles at each
//  level above that.
//
static size_t Log2Size(NbHeap::level_t level)
{
   return (MinBlockSizeLog2 + LastLevel - level);
}

//------------------------------------------------------------------------------
//
//  Returns the size of a block at LEVEL.
//
static size_t LevelToSize(NbHeap::level_t level)
{
   return size_t(1) << Log2Size(level);
}

//------------------------------------------------------------------------------
//
//  Returns the level associated with a block of SIZE.
//
static NbHeap::level_t SizeToLevel(size_t size)
{
   return LastLevel - (log2(size, true) - MinBlockSizeLog2);
}

//==============================================================================
//
//  Heap management information.
//
struct HeapPriv
{
   //  For locking the heap during operations.
   //
   std::unique_ptr< SysMutex > lock;

   //  The logical start of the heap.  If the heap's size is a power of 2,
   //  this is the same as its actual start.  If not, the heap's logical
   //  size is the least power of 2 that would span the entire heap.  The
   //  heap then begins with blocks that are marked allocated because they
   //  are located *before* the start of the actual heap.  This is followed
   //  by blocks reserved for the heap management information.  After this
   //  are the useable blocks, which run to the true heap of the heap.
   //
   uintptr_t leftAddr;

   //  The first valid block address after the management information.  The
   //  address of a block allocated from the heap must be >= to this value.
   //
   uintptr_t minAddr;

   //  The last valid block address.
   //
   uintptr_t maxAddr;

   //  The first level where blocks can be queued.
   //
   NbHeap::level_t minLevel;

   //  The maximum index into the STATE array.
   //
   NbHeap::index_t maxIndex;

   //  The queues of free blocks.  Blocks at head_[LastLevel] have a size of
   //  MinBlockSize, and their size doubles as the index decrements.  The
   //  queue is a two-way queue so that a sibling can be extracted quickly
   //  when two blocks can be merged.
   //
   Q2Way< HeapBlock > freeq[NumLevels];

   //  The state of each block (see NbHeap::BlockState).  Each state uses
   //  two bits.
   //
   uint8_t* state;

   HeapPriv() :
      leftAddr(0),
      minAddr(0),
      maxAddr(0),
      minLevel(0),
      maxIndex(0),
      state(nullptr)
   {
      for(auto i = 0; i <= LastLevel; ++i) freeq[i].Init(0);
   }
};

//------------------------------------------------------------------------------

fn_name NbHeap_ctor = "NbHeap.ctor";

NbHeap::NbHeap(MemoryType type, size_t size) : Heap(),
   heap_(nullptr),
   size_(0),
   type_(type)
{
   Debug::ft(NbHeap_ctor);

   //  Round up the size of the heap management data to the next power of 2
   //  so that it will overlay a whole number of blocks.
   //
   size_t infoSize = round_to_2_exp_n(sizeof(HeapPriv), MinBlockSizeLog2, true);
   size_t minSize = (size_t(1) << log2(infoSize, true));

   //  SIZE must be at least the smallest power of 2 that is larger than the
   //  size of the heap management data.
   //
   if(size < minSize)
   {
      std::ostringstream expl;
      expl << "heap size must be at least " << minSize;
      Debug::SwLog(NbHeap_ctor, expl.str(), size);
      return;
   }

   //  Allocate the heap's mutex.
   //
   std::ostringstream stream;
   stream << "HeapLock(" << type_ << ')';
   lockName_ = stream.str();
   std::unique_ptr< SysMutex > lock(new SysMutex(lockName_.c_str()));

   if(lock == nullptr)
   {
      Restart::Initiate(RestartWarm, MutexCreationFailed, 0);
      return;
   }

   //  Round SIZE up to a multiple of the smallest block size.  Allocate
   //  memory for the heap, initialize its management data, and have it
   //  take ownership of the lock.
   //
   size_ = round_to_2_exp_n(size, MinBlockSizeLog2, true);
   heap_ = (HeapPriv*) SysMemory::Alloc(nullptr, size_);

   if(heap_ == nullptr)
   {
      size_ = 0;
      lock.release();
      Restart::Initiate(RestartWarm, HeapCreationFailed, type);
      return;
   }

   new (heap_) HeapPriv();
   heap_->lock.reset(lock.release());

   //  Find the heap's lowest level, which is the level where the smallest
   //  block that would span the entire heap would be placed.  Update SIZE
   //  to the lowest power of 2 that would span the entire heap.
   //
   auto spanLog2 = log2(size_, true);
   heap_->minLevel = LastLevel - (spanLog2 - MinBlockSizeLog2);

   //  Set the heap's leftmost address, which precedes heap_ (its true start)
   //  if its size_ is not a power of 2.
   //
   auto heapAddr = uintptr_t(heap_);
   auto spanSize = size_t(1) << spanLog2;
   heap_->leftAddr = heapAddr + size_ - spanSize;

   //  Find the size of the STATE array.  There is a state for each block that
   //  could be allocated: this is *twice* the number of blocks of MinBlockSize,
   //  because buddies can be merged to handle larger requests.  Each state is
   //  2 bits, so each byte can hold 4 states.  Round off the size of STATES
   //  so that it overlays a whole number of blocks.
   //
   size_t maxBlocks = spanSize >> MinBlockSizeLog2;
   heap_->maxIndex = maxBlocks - 1;
   size_t stateSize = (2 * maxBlocks) / 4;
   stateSize = round_to_2_exp_n(stateSize, MinBlockSizeLog2, true);

   //  Set the addresses of the STATE array and initialize it to indicate that
   //  all blocks are merged.
   //
   heap_->state = (uint8_t*) (heapAddr + infoSize);
   for(size_t i = 0; i < stateSize; ++i) heap_->state[i] = 0;

   //  Set the addresses of the first and last blocks that can be allocated
   //  from the heap.
   //
   heap_->minAddr = heapAddr + infoSize + stateSize;
   heap_->maxAddr = heapAddr + size_ - MinBlockSize;

   //  Initialize the heap's free queues.
   //
   for(auto i = 0; i <= LastLevel; ++i) heap_->freeq[i].Init(0);

   //  Put the available memory on the heap's free queues.  The front of the
   //  heap contains memory that is off-limits because it either precedes the
   //  heap (to make its logical size a power of 2) or because it contains the
   //  management information.  We therefore work backwards from the *end* of
   //  the heap, starting with a block whose size is half that of the heap,
   //  rounded up to the next power of 2. Halve the size of each successive
   //  block while checking that it does not infringe on the management data.
   //
   size = (size_t(1) << log2(size_, true)) >> 1;
   auto addr = heapAddr + size_;
   auto level = SizeToLevel(size);
   auto avail = heapAddr + size_ - heap_->minAddr;

   while(avail > 0)
   {
      if(size <= avail)
      {
         addr -= size;
         avail -= size;
         ReleaseBlock((HeapBlock*) addr, level);
      }

      ++level;
      size >>= 1;
   }

   //  Mark all the blocks in the heap management area as allocated.
   //
   for(addr = heap_->leftAddr; addr < heap_->minAddr; addr += MinBlockSize)
   {
      ReserveBlock((HeapBlock*) addr);
   }
}

//------------------------------------------------------------------------------

NbHeap::~NbHeap()
{
   Debug::ftnt("NbHeap.dtor");

   if(heap_ == nullptr) return;
   heap_->lock->Acquire(TIMEOUT_NEVER);

   std::unique_ptr< SysMutex > lock(heap_->lock.release());
   SetPermissions(MemReadWrite);
   SysMemory::Free(heap_);
   heap_ = nullptr;
   lock->Release();
   lock.reset();
}

//------------------------------------------------------------------------------

bool NbHeap::AddrIsValid(const void* addr, bool header) const
{
   auto iAddr = uintptr_t(addr);

   if((iAddr >= heap_->minAddr) && (iAddr <= heap_->maxAddr))
   {
      return (find_first_one(iAddr - heap_->minAddr) >= MinBlockSizeLog2);
   }

   if(header)
   {
      //  A queued block can point to the queue header, which is included in
      //  the chain (and which points to itself if the queue is empty).
      //
      return ((addr >= &heap_->freeq[0]) && (addr < &heap_->freeq[NumLevels]));
   }

   return false;
}

//------------------------------------------------------------------------------

void* NbHeap::Alloc(size_t size)
{
   Debug::ft("NbHeap.Alloc");

   //  Allocate a block at the level that can accommodate SIZE.
   //
   MutexGuard guard(heap_->lock.get());

   if(size < MinBlockSize) size = MinBlockSize;
   auto level = SizeToLevel(size);
   if(level > LastLevel) return nullptr;

   auto block = AllocBlock(level, size);
   Requested(size, block);
   return block;
}

//------------------------------------------------------------------------------

HeapBlock* NbHeap::AllocBlock(level_t level, size_t size)
{
   //  Allocate a block at LEVEL.  If no block is available, try the next
   //  level with larger blocks.  If a block is obtained that could be split
   //  and still accommodate SIZE, split it and requeue its right child before
   //  before returning it.
   //
   if(level < heap_->minLevel) return nullptr;

   auto block = Dequeue(level);
   if(block == nullptr) block = AllocBlock(level - 1, size);
   if(block == nullptr) return nullptr;

   auto index = BlockToIndex(block, level);

   if(LevelToSize(level + 1) >= size)
   {
      SetState(index, Split);
      auto child = (HeapBlock*) (uintptr_t(block) + LevelToSize(level + 1));
      index = IndexToChild(index) + 1;
      EnqBlock(child, index, level + 1);
   }
   else
   {
      SetState(index, Allocated);
   }

   return block;
}

//------------------------------------------------------------------------------

NbHeap::index_t NbHeap::BlockToIndex
   (const HeapBlock* block, level_t level) const
{
   //  BLOCK's index is found by adding the index of the first block in
   //  LEVEL to the number of blocks that precede BLOCK within LEVEL.
   //
   auto first = (size_t(1) << (level - heap_->minLevel)) - 1;
   auto offset = (uintptr_t(block) - heap_->leftAddr) >> Log2Size(level);
   return first + offset;
}

//------------------------------------------------------------------------------

size_t NbHeap::BlockToSize(const void* addr) const
{
   Debug::ft("NbHeap.BlockToSize");

   //  ADDR can be used at any level where it falls on a block boundary.
   //  Find the number of "0" bits after its last "1" bit.  It must have
   //  at least MinBlockSizeLog2 of them to be a valid address at LastLevel.
   //  The more of them that it has, the higher up the tree it can coincide
   //  with a block boundary.
   //
   if(!AddrIsValid(addr, false)) return 0;

   auto zeroes = find_first_one(uintptr_t(addr) - heap_->leftAddr);
   if(zeroes < MinBlockSizeLog2) return 0;

   //  Find the first level at which BLOCK might currently reside and then
   //  find its index.
   //
   auto block = (const HeapBlock*) addr;
   auto level = LastLevel + MinBlockSizeLog2 - zeroes;
   auto index = BlockToIndex(block, level);

   //  BLOCK is a valid address at LEVEL or greater, and INDEX is its index
   //  at LEVEL.  Proceed down the levels until BLOCK's address matches that
   //  of a block that has not been split.
   //
   while(level <= LastLevel)
   {
      switch(GetState(index))
      {
      case Split:
         ++level;
         index = (index << 1) + 1;
         continue;

      case Allocated:
         return LevelToSize(level);

      default:
         //
         //  The block is available or merged, so ADDR does not match that
         //  of an in-use block.
         //
         return 0;
      }
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  Invoked when heap corruption is detected.
//
NbHeap::BlockState NbHeap::Corrupt(int reason, bool restart) const
{
   if(restart && !Element::RunningInLab())
   {
      Restart::Initiate(Restart::LevelToClear(Type()), HeapCorruption, reason);
   }

   return Invalid;
}

//------------------------------------------------------------------------------

HeapBlock* NbHeap::Dequeue(level_t level)
{
   auto block = heap_->freeq[level].Deq();
   if(block == nullptr) return nullptr;
   auto index = BlockToIndex(block, level);
   SetState(index, Allocated);
   ValidateBlock(index, level, true);
   return block;
}

//------------------------------------------------------------------------------

void NbHeap::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Heap::Display(stream, prefix, options);

   auto lead = prefix + spaces(2);

   stream << prefix << "heap     : " << heap_ << CRLF;
   stream << prefix << "size     : " << size_ << CRLF;
   stream << prefix << "type     : " << type_ << CRLF;
   stream << prefix << "lock     : " << heap_->lock.get() << CRLF;
   stream << std::hex;
   stream << prefix << "leftAddr : " << heap_->leftAddr << CRLF;
   stream << prefix << "minAddr  : " << heap_->minAddr << CRLF;
   stream << prefix << "maxAddr  : " << heap_->maxAddr << CRLF;
   stream << std::dec;
   stream << prefix << "minLevel : " << heap_->minLevel << CRLF;
   stream << prefix << "lock     : ";
   heap_->lock->Display(stream, lead, options);

   auto verbose = options.test(DispVerbose);

   if(verbose)
   {
      size_t free = 0;

      stream << prefix << "freeq [level] : " << CRLF;

      for(auto level = 0; level <= LastLevel; ++level)
      {
         auto count = heap_->freeq[level].Size();
         if(count == 0) continue;
         stream << lead << strIndex(level) << "count=" << count << CRLF;
         free += (count * LevelToSize(level));
      }

      stream << prefix << "Free bytes : " << free << CRLF;

      if(LastLevel - heap_->minLevel <= 7)
      {
         stream << prefix << "Block states : " << CRLF;
         index_t index = 0;
         size_t nextLevelBegin = 1;

         for(auto level = heap_->minLevel; level <= LastLevel; ++level)
         {
            auto first = true;
            auto tab = spaces((1 << (LastLevel - level)) >> 1);

            for(NO_OP; index < nextLevelBegin; ++index)
            {
               char c = '?';

               switch(GetState(index))
               {
               case Available: c = 'F'; break;
               case Allocated: c = 'A'; break;
               case Split:     c = 'S'; break;
               case Merged:    c = 'm'; break;
               }

               if(level == LastLevel)
               {
                  auto block = IndexToBlock(index, level);

                  if(block < (HeapBlock*) heap_)
                     c = '-';
                  else if(block < (HeapBlock*) heap_->minAddr)
                     c = 'a';
               }

               stream << tab << c;

               if(first)
               {
                  first = false;
                  tab = spaces(2 * tab.size() - 1);
               }
            }

            stream << CRLF;
            nextLevelBegin = (nextLevelBegin << 1) + 1;
         }
      }
   }
}

//------------------------------------------------------------------------------

void NbHeap::EnqBlock(HeapBlock* block, index_t index, level_t level)
{
   new (block) HeapBlock();
   heap_->freeq[level].Enq(*block);
   SetState(index, Available);
}

//------------------------------------------------------------------------------

HeapBlock* NbHeap::Enqueue(HeapBlock* block, level_t level)
{
   auto b = BlockToIndex(block, level);
   auto s = IndexToSibling(b);

   if(GetState(s) != Available)
   {
      EnqBlock(block, b, level);
      return nullptr;
   }

   auto sibling = IndexToBlock(s, level);
   ValidateBlock(s, level, true);

   if(!heap_->freeq[level].Exq(*sibling))
   {
      Restart::Initiate
         (Restart::LevelToClear(Type()), HeapCorruption, ExqFailure);
   }

   return sibling;
}

//------------------------------------------------------------------------------

void NbHeap::Free(void* addr)
{
   Debug::ft("NbHeap.Free");

   auto size = BlockToSize(addr);
   if(size == 0) return;

   MutexGuard guard(heap_->lock.get());

   Freeing(addr, size);
   auto level = SizeToLevel(size);
   if(level > LastLevel) return;

   FreeBlock((HeapBlock*) addr, level);
}

//------------------------------------------------------------------------------

void NbHeap::FreeBlock(HeapBlock* block, level_t level)
{
   //  Return ADDR to its free queue.  If its sibling is not in use, Enqueue
   //  exqueues and returns the sibling, so merge the two blocks and free the
   //  resulting block, which might cause additional mergers.
   //
   auto sibling = Enqueue(block, level);
   if(sibling == nullptr) return;

   auto index = BlockToIndex(block, level);
   SetState(index, Merged);
   index = IndexToSibling(index);
   SetState(index, Merged);
   if(block > sibling) block = sibling;
   FreeBlock(block, level - 1);
}

//------------------------------------------------------------------------------

NbHeap::BlockState NbHeap::GetState(index_t index) const
{
   //  Each byte holds four states, so right shift INDEX by 2 bits to find the
   //  first-level index.  Extract the two low-order bits as the second-level
   //  index.  Left shift the mask 0x03 by twice that distance to extract the
   //  state.
   //
   auto index0 = index >> 2;
   auto index1 = index & 0x03;
   auto mask = 0x03 << (index1 << 1);
   auto state = (heap_->state[index0] & mask) >> (index1 << 1);
   return BlockState(state);
}

//------------------------------------------------------------------------------

HeapBlock* NbHeap::IndexToBlock(index_t index, level_t level) const
{
   //  BLOCK's address is found by subtracting the index of the first
   //  block in LEVEL from INDEX and then skipping over the number of
   //  blocks that precede BLOCK within LEVEL.
   //
   auto first = (size_t(1) << (level - heap_->minLevel)) - 1;
   auto offset = index - first;
   return (HeapBlock*) (heap_->leftAddr + (offset << Log2Size(level)));
}

//------------------------------------------------------------------------------

void NbHeap::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void NbHeap::ReleaseBlock(HeapBlock* block, level_t level)
{
   //  When the heap is initialized, queueing a block means that it is split
   //  from its sibling, which also means their ancestors are split.  It is
   //  safe to stop if we reach an ancestor that is already split.
   //
   auto index = BlockToIndex(block, level);
   EnqBlock(block, index, level);

   index = IndexToSibling(index);
   SetState(index, Split);
   SplitAncestors(index);
}

//------------------------------------------------------------------------------

void NbHeap::ReserveBlock(const HeapBlock* block)
{
   //  Mark BLOCK as allocated and proceed up the tree to mark its ancestors
   //  as split.  It is safe to stop if we reach an ancestor that is already
   //  split.
   //
   auto index = BlockToIndex(block, LastLevel);
   SetState(index, Allocated);
   SplitAncestors(index);
}

//------------------------------------------------------------------------------

void NbHeap::SetState(index_t index, BlockState state)
{
   //  Each byte holds four states, so right shift INDEX by 2 bits to find the
   //  first-level index.  Extract the two low-order bits as the second-level
   //  index.  Left shift the mask 0x03 by twice that distance to clear the
   //  state, and then left-shift the state by that amount to set its value.
   //
   auto index0 = index >> 2;
   auto index1 = index & 0x03;
   auto mask = 0x03 << (index1 << 1);
   heap_->state[index0] &= ~mask;
   if(state != Merged) heap_->state[index0] |= (state << (index1 << 1));
}

//------------------------------------------------------------------------------

void NbHeap::SplitAncestors(index_t block)
{
   while(block > 0)
   {
      block = IndexToParent(block);

      if(GetState(block) == Merged)
         SetState(block, Split);
      else
         return;
   }
}

//------------------------------------------------------------------------------

bool NbHeap::Validate(const void* addr) const
{
   Debug::ft("NbHeap.Validate");

   MutexGuard guard(heap_->lock.get());

   if(addr != nullptr)
   {
      auto size = BlockToSize((const HeapBlock*) addr);
      if(size == 0) return false;
      auto level = SizeToLevel(size);
      auto index = BlockToIndex((const HeapBlock*) addr, level);
      return (ValidateBlock(index, level, false) == Allocated);
   }

   size_t index = 0;
   size_t levelSize = 1;

   for(auto level = heap_->minLevel; level <= LastLevel; ++level)
   {
      for(size_t n = 0; n < levelSize; ++index, ++n)
      {
         if(ValidateBlock(index, level, false) == Invalid) return false;
      }

      levelSize <<= 1;
   }

   return true;
}

//------------------------------------------------------------------------------

NbHeap::BlockState NbHeap::ValidateBlock
   (index_t index, level_t level, bool restart) const
{
   //  Find the block's state.  If the block is available, it should be
   //  on the free queue, so check its links and fence.
   //
   auto state = GetState(index);

   switch(state)
   {
   case Merged:
   case Split:
      break;

   case Available:
   {
      //  The block is on the free queue, so check its links and fence.
      //
      auto block = IndexToBlock(index, level);

      if(!AddrIsValid(block->link.prev, true))
         return Corrupt(PrevInvalid, restart);
      if(!AddrIsValid(block->link.next, true))
         return Corrupt(NextInvalid, restart);

      if(block->fence[0] != HeapBlock::FencePattern)
         return Corrupt(FenceInvalid, restart);
      if(block->fence[1] != HeapBlock::FencePattern)
         return Corrupt(FenceInvalid, restart);

      if((HeapBlock*) block->link.prev->next != block)
         return Corrupt(PrevNextInvalid, restart);
      if((HeapBlock*) block->link.next->prev != block)
         return Corrupt(NextPrevInvalid, restart);
      //  [[fallthrough]]
   }

   case Allocated:
   {
      //  The block's sibling should not be merged.  Its parent
      //  should be Split, and its children should be Merged.
      //
      auto sibling = IndexToSibling(index);
      auto other = GetState(sibling);

      if(other == Merged)
         return Corrupt(SiblingStateInvalid, restart);

      auto parent = IndexToParent(index);
      other = GetState(parent);
      if(other != Split)
         return Corrupt(ParentStateInvalid, restart);

      auto child = IndexToChild(index);
      if(child > heap_->maxIndex) break;
      other = GetState(child);
      if(other != Merged)
         return Corrupt(ChildStateInvalid, restart);
      other = GetState(child + 1);
      if(other != Merged)
         return Corrupt(ChildStateInvalid, restart);
      break;
   }
   }

   return state;
}
}
