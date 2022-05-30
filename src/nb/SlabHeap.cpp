//==============================================================================
//
//  SlabHeap.cpp
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
#include "SlabHeap.h"
#include <cstdint>
#include <ios>
#include <iosfwd>
#include <map>
#include <memory>
#include <vector>
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "Formatters.h"
#include "HeapCfg.h"
#include "NbTypes.h"
#include "Q2Link.h"
#include "Q2Way.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysMemory.h"
#include "SysMutex.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For tracking a memory segment allocated by the heap.
// 
struct SlabInfo
{
   SlabInfo(void* addr, size_t size) : addr_(addr), size_(size) { }

   void Print(ostream& stream) const
   {
      stream << "addr=" << addr_ << spaces(2);
      stream << "size=" << size_;
   }

   void* addr_;   // segment's address
   size_t size_;  // segment's size
};

//------------------------------------------------------------------------------
//
//  For tracking a block within a slab.
// 
struct BlockInfo
{
   BlockInfo(void* addr, size_t size, bool used) :
      addr_(addr), size_(size), used_(used) { }

   void Print(ostream& stream) const
   {
      stream << "addr=" << addr_ << spaces(2);
      stream << "size=" << size_ << spaces(2);
      stream << "used=" << used_;
   }

   void* addr_;   // block's address
   size_t size_;  // block's size
   bool used_;    // set if block is in use
};

//------------------------------------------------------------------------------
//
//  For tracking an available block.
// 
struct AvailInfo
{
   AvailInfo(void* addr, size_t size) : addr_(addr), size_(size) { }

   void Print(ostream& stream) const
   {
      stream << "addr=" << addr_ << spaces(2);
      stream << "size=" << size_;
   }

   void* addr_;   // block's address
   size_t size_;  // block's size
};

//==============================================================================
//
//> The size of a slab.
//
constexpr size_t SlabSize = 2 * MBs;

//  Types of corruption that can be detected.
//
enum SlabCorruptionReason
{
   FreeBlockNotFound  // block in avail_ not found in blocks_
};

//==============================================================================
//
//  Heap management information.
//
//  NOTE: This data is currently allocated on the default heap, even though
//  ====  it should reside in the same MemoryType as that managed by the heap.
//        This would be important for a write-protected heap, but it would mean
//        using the corresponding custom allocator (e.g. ProtectedAllocator)
//        for std::vector, std::map, and std::multimap.
//
class SlabPriv
{
public:
   //  Constructor.
   //
   explicit SlabPriv(MemoryType type);

   //  Destructor.
   //
   ~SlabPriv();

   //  Returns the type of memory managed by the heap.
   //
   MemoryType Type() const { return type_; }

   //  Allocates a block of SIZE.
   //
   void* Alloc(size_t size);

   //  If ADDR is an in-use block, returns its size, else returns 0.
   //
   size_t BlockToSize(const void* addr) const;

   //  Returns the amount of memory currently available.
   //
   size_t CurrAvail() const;

   //  Displays heap information.
   //
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const;

   //  Frees the block at ADDR.  Returns false if ADDR is not an in-use
   //  block.
   //
   bool Free(void* addr);

   //  Returns the number of bytes of heap management overhead.
   //
   size_t Overhead() const;

   //  Validates the heap.  If ADDR is not nullptr, only the block that
   //  is alleged to be at ADDR is validated.
   //
   bool Validate(const void* addr) const;
private:
   //  Adds a slab when there isn't a free block that can satisfy an
   //  allocation request.  Returns false if allocation fails.
   //
   bool Extend();

   //  Invoked when corruption is detected.  REASON specifies the type
   //  of corruption, and RESTART is set to initiate a restart.
   //
   void Corrupt(int reason, bool restart) const;

   //  The type of memory that the heap manages.
   //
   const MemoryType type_;

   //  For locking the heap during operations.
   //
   std::unique_ptr<SysMutex> mutex_;

   //  The slabs allocated for the heap.
   //
   std::vector<SlabInfo> slabs_;

   //  All blocks, sorted by address.
   //
   std::map<const void*, BlockInfo> blocks_;

   //  Available blocks, grouped by size.
   //
   std::multimap<size_t, AvailInfo> avail_;
};

typedef std::pair<const void*, BlockInfo> AddrPair;  //* to be deleted
typedef std::pair<size_t, AvailInfo> SizePair;       //* to be deleted

//------------------------------------------------------------------------------

SlabPriv::SlabPriv(MemoryType type) : type_(type)
{
   Debug::ft("SlabPriv.ctor");

   //  Allocate the mutex.
   //
   std::ostringstream stream;
   stream << "SlabLock(" << type_ << ')';
   mutex_.reset(new SysMutex(stream.str().c_str()));

   if(mutex_ == nullptr)
   {
      Restart::Initiate(RestartWarm, MutexCreationFailed, 0);
   }
}

