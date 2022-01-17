//==============================================================================
//
//  ObjectPool.cpp
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
#include "ObjectPool.h"
#include "CfgIntParm.h"
#include "Dynamic.h"
#include "Persistent.h"
#include <bitset>
#include <new>
#include <sstream>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "AllocationException.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Log.h"
#include "Memory.h"
#include "NbLogs.h"
#include "ObjectPoolRegistry.h"
#include "ObjectPoolTrace.h"
#include "Pooled.h"
#include "Q1Link.h"
#include "Q1Way.h"
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
};

//  This struct references the block for a Pooled and the location
//  where the actual object begins.
//
struct ObjectBlock
{
   BlockHeader header;  // block management information
   Pooled obj;          // the actual location of the object
};

constexpr size_t BlockHeaderSize = sizeof(ObjectBlock) - sizeof(Pooled);

//------------------------------------------------------------------------------
//
//  Accesses the header that resides above OBJ.
//
static ObjectBlock* ObjToBlock(const Pooled* obj)
{
   if(obj == nullptr) return nullptr;
   return (ObjectBlock*) getptr1(obj, BlockHeaderSize);
}

//==============================================================================
//
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
   void SetCurr() override;
private:
   RestartLevel RestartRequired() const override;
   ObjectPool* const pool_;
};

//------------------------------------------------------------------------------

ObjectPoolSizeCfg::ObjectPoolSizeCfg(ObjectPool* pool) :
   CfgIntParm(pool->key_.c_str(), "1", 0,
      ObjectPool::MaxSegments, "number of segments of 1K objects"),
   pool_(pool)
{
   Debug::ft("ObjectPoolSizeCfg.ctor");
}

//------------------------------------------------------------------------------

fn_name ObjectPoolSizeCfg_dtor = "ObjectPoolSizeCfg.dtor";

