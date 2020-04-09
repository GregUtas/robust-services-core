//==============================================================================
//
//  InvokerPool.cpp
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
#include "InvokerPool.h"
#include <cstdint>
#include <sstream>
#include "Algorithms.h"
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Context.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "InvokerPoolRegistry.h"
#include "InvokerThread.h"
#include "LocalAddress.h"
#include "Log.h"
#include "Message.h"
#include "MsgHeader.h"
#include "Q2Way.h"
#include "Restart.h"
#include "SbDaemons.h"
#include "SbIpBuffer.h"
#include "SbLogs.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "SbTracer.h"
#include "Singleton.h"
#include "Statistics.h"
#include "ThisThread.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Statistics for each invoker pool.
//
class InvokerPoolStats : public Dynamic
{
public:
   InvokerPoolStats();
   ~InvokerPoolStats();

   HighWatermarkPtr maxTrans_;
   CounterPtr       requeues_;
   CounterPtr       trojans_;
   CounterPtr       lockouts_;
};

//  The work of a given priority that is waiting for an invoker pool.
//
class InvokerWork : public Dynamic
{
public:
   //  Creates an empty queue and its statistics.
   //
   InvokerWork();

   //  Purges any items in the queue.
   //
   ~InvokerWork();

   //  Deleted to prohibit copying.
   //
   InvokerWork(const InvokerWork& that) = delete;
   InvokerWork& operator=(const InvokerWork& that) = delete;

   //  Queue of contexts that have messages waiting to be processed.
   //
   Q2Way< Context > contextq_;

   //  The current length of the queue.
   //
   size_t length_;

   //  The number of contexts dequeued.
   //
   CounterPtr dequeues_;

   //  The longest length of the queue.
   //
   HighWatermarkPtr maxLength_;

   //  The longest time that a context was queued.
   //
   HighWatermarkPtr maxDelay_;
};

//------------------------------------------------------------------------------

fn_name InvokerPoolStats_ctor = "InvokerPoolStats.ctor";

InvokerPoolStats::InvokerPoolStats()
{
   Debug::ft(InvokerPoolStats_ctor);

   maxTrans_.reset(new HighWatermark("most transactions before yielding"));
   requeues_.reset(new Counter("contexts requeued after priority work"));
   trojans_.reset(new Counter("corrupt contexts found on work queue"));
   lockouts_.reset(new Counter("times that all invokers were blocked"));
}

//------------------------------------------------------------------------------

fn_name InvokerPoolStats_dtor = "InvokerPoolStats.dtor";

InvokerPoolStats::~InvokerPoolStats()
{
   Debug::ft(InvokerPoolStats_dtor);
}

//==============================================================================

fn_name InvokerWork_ctor = "InvokerWork.ctor";

InvokerWork::InvokerWork() : length_(0)
{
   Debug::ft(InvokerWork_ctor);

   contextq_.Init(Context::LinkDiff());

   dequeues_.reset(new Counter("contexts dequeued"));
   maxLength_.reset(new HighWatermark("longest length of work queue"));
   maxDelay_.reset(new HighWatermark("longest queue delay in msecs"));
}

//------------------------------------------------------------------------------

fn_name InvokerWork_dtor = "InvokerWork.dtor";

InvokerWork::~InvokerWork()
{
   Debug::ft(InvokerWork_dtor);

   contextq_.Purge();
}

//==============================================================================

const size_t InvokerPool::MaxInvokers = 10;

//------------------------------------------------------------------------------

fn_name InvokerPool_ctor = "InvokerPool.ctor";

