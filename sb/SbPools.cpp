//==============================================================================
//
//  SbPools.cpp
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
#include "SbPools.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Event.h"
#include "FunctionGuard.h"
#include "GlobalAddress.h"
#include "InvokerPoolRegistry.h"
#include "Message.h"
#include "MsgPort.h"
#include "NbAppIds.h"
#include "ProtocolSM.h"
#include "Restart.h"
#include "RootServiceSM.h"
#include "SbIpBuffer.h"
#include "SbTypes.h"
#include "Singleton.h"
#include "SsmContext.h"
#include "Statistics.h"
#include "SysTypes.h"
#include "ThisThread.h"
#include "Timer.h"
#include "TimerRegistry.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
const size_t SbIpBufferPool::BlockSize = sizeof(SbIpBuffer);

//------------------------------------------------------------------------------

SbIpBufferPool::SbIpBufferPool() :
   ObjectPool(SbIpBufferObjPoolId, MemDynamic, BlockSize, "SbIpBuffers")
{
   Debug::ft("SbIpBufferPool.ctor");
}

//------------------------------------------------------------------------------

SbIpBufferPool::~SbIpBufferPool()
{
   Debug::ftnt("SbIpBufferPool.dtor");
}

//------------------------------------------------------------------------------

void SbIpBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t ContextPool::BlockSize = sizeof(SsmContext);

//------------------------------------------------------------------------------

ContextPool::ContextPool() :
   ObjectPool(ContextObjPoolId, MemDynamic, BlockSize, "Contexts")
{
   Debug::ft("ContextPool.ctor");
}

//------------------------------------------------------------------------------

ContextPool::~ContextPool()
{
   Debug::ftnt("ContextPool.dtor");
}

//------------------------------------------------------------------------------

void ContextPool::ClaimBlocks()
{
   Debug::ft("ContextPool.ClaimBlocks");

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

EventPool::EventPool() :
   ObjectPool(EventObjPoolId, MemDynamic, BlockSize, "Events")
{
   Debug::ft("EventPool.ctor");
}

//------------------------------------------------------------------------------

EventPool::~EventPool()
{
   Debug::ftnt("EventPool.dtor");
}

//------------------------------------------------------------------------------

void EventPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t MessagePool::BlockSize = sizeof(Message) + (40 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

MessagePool::MessagePool() :
   ObjectPool(MessageObjPoolId, MemDynamic, BlockSize, "Messages")
{
   Debug::ft("MessagePool.ctor");
}

//------------------------------------------------------------------------------

MessagePool::~MessagePool()
{
   Debug::ftnt("MessagePool.dtor");
}

//------------------------------------------------------------------------------

void MessagePool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t MsgPortPool::BlockSize = sizeof(MsgPort);

//------------------------------------------------------------------------------

MsgPortPool::MsgPortPool() :
   ObjectPool(MsgPortObjPoolId, MemDynamic, BlockSize, "MsgPorts")
{
   Debug::ft("MsgPortPool.ctor");
}

//------------------------------------------------------------------------------

MsgPortPool::~MsgPortPool()
{
   Debug::ftnt("MsgPortPool.dtor");
}

//------------------------------------------------------------------------------

MsgPort* MsgPortPool::FindPeerPort(const GlobalAddress& remAddr) const
{
   Debug::ft("MsgPortPool.FindPeerPort");

   PooledObjectId bid;

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

//  The identifier of the PSM currently being audited.
//
static NodeBase::PooledObjectId PsmToAudit_ = NIL_ID;

//------------------------------------------------------------------------------

ProtocolSMPool::ProtocolSMPool() :
   ObjectPool(ProtocolSMObjPoolId, MemDynamic, BlockSize, "ProtocolSMs")
{
   Debug::ft("ProtocolSMPool.ctor");
}

//------------------------------------------------------------------------------

ProtocolSMPool::~ProtocolSMPool()
{
   Debug::ftnt("ProtocolSMPool.dtor");
}

//------------------------------------------------------------------------------

void ProtocolSMPool::ClaimBlocks()
{
   Debug::ft("ProtocolSMPool.ClaimBlocks");

   //  Find the context for each in-use PSM and mark all the objects in its
   //  context as being in use.  A context that is not on a work queue MUST
   //  have a PSM, so its objects are claimed via PSMs.  This way, the audit
   //  will recover any context that is not on a work queue and has no PSM.
   //  If we encounter a corrupt PSM, the audit invokes this function again
   //  after recovering from a trap.  PsmToAudit_ therefore persists outside
   //  this function so that we can continue from where we left off.
   //
   size_t count = 0;

   auto psm = static_cast< ProtocolSM* >(NextUsed(PsmToAudit_));

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

      psm = static_cast< ProtocolSM* >(NextUsed(PsmToAudit_));
   }

   PsmToAudit_ = NIL_ID;
}

//------------------------------------------------------------------------------

void ProtocolSMPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ObjectPool::Display(stream, prefix, options);

   stream << prefix << "psmToAudit : " << PsmToAudit_ << CRLF;
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

ServiceSMPool::ServiceSMPool() :
   ObjectPool(ServiceSMObjPoolId, MemDynamic, BlockSize, "ServiceSMs")
{
   Debug::ft("ServiceSMPool.ctor");
}

//------------------------------------------------------------------------------

ServiceSMPool::~ServiceSMPool()
{
   Debug::ftnt("ServiceSMPool.dtor");
}

//------------------------------------------------------------------------------

void ServiceSMPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t TimerPool::BlockSize = sizeof(Timer);

//------------------------------------------------------------------------------

TimerPool::TimerPool() :
   ObjectPool(TimerObjPoolId, MemDynamic, BlockSize, "Timers")
{
   Debug::ft("TimerPool.ctor");

   timeouts_.reset(new Counter("timeout messages sent"));
}

//------------------------------------------------------------------------------

TimerPool::~TimerPool()
{
   Debug::ftnt("TimerPool.dtor");
}

//------------------------------------------------------------------------------

void TimerPool::ClaimBlocks()
{
   Debug::ft("TimerPool.ClaimBlocks");

   Singleton< TimerRegistry >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void TimerPool::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft("TimerPool.DisplayStats");

   ObjectPool::DisplayStats(stream, options);

   timeouts_->DisplayStat(stream, options);
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

void TimerPool::Shutdown(RestartLevel level)
{
   Debug::ft("TimerPool.Shutdown");

   FunctionGuard guard(Guard_MemUnprotect);
   Restart::Release(timeouts_);

   ObjectPool::Shutdown(level);
}

//------------------------------------------------------------------------------

void TimerPool::Startup(RestartLevel level)
{
   Debug::ft("TimerPool.Startup");

   ObjectPool::Startup(level);

   if(timeouts_ == nullptr)
   {
      FunctionGuard guard(Guard_MemUnprotect);
      timeouts_.reset(new Counter("timeout messages sent"));
   }
}

//==============================================================================

const size_t BtIpBufferPool::BlockSize = sizeof(SbIpBuffer);

//------------------------------------------------------------------------------

BtIpBufferPool::BtIpBufferPool() :
   ObjectPool(BtIpBufferObjPoolId, MemDynamic, BlockSize, "BtIpBuffers")
{
   Debug::ft("BtIpBufferPool.ctor");
}

//------------------------------------------------------------------------------

BtIpBufferPool::~BtIpBufferPool()
{
   Debug::ftnt("BtIpBufferPool.dtor");
}

//------------------------------------------------------------------------------

void BtIpBufferPool::ClaimBlocks()
{
   Debug::ft("BtIpBufferPool.ClaimBlocks");

   Singleton< TraceBuffer >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void BtIpBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}
}
