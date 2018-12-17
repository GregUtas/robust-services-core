//==============================================================================
//
//  ObjectPool.cpp
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
#include "ObjectPool.h"
#include "CfgIntParm.h"
#include "Dynamic.h"
#include <sstream>
#include "Algorithms.h"
#include "AllocationException.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "Log.h"
#include "Memory.h"
#include "ObjectPoolRegistry.h"
#include "ObjectPoolTrace.h"
#include "Pooled.h"
#include "Q1Link.h"
#include "Restart.h"
#include "Singleton.h"
#include "Statistics.h"
#include "ThisThread.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The header for a Pooled (a block in the pool).  Data in the header
//  is not nullified when an object is deleted.
//
struct BlockHeader
{
   ObjectPoolId pid : 8;       // the pool to which the block belongs
   PooledObjectSeqNo seq : 8;  // the block's sequence number
   static const size_t Size;   // the size of a BlockHeader
};

//  This struct references the block for a Pooled and the location
//  where the actual object begins.
//
struct ObjectBlock
{
   BlockHeader header;  // block management information
   Pooled obj;          // the actual location of the object
};

//  The configuration parameter for an object pool, which expands
//  the pool's size if the pool was created *before* its tuple was
//  read from the element configuration file.
//
class ObjectPoolSizeCfg : public CfgIntParm
{
public:
   explicit ObjectPoolSizeCfg(ObjectPool* pool);
   ~ObjectPoolSizeCfg();
protected:
   virtual void SetCurr() override;
private:
   ObjectPool* pool_;
};

//  Statistics for each object pool.
//
class ObjectPoolStats : public Dynamic
{
public:
   ObjectPoolStats();
   ~ObjectPoolStats();

   LowWatermarkPtr lowCount_;
   CounterPtr      allocCount_;
   CounterPtr      freeCount_;
   CounterPtr      failCount_;
   CounterPtr      auditCount_;
   LowWatermarkPtr lowExcess_;
};

//==============================================================================

const size_t BlockHeader::Size = Memory::Align(sizeof(BlockHeader));

//==============================================================================

fn_name ObjectPoolSizeCfg_ctor = "ObjectPoolSizeCfg.ctor";

