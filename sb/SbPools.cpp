//==============================================================================
//
//  SbPools.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SbPools.h"
#include <memory>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Event.h"
#include "InvokerPoolRegistry.h"
#include "Message.h"
#include "MsgPort.h"
#include "NbAppIds.h"
#include "ProtocolSM.h"
#include "RootServiceSM.h"
#include "SbIpBuffer.h"
#include "Singleton.h"
#include "SsmContext.h"
#include "Statistics.h"
#include "SysTypes.h"
#include "ThisThread.h"
#include "Timer.h"
#include "TimerRegistry.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
const size_t SbIpBufferPool::BlockSize = sizeof(SbIpBuffer);

//------------------------------------------------------------------------------

fn_name SbIpBufferPool_ctor = "SbIpBufferPool.ctor";

SbIpBufferPool::SbIpBufferPool() :
   ObjectPool(SbIpBufferObjPoolId, MemDyn, BlockSize, "SbIpBuffers")
{
   Debug::ft(SbIpBufferPool_ctor);
}

//------------------------------------------------------------------------------

fn_name SbIpBufferPool_dtor = "SbIpBufferPool.dtor";

SbIpBufferPool::~SbIpBufferPool()
{
   Debug::ft(SbIpBufferPool_dtor);
}

//------------------------------------------------------------------------------

void SbIpBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t ContextPool::BlockSize = sizeof(SsmContext);

//------------------------------------------------------------------------------

fn_name ContextPool_ctor = "ContextPool.ctor";

ContextPool::ContextPool() :
   ObjectPool(ContextObjPoolId, MemDyn, BlockSize, "Contexts")
{
   Debug::ft(ContextPool_ctor);
}

//------------------------------------------------------------------------------

fn_name ContextPool_dtor = "ContextPool.dtor";

ContextPool::~ContextPool()
{
   Debug::ft(ContextPool_dtor);
}

//------------------------------------------------------------------------------

fn_name ContextPool_ClaimBlocks = "ContextPool.ClaimBlocks";