ObjectPoolSizeCfg::~ObjectPoolSizeCfg()
{
   Debug::ftnt(ObjectPoolSizeCfg_dtor);

   Debug::SwLog(ObjectPoolSizeCfg_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

RestartLevel ObjectPoolSizeCfg::RestartRequired() const
{
   Debug::ft("ObjectPoolSizeCfg.RestartRequired");

   if(NextValue() > CurrValue()) return RestartNone;
   return Restart::LevelToClear(pool_->BlockType());
}

//------------------------------------------------------------------------------

void ObjectPoolSizeCfg::SetCurr()
{
   Debug::ft("ObjectPoolSizeCfg.SetCurr");

   FunctionGuard guard(Guard_MemUnprotect);
   CfgIntParm::SetCurr();

   //  If the pool contains no blocks, it is currently being constructed,
   //  so do nothing.  But if it already contains blocks, expand its size
   //  to the new value.
   //
   if(pool_->currSegments_ > 0)
   {
      pool_->AllocBlocks();
   }
}

//==============================================================================
//
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
   CounterPtr      expansions_;
   LowWatermarkPtr lowExcess_;
};

//------------------------------------------------------------------------------

ObjectPoolStats::ObjectPoolStats()
{
   Debug::ft("ObjectPoolStats.ctor");

   lowCount_.reset(new LowWatermark("fewest remaining blocks"));
   allocCount_.reset(new Counter("successful allocations"));
   freeCount_.reset(new Counter("deallocations"));
   failCount_.reset(new Counter("unsuccessful allocations"));
   auditCount_.reset(new Counter("blocks recovered by audit"));
   expansions_.reset(new Counter("number of times pool was expanded"));
   lowExcess_.reset(new LowWatermark("size of block minus largest object"));
}

//------------------------------------------------------------------------------

ObjectPoolStats::~ObjectPoolStats()
{
   Debug::ftnt("ObjectPoolStats.dtor");
}

//==============================================================================
//
//  Data that changes too frequently to unprotect and reprotect memory
//  when it needs to be modified.
//
struct ObjectPoolDynamic : public Persistent
{
   //  Constructor.
   //
   ObjectPoolDynamic() :
      availCount_(0),
      totalCount_(0),
      delta_(0),
      corruptQHead_(false)
   {
      freeq_.Init(Pooled::LinkDiff());
   }

   //  The queue of available blocks.
   //
   Q1Way< Pooled > freeq_;

   //  The number of blocks in freeq_.
   //
   size_t availCount_;

   //  The total number of blocks currently allocated.
   //
   size_t totalCount_;

   //  Used to reduce calls to UpdateAlarm.
   //
   int8_t delta_;

   //  Used to detect a corrupt queue header when auditing freeq_.
   //
   bool corruptQHead_;
};

//==============================================================================

const ObjectPoolId ObjectPool::MaxId = 250;

//> The number of audit cycles over which a block must be unclaimed
//  before it is recovered.
//
constexpr uint8_t OrphanThreshold = 4;

//> The maximum number of logs that display the contents of an orphaned
//  block in a given pool during each audit cycle.
//
constexpr size_t OrphanMaxLogs = 8;

ObjectPool::ObjectPool
   (ObjectPoolId pid, MemoryType mem, size_t size, const string& name) :
   name_(name.c_str()),
   key_("NumOf" + name_),
   mem_(mem),
   blockSize_(0),
   segIncr_(0),
   segSize_(0),
   currSegments_(0),
   targSegmentsCfg_(nullptr),
   alarm_(nullptr)
{
   Debug::ft("ObjectPool.ctor");

   //  The block size must account for the header above each Pooled object.
   //
   blockSize_ = BlockHeaderSize + Memory::Align(size);
   segIncr_ = blockSize_ >> BYTES_PER_WORD_LOG2;
   segSize_ = segIncr_ * ObjectsPerSegment;

   pid_.SetId(pid);

   for(auto i = 0; i < MaxSegments; ++i) blocks_[i] = nullptr;
   dyn_.reset(new ObjectPoolDynamic);
   targSegmentsCfg_.reset(new ObjectPoolSizeCfg(this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*targSegmentsCfg_);
   EnsureAlarm();
   stats_.reset(new ObjectPoolStats);
   Singleton< ObjectPoolRegistry >::Instance()->BindPool(*this);
}

//------------------------------------------------------------------------------

fn_name ObjectPool_dtor = "ObjectPool.dtor";

ObjectPool::~ObjectPool()
{
   Debug::ftnt(ObjectPool_dtor);

   Debug::SwLog(ObjectPool_dtor, UnexpectedInvocation, 0);

   for(size_t i = 0; i < currSegments_; ++i)
   {
      Memory::Free(blocks_[i], mem_);
      blocks_[i] = nullptr;
   }

   Singleton< ObjectPoolRegistry >::Extant()->UnbindPool(*this);
}

//------------------------------------------------------------------------------

bool ObjectPool::AllocBlocks()
{
   Debug::ft("ObjectPool.AllocBlocks");

   while(word(currSegments_) < targSegmentsCfg_->CurrValue())
   {
      auto pid = Pid();
      auto size = sizeof(uword) * segSize_;
      blocks_[currSegments_] = (uword*) Memory::Alloc(size, mem_, std::nothrow);

      if(blocks_[currSegments_] == nullptr)
      {
         auto log = Log::Create(ObjPoolLogGroup, ObjPoolExpansionFailed);

         if(log != nullptr)
         {
            *log << Log::Tab << "pool=" << int(pid);
            *log << " target=" << targSegmentsCfg_->CurrValue();
            *log << " actual=" << currSegments_;
            Log::Submit(log);
         }

         return false;
      }

      auto bid = currSegments_ << ObjectsPerSegmentLog2;
      ++currSegments_;
      dyn_->totalCount_ = currSegments_ * ObjectsPerSegment;
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

void ObjectPool::AuditFreeq()
{
   Debug::ft("ObjectPool.AuditFreeq");

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
      auto item = dyn_->freeq_.tail_.next;

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
         Pooled* prev = nullptr;
         auto badLink = false;

         while(count <= dyn_->totalCount_)
         {
            auto curr = (Pooled*) getptr1(item, diff);

            if(prev == nullptr)
            {
               if(dyn_->corruptQHead_)
                  badLink = true;
               else
                  dyn_->corruptQHead_ = true;
            }
            else
            {
               if(prev->corrupt_)
                  badLink = true;
               else
                  prev->corrupt_ = true;
            }

            //  CURR has not yet been claimed, so it should still be marked as
            //  orphaned (a value in the range 1 to OrphanThreshold).  If it
            //  isn't, PREV's link must be corrupt.  PREV might be pointing
            //  back into the middle of the queue, or it might be a random
            //  but legal address.
            //
            badLink = badLink ||
               ((curr->orphaned_ == 0) || (curr->orphaned_ > OrphanThreshold));

            //  If a bad link was detected, generate a log and truncate the
            //  queue.
            //
            if(badLink)
            {
               auto log = Log::Create(ObjPoolLogGroup, ObjPoolQueueCorrupt);

               if(log != nullptr)
               {
                  *log << Log::Tab << "pool=" << int(Pid());
                  *log << " available=" << dyn_->availCount_;
                  *log << " revised=" << count;
                  Log::Submit(log);
               }

               if(prev == nullptr)
               {
                  dyn_->corruptQHead_ = false;
                  dyn_->freeq_.Init(Pooled::LinkDiff());
                  dyn_->availCount_ = 0;
               }
               else
               {
                  prev->corrupt_ = false;
                  prev->link_.next = dyn_->freeq_.tail_.next;
                  dyn_->availCount_ = count;
               }

               UpdateAlarm();
               buff->Unlock();
               return;
            }

            curr->orphaned_ = 0;
            ++count;

            if(prev == nullptr)
               dyn_->corruptQHead_ = false;
            else
               prev->corrupt_ = false;

            prev = curr;
            item = item->next;

            if(dyn_->freeq_.tail_.next == item) break;  // reached tail again
         }
      }
   }
   buff->Unlock();

   //  The queue has been traversed.  Check the free count before returning.
   //
   if(dyn_->availCount_ != count)
   {
      auto log = Log::Create(ObjPoolLogGroup, ObjPoolQueueCount);

      if(log != nullptr)
      {
         *log << Log::Tab << "pool=" << int(Pid());
         *log << " available=" << dyn_->availCount_;
         *log << " revised=" << count;
         Log::Submit(log);
      }

      dyn_->availCount_ = count;
      UpdateAlarm();
   }
}

//------------------------------------------------------------------------------

size_t ObjectPool::AvailCount() const
{
   return dyn_->availCount_;
}

//------------------------------------------------------------------------------

bool ObjectPool::BidToIndices(PooledObjectId bid, size_t& i, size_t& j) const
{
   if(bid == NIL_ID) return false;

   --bid;
   i = bid >> ObjectsPerSegmentLog2;
   if(i >= currSegments_) return false;

   j = (bid & ObjectSecondIndexMask) * segIncr_;
   return true;
}

//------------------------------------------------------------------------------

Pooled* ObjectPool::BidToObj(PooledObjectId bid) const
{
   Debug::ft("ObjectPool.BidToObj");

   if(bid == NIL_ID) return nullptr;

   --bid;

   auto b = (bid == NIL_ID ? First(bid) : Next(bid));
   if((b != nullptr) && b->obj.assigned_) return &b->obj;
   return nullptr;
}

//------------------------------------------------------------------------------

ptrdiff_t ObjectPool::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const ObjectPool* >(&local);
   return ptrdiff(&fake->pid_, fake);
}

//------------------------------------------------------------------------------

bool ObjectPool::Corrupt(size_t n)
{
   Debug::ft("ObjectPool.Corrupt");

   if(!Element::RunningInLab()) return false;

   if((n == 0) || dyn_->freeq_.Empty())
   {
      dyn_->freeq_.Corrupt(nullptr);
   }
   else
   {
      auto p = dyn_->freeq_.First();

      while((p != nullptr) && (n > 1))
      {
         dyn_->freeq_.Next(p);
         --n;
      }

      if(p == nullptr) return false;
      dyn_->freeq_.Corrupt(p);
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name ObjectPool_DeqBlock = "ObjectPool.DeqBlock";

Pooled* ObjectPool::DeqBlock(size_t size)
{
   Debug::ft(ObjectPool_DeqBlock);

   auto maxsize = blockSize_ - BlockHeaderSize;

   if(size > maxsize)
   {
      Debug::SwLog(ObjectPool_DeqBlock, "size too large", size);
      throw AllocationException(mem_, size);
   }

   stats_->lowExcess_->Update(maxsize - size);

   //  If the free queue is empty, invoke UpdateAlarm, which will also
   //  allocate another segment.
   //
   auto empty = false;

   if(dyn_->freeq_.Empty())
   {
      empty = true;
      UpdateAlarm();
      stats_->lowCount_->Update(0);
   }

   auto item = dyn_->freeq_.Deq();

   if(item == nullptr)
   {
      stats_->failCount_->Incr();
      throw AllocationException(mem_, size);
   }

   --dyn_->availCount_;
   stats_->allocCount_->Incr();

   if(!empty)
   {
      stats_->lowCount_->Update(dyn_->availCount_);

      if(--dyn_->delta_ <= -50)
      {
         UpdateAlarm();
      }
   }

   if(Debug::TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Instance();

      if(buff->ToolIsOn(ObjPoolTracer))
      {
         auto rec = new ObjectPoolTrace(ObjectPoolTrace::Dequeued, *item);
         buff->Insert(rec);
      }
   }

   return item;
}

//------------------------------------------------------------------------------

void ObjectPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "pid             : " << pid_.to_str() << CRLF;
   stream << prefix << "name            : " << name_ << CRLF;
   stream << prefix << "key             : " << key_ << CRLF;
   stream << prefix << "mem             : " << mem_ << CRLF;
   stream << prefix << "blockSize       : " << blockSize_ << CRLF;
   stream << prefix << "segIncr         : " << segIncr_ << CRLF;
   stream << prefix << "segSize         : " << segSize_ << CRLF;
   stream << prefix << "currSegments    : " << currSegments_ << CRLF;
   stream << prefix << "targSegmentsCfg : ";
   stream << strObj(targSegmentsCfg_.get()) << CRLF;
   stream << prefix << "availCount      : " << dyn_->availCount_ << CRLF;
   stream << prefix << "totalCount      : " << dyn_->totalCount_ << CRLF;
   stream << prefix << "alarm           : " << strObj(alarm_) << CRLF;
   stream << prefix << "delta           : " << int(dyn_->delta_) << CRLF;
   stream << prefix << "corruptQHead    : " << dyn_->corruptQHead_ << CRLF;

   auto lead = prefix + spaces(2);
   stream << prefix << "blocks [segment]" << CRLF;

   for(size_t i = 0; i < currSegments_; ++i)
   {
      stream << lead << strIndex(i) << blocks_[i] << CRLF;
   }
}

//------------------------------------------------------------------------------

void ObjectPool::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft("ObjectPool.DisplayStats");

   stream << spaces(2) << name_ << SPACE << strIndex(Pid(), 0, false) << CRLF;

   stats_->lowCount_->DisplayStat(stream, options);
   stats_->allocCount_->DisplayStat(stream, options);
   stats_->freeCount_->DisplayStat(stream, options);
   stats_->failCount_->DisplayStat(stream, options);
   stats_->auditCount_->DisplayStat(stream, options);
   stats_->lowExcess_->DisplayStat(stream, options);
}

//------------------------------------------------------------------------------

bool ObjectPool::DisplayUsed(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PooledObjectId bid;
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
   if(deleted) Debug::ftnt(ObjectPool_EnqBlock);

   if(obj == nullptr) return;

   if(Debug::TraceOn() && deleted)
   {
      auto buff = Singleton< TraceBuffer >::Extant();

      if(buff->ToolIsOn(ObjPoolTracer))
      {
         auto rec = new ObjectPoolTrace(ObjectPoolTrace::Enqueued, *obj);
         buff->Insert(rec);
      }
   }

   //  If a block is already on the free queue or another queue, putting it
   //  on the free queue creates a mess.
   //
   if(!obj->assigned_)
   {
      if(obj->orphaned_ == 0)
      {
         Debug::SwLog(ObjectPool_EnqBlock,
            "block not in use", debug64_t(size_t(obj)));
         return;
      }
   }
   else if(obj->link_.next != nullptr)
   {
      Debug::SwLog(ObjectPool_EnqBlock,
         "block still queued", debug64_t(size_t(obj)));
      return;
   }

   auto reg = Singleton< ObjectPoolRegistry >::Extant();
   auto nullify = reg->NullifyObjectData();
   obj->Nullify(nullify ? blockSize_ - BlockHeaderSize : 0);

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

   if(!dyn_->freeq_.Enq(*obj))
   {
      Debug::SwLog(ObjectPool_EnqBlock,
         "block not queued", debug64_t(size_t(obj)));
      return;
   }

   ++dyn_->availCount_;

   if(deleted)
   {
      if(++dyn_->delta_ >= 50)
      {
         UpdateAlarm();
      }

      stats_->freeCount_->Incr();
   }
}

//------------------------------------------------------------------------------

void ObjectPool::EnsureAlarm()
{
   Debug::ft("ObjectPool.EnsureAlarm");

   //  If the high usage alarm is not registered, create it.
   //
   auto reg = Singleton< AlarmRegistry >::Instance();
   auto alarmName = "OBJPOOL" + std::to_string(Pid());
   alarm_ = reg->Find(alarmName);

   if(alarm_ == nullptr)
   {
      auto alarmExpl = "High percentage of in-use " + name_;
      FunctionGuard guard(Guard_ImmUnprotect);
      alarm_ = new Alarm(alarmName.c_str(), alarmExpl.c_str(), 30);
   }
}

//------------------------------------------------------------------------------

size_t ObjectPool::Expansions() const
{
   return stats_->expansions_->Curr();
}

//------------------------------------------------------------------------------

size_t ObjectPool::FailCount() const
{
   return stats_->failCount_->Curr();
}

//------------------------------------------------------------------------------

ObjectBlock* ObjectPool::First(PooledObjectId& bid) const
{
   Debug::ft("ObjectPool.First");

   if(currSegments_ > 0)
   {
      bid = 1;
      return (ObjectBlock*) &blocks_[0][0];
   }

   bid = NIL_ID;
   return nullptr;
}

//------------------------------------------------------------------------------

Pooled* ObjectPool::FirstUsed(PooledObjectId& bid) const
{
   Debug::ft("ObjectPool.FirstUsed");

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

bool ObjectPool::IndicesToBid(size_t i, size_t j, PooledObjectId& bid) const
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
   return dyn_->totalCount_ - dyn_->availCount_;
}

//------------------------------------------------------------------------------

size_t ObjectPool::LowAvailCount() const
{
   return stats_->lowCount_->Curr();
}

//------------------------------------------------------------------------------

ObjectBlock* ObjectPool::Next(PooledObjectId& bid) const
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

Pooled* ObjectPool::NextUsed(PooledObjectId& bid) const
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
               Debug::SwLog(ObjectPool_NextUsed,
                  "index error", debug64_t(size_t(b)));
               return nullptr;
            }
         }
      }
   }

   bid = NIL_ID;
   return nullptr;
}

