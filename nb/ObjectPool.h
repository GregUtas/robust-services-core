//==============================================================================
//
//  ObjectPool.h
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
#ifndef OBJECTPOOL_H_INCLUDED
#define OBJECTPOOL_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Alarm;
   struct ObjectBlock;
   struct ObjectPoolDynamic;
   class ObjectPoolStats;
   class Pooled;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  An object pool allocates blocks during system initialization.  The blocks
//  are placed on a free queue and are dequeued at run-time to provide memory
//  for instantiating PooledObjects.  To simplify the engineering of pool
//  sizes, all objects subclassed from a common application framework class
//  should draw their blocks from the same pool.
//
class ObjectPool : public Protected
{
   friend class ObjectPoolRegistry;
   friend class ObjectPoolSizeCfg;
   friend class Registry< ObjectPool >;
public:
   //  Deleted to prohibit copying.
   //
   ObjectPool(const ObjectPool& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ObjectPool& operator=(const ObjectPool& that) = delete;

   //> Highest valid object pool identifier.
   //
   static const ObjectPoolId MaxId;

   //> The maximum number of segments in an object pool.
   //
   static const size_t MaxSegments = 256;

   //  Blocks for pooled objects are allocated in segments of 1K blocks.
   //
   static const size_t ObjectsPerSegment = 1024;

   //  Used in a shift (>>) operation to find the segment to which a block
   //  belongs.
   //
   static const size_t ObjectsPerSegmentLog2 = 10;

   //> Highest valid sequence number.  Sequence numbers distinguish a block's
   //  incarnations.  Their use is mandatory when a Pooled object can receive
   //  interprocessor messages, as they allow stale messages to be detected
   //  and discarded.
   //
   static const PooledObjectSeqNo MaxSeqNo = UINT8_MAX;

   //  Returns the pool's name.
   //
   fixed_string Name() const { return name_.c_str(); }

   //  Returns the pool's identifier.
   //
   ObjectPoolId Pid() const { return ObjectPoolId(pid_.GetId()); }

   //  Allocates a block from the free queue.  SIZE specifies the size
   //  of the object to be constructed within the block.
   //
   virtual Pooled* DeqBlock(size_t size);

   //  Returns an object's block to the free queue.  DELETED is set if
   //  the block was freed by an implementation of operator delete.
   //
   virtual void EnqBlock(Pooled* obj, bool deleted);

   //  Returns the pool's first in-use block and updates the iterator
   //  BID to reference it.
   //
   Pooled* FirstUsed(PooledObjectId& bid) const;

   //  Returns the pool's next in-use block after the one referenced
   //  by the iterator BID.
   //
   Pooled* NextUsed(PooledObjectId& bid) const;

   //  Converts OBJ to an object block identifier.  Returns -1 if OBJ does
   //  not reference a block in the pool or if inUseOnly is true and the
   //  block is not currently unassigned.
   //
   PooledObjectId ObjBid(const Pooled* obj, bool inUseOnly) const;

   //  Returns the pool to which an object belongs.
   //
   static ObjectPoolId ObjPid(const Pooled* obj);

   //  Returns an object's sequence number.
   //
   static PooledObjectSeqNo ObjSeq(const Pooled* obj);

   //  Returns a pointer to the object identified by BID.  Returns nullptr if
   //  BID is invalid or the block identified by BID is currently unassigned.
   //
   Pooled* BidToObj(PooledObjectId bid) const;

   //  Returns the total number of blocks on the free queue.
   //
   size_t AvailCount() const;

   //  Returns the total number of blocks currently in use.
   //
   size_t InUseCount() const;

   //  Returns the minimum number of available blocks during the
   //  current statistics interval.
   //
   size_t LowAvailCount() const;

   //  Returns the number of allocation failures during the current
   //  statistics interval.
   //
   size_t FailCount() const;

   //  Returns the number of allocations.
   //
   size_t AllocCount() const;

   //  Returns the number of deallocations.
   //
   size_t FreeCount() const;

   //  Returns the number times the pool was expanded.
   //
   size_t Expansions() const;

   //  Returns the type of memory used by the pool's blocks.
   //
   MemoryType BlockType() const { return mem_; }

   //  Displays statistics.  May be overridden to include pool-specific
   //  statistics, but the base class version must be invoked.
   //
   virtual void DisplayStats(std::ostream& stream, const Flags& options) const;

   //  Displays in-use blocks.  Returns false if no block were in use.
   //
   bool DisplayUsed(std::ostream& stream,
      const std::string& prefix, const Flags& options) const;

   //  Corrupts the Nth link on the free queue for testing (0 = queue header).
   //  Returns false if the queue contained less than N elements.
   //
   bool Corrupt(size_t n);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;
protected:
   //  Defines a pool, identified by NAME and PID, that allocates blocks of
   //  type MEM and SIZE bytes.  Protected because this class is virtual.
   //
   ObjectPool(ObjectPoolId pid, MemoryType mem,
      size_t size, const std::string& name);

   //  Frees all blocks.  Protected because subclasses should be singletons.
   //
   virtual ~ObjectPool();
private:
   //  Used in a mask (&) operation to find a block's offset in its segment.
   //
   static const size_t ObjectSecondIndexMask = ObjectsPerSegment - 1;

   //  Creates or expands the object pool so that it contains the target
   //  number of segments.  A pool's size can be increased at run time,
   //  but it can only be decreased during a restart.
   //
   bool AllocBlocks();

   //  Returns the first block in the pool and updates the iterator BID
   //  to reference it.
   //
   ObjectBlock* First(PooledObjectId& bid) const;

   //  Returns the next block in the pool after the one referenced by the
   //  iterator BID.
   //
   ObjectBlock* Next(PooledObjectId& bid) const;

   //  Maps the block identifier BID to the first and second level indices
   //  that access it in blocks_.  Returns false if BID does not reference
   //  a block in the pool.
   //
   bool BidToIndices(PooledObjectId bid, size_t& i, size_t& j) const;

   //  Maps the first and second level indices, I and J, to the block
   //  identifier BID.  Returns false if I or J is invalid.
   //
   bool IndicesToBid(size_t i, size_t j, PooledObjectId& bid) const;

   //  Ensures that the low availability alarm exists.
   //
   void EnsureAlarm();

   //  Updates the status of the low availability alarm.
   //
   void UpdateAlarm();

   //  Marks all blocks as orphaned and audits the free queue for sanity,
   //  unmarking its blocks so that they will not be recovered.
   //
   void AuditFreeq();

   //  Recovers orphaned blocks after AuditFreeq and ClaimBlocks have
   //  marked all in-use and free blocks.
   //
   void RecoverBlocks();

   //  Accesses the header that resides above OBJ.
   //
   static ObjectBlock* ObjToBlock(const Pooled* obj);

   //  Returns the offset to pid_.
   //
   static ptrdiff_t CellDiff();

   //  The pool's identifier.
   //
   RegCell pid_;

   //  The pool's name.
   //
   const ProtectedStr name_;

   //  The string "NumOf" + name_, which identifies (in the element
   //  configuration file) the parameter that determines the number
   //  of blocks in the pool.
   //
   const ProtectedStr key_;

   //  The type of memory used for blocks in the pool.
   //
   const MemoryType mem_;

   //  The size of each block in bytes, rounded up for alignment purposes.
   //
   size_t blockSize_;

   //  The increment used when iterating through the blocks in a segment.
   //
   size_t segIncr_;

   //  The size of a segment in bytes (and therefore the first out-of-bounds
   //  index when iterating through blocks in the segment).
   //
   size_t segSize_;

   //  The current number of segments in the pool.
   //
   size_t currSegments_;

   //  The configuration parameter for the number of segments in the pool.
   //
   CfgIntParmPtr targSegmentsCfg_;

   //  All of the blocks in the pool, allocated in segments.
   //
   uword* blocks_[MaxSegments];

   //  The alarm raised when the percentage of blocks in use is high.
   //
   Alarm* alarm_;

   //  Data that changes too frequently to unprotect and reprotect memory
   //  when it needs to be modified.
   //
   std::unique_ptr< ObjectPoolDynamic > dyn_;

   //  The pool's statistics.
   //
   std::unique_ptr< ObjectPoolStats > stats_;

   //> The number of audit cycles over which a block must be unclaimed
   //  before it is recovered.
   //
   static const uint8_t OrphanThreshold;

   //> The maximum number of logs that display the contents of an orphaned
   //  block in a given pool during each audit cycle.
   //
   static const size_t OrphanMaxLogs;
};
}
#endif