void ContextPool::ClaimBlocks()
{
   Debug::ft(ContextPool_ClaimBlocks);

   Singleton< InvokerPoolRegistry >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void ContextPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t EventPool::BlockSize = sizeof(Event) + (20 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name EventPool_ctor = "EventPool.ctor";

EventPool::EventPool() :
   ObjectPool(EventObjPoolId, MemDyn, BlockSize, "Events")
{
   Debug::ft(EventPool_ctor);
}

//------------------------------------------------------------------------------

fn_name EventPool_dtor = "EventPool.dtor";

EventPool::~EventPool()
{
   Debug::ft(EventPool_dtor);
}

//------------------------------------------------------------------------------

void EventPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t MessagePool::BlockSize = sizeof(Message) + (40 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name MessagePool_ctor = "MessagePool.ctor";

MessagePool::MessagePool() :
   ObjectPool(MessageObjPoolId, MemDyn, BlockSize, "Messages")
{
   Debug::ft(MessagePool_ctor);
}

//------------------------------------------------------------------------------

fn_name MessagePool_dtor = "MessagePool.dtor";

MessagePool::~MessagePool()
{
   Debug::ft(MessagePool_dtor);
}

//------------------------------------------------------------------------------

void MessagePool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t MsgPortPool::BlockSize = sizeof(MsgPort);

//------------------------------------------------------------------------------

fn_name MsgPortPool_ctor = "MsgPortPool.ctor";

MsgPortPool::MsgPortPool() :
   ObjectPool(MsgPortObjPoolId, MemDyn, BlockSize, "MsgPorts")
{
   Debug::ft(MsgPortPool_ctor);
}

//------------------------------------------------------------------------------

fn_name MsgPortPool_dtor = "MsgPortPool.dtor";

MsgPortPool::~MsgPortPool()
{
   Debug::ft(MsgPortPool_dtor);
}

//------------------------------------------------------------------------------

fn_name MsgPortPool_FindPeerPort = "MsgPortPool.FindPeerPort";

MsgPort* MsgPortPool::FindPeerPort(const GlobalAddress& remAddr) const
{
   Debug::ft(MsgPortPool_FindPeerPort);

   Bid bid;

   //e This could be sped up by placing each in-use MsgPort in a queue selected
   //  by, say, the last 10 bits of its PooledObjectId.  Only MsgPorts created
   //  to *receive* an initial message would be queued.  The extra queueing and
   //  dequeuing might, in fact, reduce overall capacity.  However, the cost of
   //  an incoming message would be far more predictable, because we currently
   //  search through all ports (of which there could easily be 64K).  Provided
   //  that this function is infrequently used, searching seems acceptable.
   //
   for(auto obj = FirstUsed(bid); obj != nullptr; obj = NextUsed(bid))
   {
      auto port = static_cast< MsgPort* >(obj);

      if(port->remAddr_ == remAddr)
      {
         return port;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void MsgPortPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t ProtocolSMPool::BlockSize =
   sizeof(ProtocolSM) + (60 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name ProtocolSMPool_ctor = "ProtocolSMPool.ctor";

ProtocolSMPool::ProtocolSMPool() :
   ObjectPool(ProtocolSMObjPoolId, MemDyn, BlockSize, "ProtocolSMs"),
   psmToAudit_(NIL_ID)
{
   Debug::ft(ProtocolSMPool_ctor);
}

//------------------------------------------------------------------------------

fn_name ProtocolSMPool_dtor = "ProtocolSMPool.dtor";

ProtocolSMPool::~ProtocolSMPool()
{
   Debug::ft(ProtocolSMPool_dtor);
}

//------------------------------------------------------------------------------

fn_name ProtocolSMPool_ClaimBlocks = "ProtocolSMPool.ClaimBlocks";

void ProtocolSMPool::ClaimBlocks()
{
   Debug::ft(ProtocolSMPool_ClaimBlocks);

   //  Find the context for each in-use PSM and mark all the objects in its
   //  context as being in use.  A context that is not on a work queue MUST
   //  have a PSM, so its objects are claimed via PSMs.  This way, the audit
   //  will recover any context that is not on a work queue and has no PSM.
   //  If we encounter a corrupt PSM, the audit invokes this function again
   //  after recovering from a trap.  psmToAudit_ therefore persists outside
   //  this function so that we can continue from where we left off.
   //
   size_t count = 0;

   auto psm = static_cast< ProtocolSM* >(NextUsed(psmToAudit_));

   while(psm != nullptr)
   {
      auto ctx = psm->GetContext();

      if(ctx != nullptr)
      {
         ctx->ClaimBlocks();

         if(++count >= 100)
         {
            ThisThread::PauseOver(90);
            count = 0;
         }
      }

      psm = static_cast< ProtocolSM* >(NextUsed(psmToAudit_));
   }

   psmToAudit_ = NIL_ID;
}

//------------------------------------------------------------------------------

void ProtocolSMPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ObjectPool::Display(stream, prefix, options);

   stream << prefix << "psmToAudit : " << psmToAudit_ << CRLF;
}

//------------------------------------------------------------------------------

void ProtocolSMPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t ServiceSMPool::BlockSize =
   sizeof(RootServiceSM) + (60 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name ServiceSMPool_ctor = "ServiceSMPool.ctor";

ServiceSMPool::ServiceSMPool() :
   ObjectPool(ServiceSMObjPoolId, MemDyn, BlockSize, "ServiceSMs")
{
   Debug::ft(ServiceSMPool_ctor);
}

//------------------------------------------------------------------------------

fn_name ServiceSMPool_dtor = "ServiceSMPool.dtor";

ServiceSMPool::~ServiceSMPool()
{
   Debug::ft(ServiceSMPool_dtor);
}

//------------------------------------------------------------------------------

void ServiceSMPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t TimerPool::BlockSize = sizeof(Timer);

//------------------------------------------------------------------------------

fn_name TimerPool_ctor = "TimerPool.ctor";

TimerPool::TimerPool() :
   ObjectPool(TimerObjPoolId, MemDyn, BlockSize, "Timers")
{
   Debug::ft(TimerPool_ctor);

   timeouts_.reset(new Counter("timeout messages sent"));
}

//------------------------------------------------------------------------------

fn_name TimerPool_dtor = "TimerPool.dtor";

TimerPool::~TimerPool()
{
   Debug::ft(TimerPool_dtor);
}

//------------------------------------------------------------------------------

fn_name TimerPool_ClaimBlocks = "TimerPool.ClaimBlocks";

void TimerPool::ClaimBlocks()
{
   Debug::ft(TimerPool_ClaimBlocks);

   Singleton< TimerRegistry >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

fn_name TimerPool_DisplayStats = "TimerPool.DisplayStats";

void TimerPool::DisplayStats(ostream& stream) const
{
   Debug::ft(TimerPool_DisplayStats);

   ObjectPool::DisplayStats(stream);

   timeouts_->DisplayStat(stream);
}

//------------------------------------------------------------------------------

void TimerPool::IncrTimeouts() const
{
   timeouts_->Incr();
}

//------------------------------------------------------------------------------

void TimerPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name TimerPool_Shutdown = "TimerPool.Shutdown";

void TimerPool::Shutdown(RestartLevel level)
{
   Debug::ft(TimerPool_Shutdown);

   if(level >= RestartCold) timeouts_ = nullptr;

   ObjectPool::Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name TimerPool_Startup = "TimerPool.Startup";

void TimerPool::Startup(RestartLevel level)
{
   Debug::ft(TimerPool_Startup);

   ObjectPool::Startup(level);

   if(timeouts_ == nullptr)
   {
      timeouts_.reset(new Counter("timeout messages sent"));
   }
}

//==============================================================================

const size_t BtIpBufferPool::BlockSize = sizeof(SbIpBuffer);

//------------------------------------------------------------------------------

fn_name BtIpBufferPool_ctor = "BtIpBufferPool.ctor";

BtIpBufferPool::BtIpBufferPool() :
   ObjectPool(BtIpBufferObjPoolId, MemDyn, BlockSize, "BtIpBuffers")
{
   Debug::ft(BtIpBufferPool_ctor);
}

//------------------------------------------------------------------------------

fn_name BtIpBufferPool_dtor = "BtIpBufferPool.dtor";

BtIpBufferPool::~BtIpBufferPool()
{
   Debug::ft(BtIpBufferPool_dtor);
}

//------------------------------------------------------------------------------

fn_name BtIpBufferPool_ClaimBlocks = "BtIpBufferPool.ClaimBlocks";

void BtIpBufferPool::ClaimBlocks()
{
   Debug::ft(BtIpBufferPool_ClaimBlocks);

   Singleton< TraceBuffer >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void BtIpBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}
}