//------------------------------------------------------------------------------

SlabPriv::~SlabPriv()
{
   mutex_->Acquire(TIMEOUT_NEVER);

   //  Free each slab.
   //
   for(auto s = slabs_.cbegin(); s != slabs_.cend(); ++s)
   {
      SysMemory::Protect(s->addr_, s->size_, MemReadWrite);
      SysMemory::Free(s->addr_, s->size_);
   }

   mutex_->Release();
}

//------------------------------------------------------------------------------

void* SlabPriv::Alloc(size_t size)
{
   Debug::ft("SlabPriv.Alloc");

   //  Find the smallest available block that can satisfy the request.
   //  If no block is available, allocate a slab and try again.
   //
   if(size > SlabSize) return nullptr;

   MutexGuard guard(mutex_.get());

   auto avail = avail_.upper_bound(size);

   if(avail == avail_.cend())
   {
      if(!Extend()) return nullptr;
      return Alloc(size);
   }

   //  We have a block.  If it has more space than required, split it
   //  and make the free portion available, and also split it within
   //  blocks_.
   //
   auto addr = avail->second.addr_;
   auto total = avail->second.size_;
   auto extra = total - size;

   auto block = blocks_.find(addr);
   if(block == blocks_.cend())
   {
      Corrupt(FreeBlockNotFound, true);
      return nullptr;
   }

   //  Remove the block that was allocated.  If it has more space than
   //  was requested, split it and make the free portion available, and
   //  also split it within blocks_.  If it has no extra space, just
   //  mark it in use within blocks_.
   //
   avail_.erase(avail);

   if(extra > 0)
   {
      auto next = (void*) (uintptr_t(addr) + size);
      avail_.insert(SizePair(extra, AvailInfo(next, extra)));

      blocks_.erase(block);
      blocks_.insert(AddrPair(addr, BlockInfo(addr, size, false)));
      blocks_.insert(AddrPair(next, BlockInfo(next, extra, true)));
   }
   else
   {
      block->second.used_ = true;
   }
}

//------------------------------------------------------------------------------

size_t SlabPriv::BlockToSize(const void* addr) const
{
   Debug::ft("SlabPriv.BlockToSize");

   auto block = blocks_.find(addr);
   if(block == blocks_.cend()) return 0;
   return (block->second.used_ ? block->second.size_ : 0);
}

//------------------------------------------------------------------------------

void SlabPriv::Corrupt(int reason, bool restart) const
{
   if(restart && !Element::RunningInLab())
      Restart::Initiate(Restart::LevelToClear(type_), HeapCorruption, reason);
   else
      Debug::SwLog("SlabPriv.Corrupt", "slab corruption", reason);
}

//------------------------------------------------------------------------------

size_t SlabPriv::CurrAvail() const
{
   size_t total = 0;

   MutexGuard guard(mutex_.get());

   for(auto a = avail_.cbegin(); a != avail_.cend(); ++a)
   {
      total += a->second.size_;
   }

   return total;
}

//------------------------------------------------------------------------------