InvokerPool::InvokerPool(Faction faction, const string& parmKey) :
   poolSize_(1),
   cfgInvokers_(nullptr),
   corrupt_(false)
{
   Debug::ft(InvokerPool_ctor);

   faction_.SetId(faction);
   invokers_.Init(MaxInvokers, InvokerThread::CellDiff2(), MemDynamic);
   stats_.reset(new InvokerPoolStats);

   //  After a restart, cfgInvokers_ may still exist, so try to look it
   //  up before creating it.
   //
   auto reg = Singleton< CfgParmRegistry >::Instance();

   cfgInvokers_.reset(static_cast< CfgIntParm* >(reg->FindParm(parmKey)));

   if(cfgInvokers_ == nullptr)
   {
      cfgInvokers_.reset(new CfgIntParm(parmKey.c_str(), "1",
         reinterpret_cast< word* >(&poolSize_), 1,
         10, "number of invokers in pool"));
      reg->BindParm(*cfgInvokers_);
   }

   for(auto p = 0; p <= MAX_PRIORITY; ++p)
   {
      work_[p].reset(new InvokerWork);
   }

   Singleton< InvokerPoolRegistry >::Instance()->BindPool(*this);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_dtor = "InvokerPool.dtor";

InvokerPool::~InvokerPool()
{
   Debug::ft(InvokerPool_dtor);

   Singleton< InvokerPoolRegistry >::Instance()->UnbindPool(*this);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_BindThread = "InvokerPool.BindThread";

bool InvokerPool::BindThread(InvokerThread& thread)
{
   Debug::ft(InvokerPool_BindThread);

   return invokers_.Insert(thread);
}

//------------------------------------------------------------------------------

ptrdiff_t InvokerPool::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const InvokerPool* >(&local);
   return ptrdiff(&fake->faction_, fake);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_ClaimBlocks = "InvokerPool.ClaimBlocks";

void InvokerPool::ClaimBlocks()
{
   Debug::ft(InvokerPool_ClaimBlocks);

   //  Mark all objects accessible through the work queues as being in use.
   //  If we trap because a work queue was corrupt, cause a restart.
   //
   if(corrupt_) Restart::Initiate(WorkQueueCorruption, GetFaction());

   corrupt_ = true;

   //  MsgPriority is unsigned, so P must be signed to end this loop.
   //
   for(int p = MAX_PRIORITY; p >= 0; --p)
   {
      auto ctxq = &work_[p]->contextq_;

      for(auto c = ctxq->First(); c != nullptr; ctxq->Next(c))
      {
         //  CTX seems to be a valid pointer.  Before we ask the context
         //  to claim all of its objects, we mark ourselves as not having
         //  trapped, given that the queue link was sane.  When traversal
         //  of the work queue resumes, we mark ourselves as having trapped
         //  again, in case the next queue link is not sane.
         //
         corrupt_ = false;
         c->ClaimBlocks();
         corrupt_ = true;
      }
   }

   corrupt_ = false;

   //  Instances of MsgContext that perform blocking operations neither
   //  appear in a work queue (because they are currently running), nor
   //  do they have PSMs.  Consequently, they can only be found through
   //  the association with the invoker thread that currently owns them.
   //
   for(auto i = invokers_.First(); i != nullptr; invokers_.Next(i))
   {
      auto ctx = i->GetContext();
      if(ctx != nullptr) ctx->ClaimBlocks();
   }
}

//------------------------------------------------------------------------------

fn_name InvokerPool_Dequeued = "InvokerPool.Dequeued";

void InvokerPool::Dequeued(MsgPriority prio) const
{
   Debug::ft(InvokerPool_Dequeued);

   auto work = work_[prio].get();

   if(work->length_ > 0)
   {
      --work->length_;
      return;
   }

   auto log = Log::Create(SessionLogGroup, InvokerWorkQueueCount);

   if(log != nullptr)
   {
      *log << Log::Tab << "pool=" << int(GetFaction());
      *log << " queue=" << prio << " [underflow]";
      Log::Submit(log);
   }

   work->length_ = work->contextq_.Size();
}

//------------------------------------------------------------------------------

void InvokerPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "faction     : " << faction_.to_str() << CRLF;
   stream << prefix << "poolSize    : " << poolSize_ << CRLF;
   stream << prefix << "corrupt     : " << corrupt_ << CRLF;
   stream << prefix << "cfgInvokers : " << CRLF;
   stream << strObj(cfgInvokers_.get()) << CRLF;

   stream << prefix << "invokers []" << CRLF;
   invokers_.Display(stream, prefix + spaces(2), options);

   auto lead = prefix + spaces(2);
   stream << prefix << "workq [MsgPriority]" << CRLF;

   for(auto p = 0; p <= MAX_PRIORITY; ++p)
   {
      stream << lead << strIndex(p);
      stream << "length=" << work_[p]->length_ << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name InvokerPool_DisplayStats = "InvokerPool.DisplayStats";

void InvokerPool::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft(InvokerPool_DisplayStats);

   stream << spaces(2) << GetFaction();
   stream << SPACE << strIndex(GetFaction(), 0, false) << CRLF;

   for(auto p = 0; p <= MAX_PRIORITY; ++p)
   {
      auto work = work_[p].get();
      stream << spaces(4) << strMsgPriority(p);
      stream << " work queue:" << CRLF;
      work->dequeues_->DisplayStat(stream, options);
      work->maxLength_->DisplayStat(stream, options);
      work->maxDelay_->DisplayStat(stream, options);
   }

   stream << spaces(4) << "pool statistics:" << CRLF;
   stats_->maxTrans_->DisplayStat(stream, options);
   stats_->requeues_->DisplayStat(stream, options);
   stats_->trojans_->DisplayStat(stream, options);
   stats_->lockouts_->DisplayStat(stream, options);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_Enqueued = "InvokerPool.Enqueued";

void InvokerPool::Enqueued(MsgPriority prio) const
{
   Debug::ft(InvokerPool_Enqueued);

   auto work = work_[prio].get();
   ++work->length_;
   work->maxLength_->Update(work->length_);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_FindWork = "InvokerPool.FindWork";

Context* InvokerPool::FindWork()
{
   Debug::ft(InvokerPool_FindWork);

   //  MsgPriority is unsigned, so PRIO must be signed to end this loop.
   //
   for(int prio = MAX_PRIORITY; prio >= 0; --prio)
   {
      auto work = work_[prio].get();
      auto ctx = work->contextq_.First();

      if(ctx != nullptr)
      {
         ctx->Exqueue();
         work->dequeues_->Incr();
         return ctx;
      }
      else if(work->length_ > 0)
      {
         auto log = Log::Create(SessionLogGroup, InvokerWorkQueueCount);

         if(log != nullptr)
         {
            *log << Log::Tab << "pool=" << int(GetFaction());
            *log << " queue=" << prio << " [zeroed]";
            Log::Submit(log);
         }

         work->length_ = 0;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name InvokerPool_GenerateLog = "InvokerPool.GenerateLog";

bool InvokerPool::GenerateLog(Factory::Rc rc)
{
   Debug::ft(InvokerPool_GenerateLog);

   //  Suppress PortNotFound logs, which arise from legitimate race conditions.
   //
   return (rc != Factory::PortNotFound);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_KickThread = "InvokerPool.KickThread";

void InvokerPool::KickThread()
{
   Debug::ft(InvokerPool_KickThread);

   //  Ensure that one of our invokers is ready to handle newly queued work.
   //
   for(auto i = invokers_.First(); i != nullptr; invokers_.Next(i))
   {
      if(i->IsScheduled()) return;
   }

   for(auto i = invokers_.First(); i != nullptr; invokers_.Next(i))
   {
      if(i->Interrupt()) return;
   }

   stats_->lockouts_->Incr();

   //  During a restart, all invoker threads exit and are recreated, so
   //  suppress the following log.
   //
   if(Restart::GetStatus() == Running)
   {
      auto log = Log::Create(SessionLogGroup, InvokerPoolBlocked);
      if(log == nullptr) return;
      *log << Log::Tab << "pool=" << int(GetFaction());
      Log::Submit(log);
   }
}

//------------------------------------------------------------------------------

fn_name InvokerPool_LogLostBuff = "InvokerPool.LogLostBuff";

bool InvokerPool::LogLostBuff
   (SbIpBuffer& buff, FactoryId fid, Factory::Rc rc) const
{
   Debug::ft(InvokerPool_LogLostBuff);

   buff.InvalidDiscarded();

   if(GenerateLog(rc))
   {
      auto log = Log::Create(SessionLogGroup, InvokerDiscardedBuffer);

      if(log != nullptr)
      {
         *log << Log::Tab << "pool=" << int(GetFaction());
         *log << " factory=" << fid;
         *log << " errval=" << rc << CRLF;
         buff.Output(*log, Log::Indent, true);
         Log::Submit(log);
      }
   }

   delete &buff;
   return false;
}

//------------------------------------------------------------------------------

fn_name InvokerPool_LogLostMsg = "InvokerPool.LogLostMsg";

bool InvokerPool::LogLostMsg(Message& msg, Factory::Rc rc, TransTrace* tt) const
{
   Debug::ft(InvokerPool_LogLostMsg);

   msg.InvalidDiscarded();

   if(GenerateLog(rc))
   {
      auto log = Log::Create(SessionLogGroup, InvokerDiscardedMessage);

      if(log != nullptr)
      {
         *log << Log::Tab << "pool=" << int(GetFaction());
         *log << " protocol=" << msg.GetProtocol();
         *log << " signal=" << msg.GetSignal();
         *log << " errval=" << rc << CRLF;
         msg.Output(*log, Log::Indent, true);
         Log::Submit(log);
      }
   }

   delete &msg;
   if(tt != nullptr) tt->EndOfTransaction();
   return false;
}

//------------------------------------------------------------------------------

void InvokerPool::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_ProcessWork = "InvokerPool.ProcessWork";

void InvokerPool::ProcessWork()
{
   Debug::ft(InvokerPool_ProcessWork);

   //  Dequeue a context from the work queue and invoke its ProcessWork
   //  function.
   //
   while(true)
   {
      auto inv = static_cast< InvokerThread* >(Thread::RunningThread());
      auto ctx = FindWork();

      if(ctx != nullptr)
      {
         auto safe = (!ctx->IsCorrupt());

         if(safe)
         {
            inv->SetContext(ctx);
            ctx->ProcessWork(inv);
            inv->ClearContext();
         }
         else
         {
            stats_->trojans_->Incr();
         }

         //  If we still have enough time to process more work, keep
         //  going, else yield.
         //
         ThisThread::PauseOver(InvokerThread::RtcYieldPercent());
      }
      else
      {
         //  No work was found.  Go to sleep indefinitely; we will be
         //  woken up when work arrives.
         //
         ThisThread::Pause(TIMEOUT_NEVER);
      }
   }
}

//------------------------------------------------------------------------------

fn_name InvokerPool_ReadyCount = "InvokerPool.ReadyCount";

size_t InvokerPool::ReadyCount() const
{
   Debug::ft(InvokerPool_ReadyCount);

   size_t ready = 0;

   //  A delaying invoker can be interrupted, so include it in the count.
   //
   for(auto i = invokers_.First(); i != nullptr; invokers_.Next(i))
   {
      switch(i->GetBlockingReason())
      {
      case NotBlocked:
      case BlockedOnClock:
         ++ready;
         break;
      }
   }

   return ready;
}

//------------------------------------------------------------------------------

fn_name InvokerPool_ReceiveBuff = "InvokerPool.ReceiveBuff";

bool InvokerPool::ReceiveBuff(SbIpBufferPtr& buff, bool atIoLevel)
{
   Debug::ft(InvokerPool_ReceiveBuff);

   auto header = buff->Header();

   //  Check that a valid message header exists.  Use it to find the factory
   //  that will receive the message.  Ask that factory to wrap BUFF in a
   //  message, which can then be injected using ReceiveMsg.
   //
   if(header == nullptr)
   {
      return LogLostBuff(*buff, 0, Factory::MsgHeaderMissing);
   }

   auto fid = header->rxAddr.fid;
   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(fid);

   if(fac == nullptr)
   {
      return LogLostBuff(*buff, fid, Factory::FactoryNotFound);
   }

   auto msg = fac->AllocIcMsg(buff);

   if(msg == nullptr)
   {
      return LogLostBuff(*buff, fid, Factory::MsgAllocFailed);
   }

   return ReceiveMsg(*msg, atIoLevel);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_ReceiveMsg = "InvokerPool.ReceiveMsg";

bool InvokerPool::ReceiveMsg(Message& msg, bool atIoLevel)
{
   Debug::ft(InvokerPool_ReceiveMsg);

   auto header = msg.Header();
   Context* ctx = nullptr;
   TransTrace* tt = nullptr;

   //  Get the message header.  Find out what factory is receiving the message.
   //
   if(header == nullptr)
   {
      return LogLostMsg(msg, Factory::MsgHeaderMissing, tt);
   }

   auto fid = header->rxAddr.fid;
   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(fid);

   if(fac == nullptr)
   {
      return LogLostMsg(msg, Factory::FactoryNotFound, tt);
   }

   //  Check the message's priority.  If tracing is active, record the message
   //  if it arrived over the IP stack.
   //
   auto prio = header->priority;

   if(prio > MAX_PRIORITY)
   {
      return LogLostMsg(msg, Factory::MsgPriorityInvalid, tt);
   }

   if(Debug::TraceOn() && atIoLevel)
   {
      tt = TraceRxNet(msg, *fac);
   }

   //  Pass the message to the factory, which returns the context on which
   //  the message was queued.  On success, update the transaction record
   //  with the context and tell the factory to record the message.
   //
   auto rc = fac->ReceiveMsg(msg, atIoLevel, tt, ctx);
   if(tt != nullptr) tt->SetContext(ctx);

   if(rc != Factory::InputOk)
   {
      return LogLostMsg(msg, rc, tt);
   }

   fac->RecordMsg(true, atIoLevel, header->length);

   //  If this is an ingress message that created a new context, see if it
   //  should be queued differently than usual.  The factory has the option
   //  of putting the new context at the front of the ingress work queue or
   //  even on another queue.
   //
   auto henq = false;

   if((prio == INGRESS) && (ctx->MsgCount(true, true) == 1))
   {
      henq = fac->ScreenFirstMsg(msg, prio);
   }

   //  Put the context on the appropriate work queue.  If the context is
   //  already on a queue, it knows how to deal with this.
   //
   ctx->Enqueue(work_[prio]->contextq_, prio, henq);

   //  Make sure that an invoker thread will handle the work.
   //
   KickThread();
   return true;
}

//------------------------------------------------------------------------------

void InvokerPool::RecordDelay(MsgPriority prio, msecs_t delay) const
{
   work_[prio]->maxDelay_->Update(delay);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_Requeue = "InvokerPool.Requeue";

void InvokerPool::Requeue(Context& ctx)
{
   Debug::ft(InvokerPool_Requeue);

   //  A context has processed its priority messages.  It still has standard
   //  messages queued against it, so it has invoked this function in order
   //  to return to the progress queue.
   //
   stats_->requeues_->Incr();
   ctx.Enqueue(work_[PROGRESS]->contextq_, PROGRESS, false);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_ScheduledOut = "InvokerPool.ScheduledOut";

void InvokerPool::ScheduledOut()
{
   Debug::ft(InvokerPool_ScheduledOut);

   if(InvokerThread::RunningInvoker_ == nullptr) return;
   if(Restart::GetStatus() != Running) return;
   stats_->maxTrans_->Update(InvokerThread::RunningInvoker_->trans_);
}

//------------------------------------------------------------------------------

fn_name InvokerPool_Startup = "InvokerPool.Startup";

void InvokerPool::Startup(RestartLevel level)
{
   Debug::ft(InvokerPool_Startup);

   auto daemon = InvokerDaemon::GetDaemon(GetFaction(), poolSize_);
   daemon->CreateThreads();
}

//------------------------------------------------------------------------------

fn_name InvokerPool_TraceRxNet = "InvokerPool.TraceRxNet";

TransTrace* InvokerPool::TraceRxNet(Message& msg, const Factory& fac)
{
   Debug::ft(InvokerPool_TraceRxNet);

   auto sbt = Singleton< SbTracer >::Instance();

   TransTrace* trans = nullptr;

   if(sbt->MsgStatus(msg, MsgIncoming) == TraceIncluded)
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      auto warp = Clock::TicksNow();

      if(buff->ToolIsOn(TransTracer))
      {
         auto rec = trans = new TransTrace(msg, fac);
         buff->Insert(rec);
      }

      if(buff->ToolIsOn(BufferTracer))
      {
         auto pool = Singleton< BtIpBufferPool >::Instance();

         if(pool->AvailCount() > 0)
         {
            auto rec = new BuffTrace(BuffTrace::IcMsg, *msg.Buffer());
            if(buff->Insert(rec)) msg.SetTrace(rec);
         }
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }

   return trans;
}

//------------------------------------------------------------------------------

fn_name InvokerPool_UnbindThread = "InvokerPool.UnbindThread";

void InvokerPool::UnbindThread(InvokerThread& thread)
{
   Debug::ft(InvokerPool_UnbindThread);

   invokers_.Erase(thread);
}

//------------------------------------------------------------------------------

size_t InvokerPool::WorkQCurrLength(MsgPriority prio) const
{
   if(prio > MAX_PRIORITY) return 0;
   return work_[prio]->length_;
}

//------------------------------------------------------------------------------

msecs_t InvokerPool::WorkQMaxDelay(MsgPriority prio) const
{
   if(prio > MAX_PRIORITY) return 0;
   return work_[prio]->maxDelay_->Curr();
}

//------------------------------------------------------------------------------

size_t InvokerPool::WorkQMaxLength(MsgPriority prio) const
{
   if(prio > MAX_PRIORITY) return 0;
   return work_[prio]->maxLength_->Curr();
}
}