//------------------------------------------------------------------------------

PooledObjectId ObjectPool::ObjBid(const Pooled* obj, bool inUseOnly) const
{
   Debug::ft("ObjectPool.ObjBid");

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

void ObjectPool::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void ObjectPool::RecoverBlocks()
{
   Debug::ft("ObjectPool.RecoverBlocks");

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
                  auto rec =
                     new ObjectPoolTrace(ObjectPoolTrace::Recovered, *p);
                  buff->Insert(rec);
               }
            }

            if(p->assigned_ && !p->logged_ && (count <= OrphanMaxLogs))
            {
               auto log = Log::Create(ObjPoolLogGroup, ObjPoolBlockRecovered);

               if(log != nullptr)
               {
                  *log << Log::Tab << "pool=" << int(pid) << CRLF;
                  p->logged_ = true;
                  p->Display(*log, Log::Tab, VerboseOpt);
                  Log::Submit(log);
               }
            }

            //  When an in-use orphan is found, we mark it corrupt and clean
            //  it up.  If it is so corrupt that it causes an exception during
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
      auto log = Log::Create(ObjPoolLogGroup, ObjPoolBlocksRecovered);

      if(log != nullptr)
      {
         *log << Log::Tab << "pool=" << int(pid);
         *log << " recovered=" << count;
         Log::Submit(log);
      }
   }
}