void SlabPriv::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MutexGuard guard(mutex_.get());

   stream << prefix << "type   : " << type_ << CRLF;
   stream << prefix << "mutex  : " << CRLF;
   mutex_->Display(stream, prefix, options);

   auto verbose = options.test(DispVerbose);
   auto indent = prefix + spaces(2);

   stream << prefix << "slabs  : " << slabs_.size() << CRLF;

   if(verbose)
   {
      size_t i = 0;

      for(auto s = slabs_.cbegin(); s != slabs_.cend(); ++s, ++i)
      {
         stream << indent << strIndex(i, 0, true);
         s->Print(stream);
         stream << CRLF;
      }
   }

   stream << prefix << "free   : " << avail_.size() << CRLF;

   if(verbose)
   {
      size_t i = 0;

      for(auto a = avail_.cbegin(); a != avail_.cend(); ++a, ++i)
      {
         stream << indent << strIndex(i, 0, true);
         a->second.Print(stream);
         stream << CRLF;
      }
   }

   stream << prefix << "blocks : " << slabs_.size() << CRLF;

   if(verbose)
   {
      size_t i = 0;

      for(auto u = blocks_.cbegin(); u != blocks_.cend(); ++u, ++i)
      {
         stream << indent << strIndex(i, 0, true);
         u->second.Print(stream);
         stream << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

bool SlabPriv::Extend()
{
   Debug::ft("SlabPriv.Extend");

   //  Allocate a new slab, record it, and add it to the available blocks.
   //
   auto addr = SysMemory::Alloc(nullptr, SlabSize);
   if(addr == nullptr) return false;
   slabs_.push_back(SlabInfo(addr, SlabSize));
   avail_.insert(SizePair(SlabSize, AvailInfo(addr, SlabSize)));
   return true;
}

//------------------------------------------------------------------------------

bool SlabPriv::Free(void* addr)
{
   Debug::ft("SlabPriv.Free");

   MutexGuard guard(mutex_.get());

   auto curr = blocks_.find(addr);
   if(curr == blocks_.cend()) return false;
   if(!curr->second.used_) return false;

   //  See if CURR can merge with its predecessor and/or successor before
   //  inserting the block that will be available.
   //
   auto avail = curr->second.addr_;
   auto size = curr->second.size_;
   auto next = std::next(curr);
   auto merged = false;

   if(curr != blocks_.cbegin())
   {
      auto prev = std::prev(curr);

      if(!prev->second.used_)
      {
         //  CURR and PREV will merge, but they might also merge with NEXT.
         //  For now, erase CURR and PREV and update the address and size
         //  for the merged block.
         //
         merged = true;
         avail = prev->second.addr_;
         size += prev->second.size_;
         curr = blocks_.erase(prev);
         next = blocks_.erase(curr);
      }
   }

   if((next != blocks_.cend()) && !next->second.used_)
   {
      size += curr->second.size_;

      //  Erase NEXT.  If MERGED is set, CURR was already erased above,
      //  else it must also be erased.
      //
      if(!merged)
      {
         next = blocks_.erase(curr);
      }

      blocks_.erase(next);
   }

   //  We now have the address and size of the free block, which may
   //  have merged with its predecessor and successor.
   //
   blocks_.insert(AddrPair(avail, BlockInfo(avail, size, true)));
   avail_.insert(SizePair(size, AvailInfo(avail, size)));
   return true;
}

//------------------------------------------------------------------------------

size_t SlabPriv::Overhead() const
{
   //  This is approximate and assumes an overhead of 4 pointers per entry
   //  (left, right, parent, and color data for nodes in a red-black tree).
   //
   size_t size = sizeof(SlabPriv);
   size += slabs_.size() * sizeof(SlabInfo);
   size += avail_.size() * (4 * sizeof(void*) + sizeof(BlockInfo));
   size += blocks_.size() * (4 * sizeof(void*) + sizeof(BlockInfo));
   return size;
}

//------------------------------------------------------------------------------

bool SlabPriv::Validate(const void* addr) const
{
   Debug::ft("SlabPriv.Validate");

   //* Iterate over blocks_ to verify that all memory in slabs_ is accounted
   //  for with no gaps or overlaps, and that an unused block also appears in
   //  avail_.
   //
   return true;
}

//==============================================================================

SlabHeap::SlabHeap(MemoryType type) : Heap(),
   priv_(new SlabPriv(type))
{
   Debug::ft("SlabHeap.ctor");

   Debug::Assert(priv_ != nullptr);
}

//------------------------------------------------------------------------------

SlabHeap::~SlabHeap()
{
   Debug::ftnt("SlabHeap.dtor");
}

//------------------------------------------------------------------------------

void* SlabHeap::Alloc(size_t size)
{
   Debug::ft("SlabHeap.Alloc");

   auto addr = priv_->Alloc(size);
   Requested(size, addr);
   return addr;
}

//------------------------------------------------------------------------------

size_t SlabHeap::BlockToSize(const void* addr) const
{
   Debug::ft("SlabHeap.BlockToSize");

   return priv_->BlockToSize(addr);
}

//------------------------------------------------------------------------------

size_t SlabHeap::CurrAvail() const
{
   return priv_->CurrAvail();
}

//------------------------------------------------------------------------------

void SlabHeap::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Heap::Display(stream, prefix, options);

   stream << prefix << "priv : " << priv_.get() << CRLF;

   if(priv_ != nullptr)
   {
      priv_->Display(stream, prefix, options);
   }
}

//------------------------------------------------------------------------------

void SlabHeap::Free(void* addr)
{
   Debug::ft("SlabHeap.Free");

   auto size = BlockToSize(addr);
   if(size == 0) return;

   if(priv_->Free(addr))
   {
      Freeing(addr, size);
   }
}

//------------------------------------------------------------------------------

size_t SlabHeap::Overhead() const
{
   return priv_->Overhead();
}

//------------------------------------------------------------------------------

void SlabHeap::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

MemoryType SlabHeap::Type() const
{
   return priv_->Type();
}

//------------------------------------------------------------------------------

bool SlabHeap::Validate(const void* addr) const
{
   Debug::ft("SlabHeap.Validate");

   return priv_->Validate(addr);
}
}