ObjectPoolSizeCfg::ObjectPoolSizeCfg(ObjectPool* pool) :
   CfgIntParm(pool->key_.c_str(), "1", &pool->targSegments_, 0,
      ObjectPool::MaxSegments, "number of segments of 1K objects"),
   pool_(pool)
{
   Debug::ft(ObjectPoolSizeCfg_ctor);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolSizeCfg_dtor = "ObjectPoolSizeCfg.dtor";

ObjectPoolSizeCfg::~ObjectPoolSizeCfg()
{
   Debug::ft(ObjectPoolSizeCfg_dtor);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolSizeCfg_SetCurr = "ObjectPoolSizeCfg.SetCurr";

void ObjectPoolSizeCfg::SetCurr()
{
   Debug::ft(ObjectPoolSizeCfg_SetCurr);

   CfgIntParm::SetCurr();

   //  If the pool contains no blocks, it is currently being constructed,
   //  so do nothing.  But if it already contains blocks, expand its size
   //  to the new value.
   //
   if(pool_->currSegments_ > 0) pool_->AllocBlocks();
}

//==============================================================================

fn_name ObjectPoolStats_ctor = "ObjectPoolStats.ctor";

ObjectPoolStats::ObjectPoolStats()
{
   Debug::ft(ObjectPoolStats_ctor);

   lowCount_.reset(new LowWatermark("fewest remaining blocks"));
   allocCount_.reset(new Counter("successful allocations"));
   freeCount_.reset(new Counter("deallocations"));
   failCount_.reset(new Counter("unsuccessful allocations"));
   auditCount_.reset(new Counter("blocks recovered by audit"));
   lowExcess_.reset(new LowWatermark("size of block minus largest object"));
}

//------------------------------------------------------------------------------

fn_name ObjectPoolStats_dtor = "ObjectPoolStats.dtor";

ObjectPoolStats::~ObjectPoolStats()
{
   Debug::ft(ObjectPoolStats_dtor);
}

//==============================================================================

const uint8_t ObjectPool::OrphanThreshold = 2;

fn_name ObjectPool_ctor = "ObjectPool.ctor";

ObjectPool::ObjectPool
   (ObjectPoolId pid, MemoryType type, size_t nBytes, const string& name) :
   name_(name),
   key_("NumOf" + name),
   type_(type),
   blockSize_(0),
   segIncr_(0),
   segSize_(0),
   currSegments_(0),
   targSegments_(1),
   cfgSegments_(nullptr),
   availCount_(0),
   corruptQHead_(false)
{
   Debug::ft(ObjectPool_ctor);

   //  The block size must account for the header above each Pooled object.
   //
   blockSize_ = Memory::Align(nBytes) + Memory::Align(BlockHeader::Size);
   segIncr_ = blockSize_ >> BYTES_PER_WORD_LOG2;
   segSize_ = segIncr_ * ObjectsPerSegment;

   pid_.SetId(pid);
   freeq_.Init(Pooled::LinkDiff());

   for(auto i = 0; i < MaxSegments; ++i) blocks_[i] = nullptr;

   stats_.reset(new ObjectPoolStats);
   cfgSegments_.reset(new ObjectPoolSizeCfg(this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*cfgSegments_);

   Singleton< ObjectPoolRegistry >::Instance()->BindPool(*this);
}

//------------------------------------------------------------------------------

fn_name ObjectPool_dtor = "ObjectPool.dtor";

ObjectPool::~ObjectPool()
{
   Debug::ft(ObjectPool_dtor);

   for(size_t i = 0; i < currSegments_; ++i)
   {
      Memory::Free(blocks_[i]);
      blocks_[i] = nullptr;
   }

   Singleton< ObjectPoolRegistry >::Instance()->UnbindPool(*this);
}

//------------------------------------------------------------------------------

fn_name ObjectPool_AllocBlocks = "ObjectPool.AllocBlocks";

bool ObjectPool::AllocBlocks()
{
   Debug::ft(ObjectPool_AllocBlocks);

   while(currSegments_ < targSegments_)
   {
      auto pid = Pid();
      auto size = sizeof(uword) * segSize_;
      blocks_[currSegments_] = (uword*) Memory::Alloc(size, type_, false);

      if(blocks_[currSegments_] == nullptr)
      {
         auto log = Log::Create("OBJECT POOL EXPANSION FAILED");

         if(log != nullptr)
         {
            *log << "pool=" << int(pid);
            *log << " target=" << targSegments_;
            *log << " actual=" << currSegments_ << CRLF;
            Log::Spool(log);
         }

         return false;
      }

      auto bid = currSegments_ << ObjectsPerSegmentLog2;
      ++currSegments_;
      auto seg = blocks_[currSegments_ - 1];

      for(size_t j = 0; j < segSize_; j += segIncr_)
      {
         auto b = (ObjectBlock*) &seg[j];
         ++bid;
         b->header.pid = pid;
         b->header.seq = 0;
         b->obj.link_.next = nullptr;
         b->obj.assigned_ = false;
         b->obj.orphaned_ = OrphanThreshold;
         EnqBlock(&b->obj, false);
      }
   }

   return true;
}

//------------------------------------------------------------------------------

size_t ObjectPool::AllocCount() const
{
   return stats_->allocCount_->Curr();
}

//------------------------------------------------------------------------------

fn_name ObjectPool_AuditFreeq = "ObjectPool.AuditFreeq";

void ObjectPool::AuditFreeq()
{
   Debug::ft(ObjectPool_AuditFreeq);

   size_t count = 0;

   //  Increment all orphan counts.  The free queue is checked immediately
   //  after marking the blocks so that if the traversal finds an unmarked
   //  block, it knows that the previous block has a bad pointer (either
   //  back to an earlier point in the queue or to something that isn't a
   //  block in the pool).
   //
   //  NOTE: The buffer is locked here because, if trace wraparound occurs,
   //  ====  a trace record's destructor might return a block to the pool.
   //        Such a block will have a zero orphaned_ count (see EnqBlock),
   //        which will cause us to believe that the queue is corrupt.
   //
   auto buff = Singleton< TraceBuffer >::Instance();

   buff->Lock();
   {
      for(size_t i = 0; i < currSegments_; ++i)
      {
         auto seg = blocks_[i];

         for(size_t j = 0; j < segSize_; j += segIncr_)
         {
            auto b = (ObjectBlock*) &seg[j];
            ++b->obj.orphaned_;
         }
      }

      //  Audit the free queue unless it is empty.  The audit checks that
      //  the links are sane and that the count of free blocks is correct.
      //
      auto diff = Pooled::LinkDiff();
      auto item = freeq_.tail_.next;

      if(item != nullptr)
      {
         //  Audit the queue header (when PREV == nullptr), then the queue.
         //  The queue header references the tail element, so the tail is
         //  the first block whose link is audited (when CURR == freeq_).
         //  The next block to be audited (when PREV == freeq_) is the head
         //  element, which follows the tail.  The entire queue has been
         //  traversed when CURR == freeq_ (the tail) for the second time.
         //
         //  Before a link (CURR) is followed, the item (queue header or
         //  block) that provided the link is marked as corrupt.  If the
         //  link is bad, a trap should occur at curr->orphaned_ = 0.
         //  Thus, if we get past that point in the code, the link should
         //  be sane, and so its owner's "corrupt" flag is cleared before
         //  continuing down the queue.
         //
         //  If a trap occurs, this code is reentered.  It starts traversing
         //  the queue again.  Eventually it reaches an item whose corrupt_
         //  flag *is already set*, at which point the queue gets truncated.
         //
         size_t maxCount = currSegments_ * ObjectsPerSegment;
         Pooled* prev = nullptr;
         auto badLink = false;

         while(count <= maxCount)
         {
            auto curr = (Pooled*) getptr1(item, diff);

            if(prev == nullptr)
            {
               if(corruptQHead_)
                  badLink = true;
               else
                  corruptQHead_ = true;
            }
            else
            {
               if(prev->corrupt_)
                  badLink = true;
               else
                  prev->corrupt_ = true;
            }

            //  If this block isn't orphaned, PREV's link is corrupt.  It
            //  could be pointing back into the middle of the queue, or it
            //  could be a random but legal address at which the offset of
            //  orphaned_ is zero.
            //
            if(!badLink) badLink = (curr->orphaned_ == 0);

            //  If a bad link was detected, generate a log and truncate the
            //  queue.
            //
            if(badLink)
            {
               auto log = Log::Create("OBJECT POOL QUEUE CORRUPT");

               if(log != nullptr)
               {
                  *log << "pool=" << int(Pid());
                  *log << " available=" << availCount_;
                  *log << " revised=" << count << CRLF;
                  Log::Spool(log);
               }

               if(prev == nullptr)
               {
                  corruptQHead_ = false;
                  freeq_.Init(Pooled::LinkDiff());
                  availCount_ = 0;
               }
               else
               {
                  prev->corrupt_ = false;
                  prev->link_.next = freeq_.tail_.next;  // tail now after PREV
                  availCount_ = count;
               }

               buff->Unlock();
               return;
            }

            curr->orphaned_ = 0;
            ++count;

            if(prev == nullptr)
               corruptQHead_ = false;
            else
               prev->corrupt_ = false;

            prev = curr;
            item = item->next;

            if(freeq_.tail_.next == item) break;  // reached tail again
         }
      }
   }
   buff->Unlock();

   //  The queue has been traversed.  Check the free count before returning.
   //
   if(availCount_ != count)
   {
      auto log = Log::Create("OBJECT POOL QUEUE COUNT INCORRECT");

      if(log != nullptr)
      {
         *log << "pool=" << int(Pid());
         *log << " available=" << availCount_;
         *log << " revised=" << count << CRLF;
         Log::Spool(log);
      }

      availCount_ = count;
   }
}

//------------------------------------------------------------------------------

bool ObjectPool::BidToIndices(Bid bid, size_t& i, size_t& j) const
{
   if(bid == NIL_ID) return false;

   --bid;
   i = bid >> ObjectsPerSegmentLog2;
   if(i >= currSegments_) return false;

   j = (bid & ObjectSecondIndexMask) * segIncr_;
   return true;
}

//------------------------------------------------------------------------------

fn_name ObjectPool_BidToObj = "ObjectPool.BidToObj";

Pooled* ObjectPool::BidToObj(Bid bid) const
{
   Debug::ft(ObjectPool_BidToObj);

   if(bid == NIL_ID) return nullptr;

   --bid;

   auto b = (bid == NIL_ID ? First(bid) : Next(bid));
   if((b != nullptr) && b->obj.assigned_) return &b->obj;
   return nullptr;
}

//------------------------------------------------------------------------------

ptrdiff_t ObjectPool::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const ObjectPool* >(&local);
   return ptrdiff(&fake->pid_, fake);
}

//------------------------------------------------------------------------------

fn_name ObjectPool_Corrupt = "ObjectPool.Corrupt";

bool ObjectPool::Corrupt(size_t n)
{
   Debug::ft(ObjectPool_Corrupt);

   if(!Element::RunningInLab()) return false;

   if((n == 0) || freeq_.Empty())
   {
      freeq_.Corrupt(nullptr);
   }
   else
   {
      auto p = freeq_.First();

      while((p != nullptr) && (n > 1))
      {
         freeq_.Next(p);
         --n;
      }

      if(p == nullptr) return false;
      freeq_.Corrupt(p);
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name ObjectPool_DeqBlock = "ObjectPool.DeqBlock";

Pooled* ObjectPool::DeqBlock(size_t size)
{
   Debug::ft(ObjectPool_DeqBlock);

   auto maxsize = blockSize_ - BlockHeader::Size;

   if(size > maxsize)
   {
      Debug::SwLog(ObjectPool_DeqBlock, Pid(), size);
      throw AllocationException(type_, size);
   }

   stats_->lowExcess_->Update(maxsize - size);

   auto item = freeq_.Deq();

   if(item == nullptr)
   {
      stats_->failCount_->Incr();
      throw AllocationException(type_, size);
   }

   --availCount_;
   stats_->lowCount_->Update(availCount_);
   stats_->allocCount_->Incr();

   if(Debug::TraceOn())
   {
      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ObjPoolTracer))
      {
         new ObjectPoolTrace(ObjectPoolTrace::Dequeued, *item);
      }
   }

   return item;
}

//------------------------------------------------------------------------------

void ObjectPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "pid          : " << pid_.to_str() << CRLF;
   stream << prefix << "name         : " << name_ << CRLF;
   stream << prefix << "key          : " << key_ << CRLF;
   stream << prefix << "type         : " << type_ << CRLF;
   stream << prefix << "blockSize    : " << blockSize_ << CRLF;
   stream << prefix << "segIncr      : " << segIncr_ << CRLF;
   stream << prefix << "segSize      : " << segSize_ << CRLF;
   stream << prefix << "currSegments : " << currSegments_ << CRLF;
   stream << prefix << "targSegments : " << targSegments_ << CRLF;
   stream << prefix << "cfgSegments  : ";
   stream << strObj(cfgSegments_.get()) << CRLF;
   stream << prefix << "availCount   : " << availCount_ << CRLF;
   stream << prefix << "corruptQHead : " << corruptQHead_ << CRLF;

   auto lead = prefix + spaces(2);
   stream << prefix << "blocks [segment]" << CRLF;

   for(size_t i = 0; i < currSegments_; ++i)
   {
      stream << lead << strIndex(i) << blocks_[i] << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name ObjectPool_DisplayStats = "ObjectPool.DisplayStats";

void ObjectPool::DisplayStats(ostream& stream) const
{
   Debug::ft(ObjectPool_DisplayStats);

   stream << spaces(2) << name_ << SPACE << strIndex(Pid(), 0, false) << CRLF;

   stats_->lowCount_->DisplayStat(stream);
   stats_->allocCount_->DisplayStat(stream);
   stats_->freeCount_->DisplayStat(stream);
   stats_->failCount_->DisplayStat(stream);
   stats_->auditCount_->DisplayStat(stream);
   stats_->lowExcess_->DisplayStat(stream);
}

//------------------------------------------------------------------------------

bool ObjectPool::DisplayUsed(ostream& stream,
      const string& prefix, const Flags& options) const
{
   Bid bid;
   auto time = 200;
   auto count = 0;

   for(auto obj = FirstUsed(bid); obj != nullptr; obj = NextUsed(bid))
   {
      ++count;

      if(options.test(DispVerbose))
      {
         obj->Display(stream, prefix, NoFlags);
         time -= 25;
      }
      else
      {
         stream << prefix << strObj(obj) << CRLF;
         --time;
      }

      if(time <= 0)
      {
         ThisThread::Pause();
         time = 200;
      }
   }

   return (count > 0);
}

//------------------------------------------------------------------------------

fn_name ObjectPool_EnqBlock = "ObjectPool.EnqBlock";

void ObjectPool::EnqBlock(Pooled* obj, bool deleted)
{
   if(deleted) Debug::ft(ObjectPool_EnqBlock);

   if(obj == nullptr) return;

   if(Debug::TraceOn() && deleted)
   {
      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ObjPoolTracer))
      {
         new ObjectPoolTrace(ObjectPoolTrace::Enqueued, *obj);
      }
   }

   //  If a block is already on the free queue or another queue, putting it
   //  on the free queue creates a mess.
   //
   if(!obj->assigned_)
   {
      if(obj->orphaned_ == 0)
      {
         Debug::SwLog(ObjectPool_EnqBlock, debug64_t(obj), pack2(Pid(), 0));
         return;
      }
   }
   else if(obj->link_.next != nullptr)
   {
      Debug::SwLog(ObjectPool_EnqBlock, debug64_t(obj), pack2(Pid(), 1));
      return;
   }

   auto nullify = ObjectPoolRegistry::NullifyObjectData();
   obj->Nullify(nullify ? blockSize_ - BlockHeader::Size : 0);

   auto block = ObjToBlock(obj);

   if(block->header.seq == MaxSeqNo)
      block->header.seq = 1;
   else
      ++block->header.seq;

   obj->link_.next = nullptr;
   obj->assigned_ = false;
   obj->orphaned_ = 0;
   obj->corrupt_ = false;
   obj->logged_ = false;

   if(!freeq_.Enq(*obj))
   {
      Debug::SwLog(ObjectPool_EnqBlock, debug64_t(obj), pack2(Pid(), 2));
      return;
   }

   ++availCount_;
   if(deleted) stats_->freeCount_->Incr();
}

//------------------------------------------------------------------------------

size_t ObjectPool::FailCount() const
{
   return stats_->failCount_->Curr();
}

//------------------------------------------------------------------------------

fn_name ObjectPool_First = "ObjectPool.First";

ObjectBlock* ObjectPool::First(Bid& bid) const
{
   Debug::ft(ObjectPool_First);

   if(currSegments_ > 0)
   {
      bid = 1;
      return (ObjectBlock*) &blocks_[0][0];
   }

   bid = NIL_ID;
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ObjectPool_FirstUsed = "ObjectPool.FirstUsed";

Pooled* ObjectPool::FirstUsed(Bid& bid) const
{
   Debug::ft(ObjectPool_FirstUsed);

   auto b = First(bid);

   if(b != nullptr)
   {
      if(b->obj.assigned_) return &b->obj;
      return NextUsed(bid);
   }

   bid = NIL_ID;
   return nullptr;
}

//------------------------------------------------------------------------------

size_t ObjectPool::FreeCount() const
{
   return stats_->freeCount_->Curr();
}

//------------------------------------------------------------------------------

bool ObjectPool::IndicesToBid(size_t i, size_t j, Bid& bid) const
{
   if(i >= currSegments_) return false;
   if(j >= segSize_) return false;
   if((j % segIncr_) != 0) return false;

   bid = (i << ObjectsPerSegmentLog2) + (j / segIncr_) + 1;
   return true;
}

//------------------------------------------------------------------------------

size_t ObjectPool::InUseCount() const
{
   return (currSegments_ * ObjectsPerSegment) - availCount_;
}

//------------------------------------------------------------------------------

size_t ObjectPool::LowAvailCount() const
{
   return stats_->lowCount_->Curr();
}

//------------------------------------------------------------------------------

ObjectBlock* ObjectPool::Next(Bid& bid) const
{
   size_t i, j;

   ++bid;

   if(BidToIndices(bid, i, j))
   {
      return (ObjectBlock*) &blocks_[i][j];
   }

   bid = NIL_ID;
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ObjectPool_NextUsed = "ObjectPool.NextUsed";

Pooled* ObjectPool::NextUsed(Bid& bid) const
{
   size_t m, n;

   ++bid;

   if(BidToIndices(bid, m, n))
   {
      for(auto i = m; i < currSegments_; ++i)
      {
         auto seg = blocks_[i];

         for(auto j = (i == m ? n : 0); j < segSize_; j += segIncr_)
         {
            auto b = (ObjectBlock*) &seg[j];

            if(b->obj.assigned_)
            {
               if(IndicesToBid(i, j, bid)) return &b->obj;
               Debug::SwLog(ObjectPool_NextUsed, i, j);
               return nullptr;
            }
         }
      }
   }

   bid = NIL_ID;
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ObjectPool_ObjBid = "ObjectPool.ObjBid";

PooledObjectId ObjectPool::ObjBid(const Pooled* obj, bool inUseOnly) const
{
   Debug::ft(ObjectPool_ObjBid);

   if(obj == nullptr) return NIL_ID;
   if(inUseOnly && !obj->assigned_) return NIL_ID;

   //  Find BLOCK, which houses OBJ and is the address that we'll look for.
   //  Search through each segment of blocks.  If BLOCK is within MAXDIFF
   //  distance of the first block in a segment, it should belong to that
   //  segment, as long as it actually references a block boundary.
   //
   auto block = (const_ptr_t) ObjToBlock(obj);
   auto maxdiff = (ptrdiff_t) (blockSize_ * (ObjectsPerSegment - 1));

   for(size_t i = 0; i < currSegments_; ++i)
   {
      auto b0 = (const_ptr_t) &blocks_[i][0];

      if(block >= b0)
      {
         ptrdiff_t diff = block - b0;

         if(diff <= maxdiff)
         {
            if(diff % blockSize_ == 0)
            {
               auto j = diff / blockSize_;
               return (i << ObjectsPerSegmentLog2) + j + 1;
            }
         }
      }
   }

   return NIL_ID;
}

//------------------------------------------------------------------------------

ObjectPoolId ObjectPool::ObjPid(const Pooled* obj)
{
   auto block = ObjToBlock(obj);
   if(block != nullptr) return block->header.pid;
   return NIL_ID;
}

//------------------------------------------------------------------------------

PooledObjectSeqNo ObjectPool::ObjSeq(const Pooled* obj)
{
   auto block = ObjToBlock(obj);
   if(block != nullptr) return block->header.seq;
   return 0;
}

//------------------------------------------------------------------------------

ObjectBlock* ObjectPool::ObjToBlock(const Pooled* obj)
{
   if(obj == nullptr) return nullptr;
   return (ObjectBlock*) getptr1(obj, BlockHeader::Size);
}

//------------------------------------------------------------------------------

void ObjectPool::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ObjectPool_RecoverBlocks = "ObjectPool.RecoverBlocks";

void ObjectPool::RecoverBlocks()
{
   Debug::ft(ObjectPool_RecoverBlocks);

   auto pid = Pid();
   auto buff = Singleton< TraceBuffer >::Instance();
   size_t count = 0;

   //  Run through all of the blocks, recovering orphans.
   //
   for(size_t i = 0; i < currSegments_; ++i)
   {
      auto seg = blocks_[i];

      for(size_t j = 0; j < segSize_; j += segIncr_)
      {
         auto b = (ObjectBlock*) &seg[j];
         auto p = &b->obj;

         if(p->orphaned_ >= OrphanThreshold)
         {
            //  Generate a log if the block is in use (don't bother with
            //  free queue orphans) and it hasn't been logged yet (which
            //  can happen if we reenter this code after a trap).
            //
            ++count;

            if(Debug::TraceOn())
            {
               if(buff->ToolIsOn(ObjPoolTracer))
               {
                  new ObjectPoolTrace(ObjectPoolTrace::Recovered, *p);
               }
            }

            if(p->assigned_ && !p->logged_ && (count <= 4))
            {
               auto log = Log::Create("OBJECT RECOVERED");

               if(log != nullptr)
               {
                  *log << "pool=" << int(pid) << CRLF;
                  p->logged_ = true;
                  p->Display(*log, EMPTY_STR, Flags(Vb_Mask));
                  Log::Spool(log);
               }
            }

            //  When an in-use orphan is found, we mark it corrupt and clean
            //  it up.  If it so corrupt that it causes an exception during
            //  cleanup, this code is reentered and encounters the block again.
            //  It will then already be marked as corrupt, in which case it
            //  will simply be returned to the free queue.
            //
            if(p->assigned_ && !p->corrupt_)
            {
               p->corrupt_ = true;
               p->Cleanup();
            }

            b->header.pid = pid;
            p->link_.next = nullptr;
            EnqBlock(p, false);
            stats_->auditCount_->Incr();
         }
      }
   }

   if(count > 0)
   {
      auto log = Log::Create("OBJECTS RECOVERED");

      if(log != nullptr)
      {
         *log << "pool=" << int(pid);
         *log << " count=" << count << CRLF;

         Log::Spool(log);
      }
   }
}

//------------------------------------------------------------------------------

fn_name ObjectPool_Shutdown = "ObjectPool.Shutdown";

void ObjectPool::Shutdown(RestartLevel level)
{
   Debug::ft(ObjectPool_Shutdown);

   switch(level)
   {
   case RestartCold:
      stats_.release();
      if((type_ == MemTemp) || (type_ == MemDyn)) break;
      return;
   case RestartWarm:
      if(type_ == MemTemp) break;
      return;
   default:
      return;
   }

   for(auto i = 0; i < MaxSegments; ++i) blocks_[i] = nullptr;
   freeq_.Init(Pooled::LinkDiff());
   currSegments_ = 0;
   availCount_ = 0;
   corruptQHead_ = false;
}

//------------------------------------------------------------------------------

fn_name ObjectPool_Startup = "ObjectPool.Startup";

void ObjectPool::Startup(RestartLevel level)
{
   Debug::ft(ObjectPool_Startup);

   if(stats_ == nullptr) stats_.reset(new ObjectPoolStats);

   if(!AllocBlocks())
   {
      Restart::Initiate(SystemOutOfMemory, Pid());
   }
}
}