//------------------------------------------------------------------------------

void ObjectPool::Shutdown(RestartLevel level)
{
   Debug::ft("ObjectPool.Shutdown");

   if(Restart::ClearsMemory(MemType())) return;

   //  Reinitialize our segments and dynamic data if the restart
   //  will destroy the heap where our blocks are allocated.
   //
   FunctionGuard guard(Guard_MemUnprotect);

   Restart::Release(stats_);

   if(Restart::ClearsMemory(mem_))
   {
      for(auto i = 0; i < MaxSegments; ++i) blocks_[i] = nullptr;
      currSegments_ = 0;
      new (dyn_.get()) ObjectPoolDynamic();
   }
}

//------------------------------------------------------------------------------

void ObjectPool::Startup(RestartLevel level)
{
   Debug::ft("ObjectPool.Startup");

   FunctionGuard guard(Guard_MemUnprotect);

   if(stats_ == nullptr) stats_.reset(new ObjectPoolStats);

   if(!AllocBlocks())
   {
      Restart::Initiate(RestartWarm, ObjectPoolCreationFailed, Pid());
   }
}

//------------------------------------------------------------------------------

void ObjectPool::UpdateAlarm()
{
   Debug::ft("ObjectPool.UpdateAlarm");

   if(alarm_ == nullptr) return;
   dyn_->delta_ = 0;

   //  The alarm level is determined by the number of available blocks
   //  compared to the total number of blocks allocated:
   //    o critical: less than 1/32nd available
   //    o major: less than 1/16th available
   //    o minor: less than 1/8th available
   //    o none: more than 1/8th available
   //
   auto status = NoAlarm;

   if(dyn_->availCount_ <= (dyn_->totalCount_ >> 5))
      status = CriticalAlarm;
   else if(dyn_->availCount_ <= (dyn_->totalCount_ >> 4))
      status = MajorAlarm;
   else if(dyn_->availCount_ <= (dyn_->totalCount_ >> 3))
      status = MinorAlarm;

   auto log = alarm_->Create(ObjPoolLogGroup, ObjPoolBlocksInUse, status);
   if(log != nullptr) Log::Submit(log);

   //  When the number of available blocks drops to a dangerous level,
   //  add another segment to the pool.
   //
   if(dyn_->availCount_ <= (dyn_->totalCount_ >> 6))
   {
      RestartLevel level;
      auto size = std::to_string(currSegments_ + 1);

      if(targSegmentsCfg_->SetValue(size.c_str(), level))
      {
         stats_->expansions_->Incr();

         log = Log::Create(ObjPoolLogGroup, ObjPoolExpanded);

         if(log != nullptr)
         {
            *log << Log::Tab << "pool=" << name_;
            *log << "  new segments=" << currSegments_;
            Log::Submit(log);
         }
      }
   }
}
}
