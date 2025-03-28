//==============================================================================
//
//  SbPools.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "SbPools.h"
#include <cstddef>
#include <iomanip>
#include <iosfwd>
#include <ostream>
#include <string>
#include <vector>
#include "Debug.h"
#include "Event.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "GlobalAddress.h"
#include "InvokerPoolRegistry.h"
#include "Message.h"
#include "MsgPort.h"
#include "NbAppIds.h"
#include "ProtocolSM.h"
#include "Restart.h"
#include "RootServiceSM.h"
#include "SbCliParms.h"
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
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Displays a summary of CTX in STREAM, under a header that contains
//    |SSM  State  PSM1  State  PSM2  State
//    |  3      7     6      7     6      7
//
static void SummarizeContext(ostream& stream, const Context* ctx)
{
   auto ssm = ctx->RootSsm();
   if(ssm != nullptr)
   {
      stream << setw(3) << ssm->Sid();
      stream << setw(7) << ssm->CurrState();
   }
   else
   {
      stream << setw(5) << "-";
      stream << setw(7) << "-";
   }

   auto psm = ctx->FirstPsm();
   if(psm != nullptr)
   {
      stream << setw(6) << psm->GetFactory();
      stream << setw(7) << psm->GetState();

      ctx->NextPsm(psm);
      if(psm != nullptr)
      {
         stream << setw(6) << psm->GetFactory();
         stream << setw(7) << psm->GetState();
      }
      else
      {
         stream << setw(6) << '-';
         stream << setw(7) << '-';
      }
   }
   else
   {
      stream << setw(6) << '-';
      stream << setw(7) << '-';
      stream << setw(6) << '-';
      stream << setw(7) << '-';
   }
}

//------------------------------------------------------------------------------

constexpr size_t SbIpBufferSize = sizeof(SbIpBuffer);

//------------------------------------------------------------------------------

SbIpBufferPool::SbIpBufferPool() :
   ObjectPool(SbIpBufferObjPoolId, MemSlab, SbIpBufferSize, "SbIpBuffers")
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

constexpr size_t ContextSize = sizeof(SsmContext);

//------------------------------------------------------------------------------

ContextPool::ContextPool() :
   ObjectPool(ContextObjPoolId, MemSlab, ContextSize, "Contexts")
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

   Singleton<InvokerPoolRegistry>::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void ContextPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string ContextHeader =
   "Type  SSM  State  PSM1  State  PSM2  State  Object";
// |   4..  3      7     6      7     6      7..<object>

size_t ContextPool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << ContextHeader << CRLF;
   size_t count = 0;

   auto items = GetUsed();

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid() && (*obj)->Passes(selector))
      {
         auto ctx = static_cast<const Context*>(*obj);
         stream << setw(4) << ctx->Type();
         stream << spaces(2);
         SummarizeContext(stream, ctx);
         stream << spaces(2) << strObj(*obj) << CRLF;
         ++count;
         ThisThread::PauseOver(90);
      }
   }

   if(count == 0)
   {
      stream << spaces(2) << NoContextsExpl << CRLF;
   }

   return count;
}

//==============================================================================

constexpr size_t EventSize = sizeof(Event) + (20 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

EventPool::EventPool() :
   ObjectPool(EventObjPoolId, MemSlab, EventSize, "Events")
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

constexpr size_t MessageSize = sizeof(Message) + (40 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

MessagePool::MessagePool() :
   ObjectPool(MessageObjPoolId, MemSlab, MessageSize, "Messages")
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

//------------------------------------------------------------------------------

fixed_string MessageHeader =
   "Pro  Sig  SSM  State  PSM1  State  PSM2  State  Object";
// |  3    5..  3      7     6      7     6      7..<object>

size_t MessagePool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << MessageHeader << CRLF;

   auto items = GetUsed();
   size_t count = 0;

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid() && (*obj)->Passes(selector))
      {
         auto msg = static_cast<const Message*>(*obj);
         stream << setw(3) << msg->GetProtocol();
         stream << setw(5) << msg->GetSignal();

         auto psm = msg->Psm();
         if(psm != nullptr)
         {
            stream << spaces(2);
            SummarizeContext(stream, psm->GetContext());
         }
         else
         {
            stream << setw(5) << '-';
            stream << setw(7) << '-';
            stream << setw(6) << '-';
            stream << setw(7) << '-';
            stream << setw(6) << '-';
            stream << setw(7) << '-';
         }

         stream << spaces(2) << strObj(*obj) << CRLF;
         ++count;
         ThisThread::PauseOver(90);
      }
   }

   if(count == 0)
   {
      stream << spaces(2) << NoMessagesExpl << CRLF;
   }

   return count;
}

//==============================================================================

constexpr size_t MsgPortSize = sizeof(MsgPort);

//------------------------------------------------------------------------------

MsgPortPool::MsgPortPool() :
   ObjectPool(MsgPortObjPoolId, MemSlab, MsgPortSize, "MsgPorts")
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
      auto port = static_cast<MsgPort*>(obj);

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

//------------------------------------------------------------------------------

fixed_string PortHeader = "SSM  State  PSM1  State  PSM2  State  Object";
//                        |  3      7     6      7     6      7..<object>

size_t MsgPortPool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << PortHeader << CRLF;

   auto items = GetUsed();
   size_t count = 0;

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid() && (*obj)->Passes(selector))
      {
         auto port = static_cast<const MsgPort*>(*obj);
         SummarizeContext(stream, port->GetContext());
         stream << spaces(2) << strObj(*obj) << CRLF;
         ++count;
         ThisThread::PauseOver(90);
      }
   }

   if(count == 0)
   {
      stream << spaces(2) << NoMsgsPortsExpl << CRLF;
   }

   return count;
}

//==============================================================================

constexpr size_t ProtocolSMSize = sizeof(ProtocolSM) + (60 * BYTES_PER_WORD);

//  The identifier of the PSM currently being audited.
//
static NodeBase::PooledObjectId PsmToAudit_ = NIL_ID;

//------------------------------------------------------------------------------

ProtocolSMPool::ProtocolSMPool() :
   ObjectPool(ProtocolSMObjPoolId, MemSlab, ProtocolSMSize, "ProtocolSMs")
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

   auto psm = static_cast<ProtocolSM*>(NextUsed(PsmToAudit_));

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

      psm = static_cast<ProtocolSM*>(NextUsed(PsmToAudit_));
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

//------------------------------------------------------------------------------

fixed_string PsmHeader = "SSM  State  PSM1  State  PSM2  State  Object";
//                       |  3      7     6      7     6      7..<object>

size_t ProtocolSMPool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << PsmHeader << CRLF;

   auto items = GetUsed();
   size_t count = 0;

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid() && (*obj)->Passes(selector))
      {
         auto psm = static_cast<const ProtocolSM*>(*obj);
         SummarizeContext(stream, psm->GetContext());
         stream << spaces(2) << strObj(*obj) << CRLF;
         ++count;
         ThisThread::PauseOver(90);
      }
   }

   if(count == 0)
   {
      stream << spaces(2) << NoPsmsExpl << CRLF;
   }

   return count;
}

//==============================================================================

constexpr size_t ServiceSMSize = sizeof(RootServiceSM) + (60 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

ServiceSMPool::ServiceSMPool() :
   ObjectPool(ServiceSMObjPoolId, MemSlab, ServiceSMSize, "ServiceSMs")
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

//------------------------------------------------------------------------------

fixed_string SsmHeader = "SSM  State  PSM1  State  PSM2  State  Object";
//                       |  3      7     6      7     6      7..<object>

size_t ServiceSMPool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << SsmHeader << CRLF;

   auto items = GetUsed();
   size_t count = 0;

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid() && (*obj)->Passes(selector))
      {
         auto ssm = static_cast<const ServiceSM*>(*obj);
         SummarizeContext(stream, ssm->GetContext());
         stream << spaces(2) << strObj(*obj) << CRLF;
         ++count;
         ThisThread::PauseOver(90);
      }
   }

   if(count == 0)
   {
      stream << spaces(2) << NoSsmsExpl << CRLF;
   }

   return count;
}

//==============================================================================

constexpr size_t TimerSize = sizeof(Timer);

//------------------------------------------------------------------------------

TimerPool::TimerPool() :
   ObjectPool(TimerObjPoolId, MemSlab, TimerSize, "Timers")
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

   Singleton<TimerRegistry>::Instance()->ClaimBlocks();
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

//------------------------------------------------------------------------------

fixed_string TimerHeader =
   " Id  PSM  SSM  State  PSM1  State  PSM2  State  Object";
// |  3..  5    5      7     6      7     6      7..<object>

size_t TimerPool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << TimerHeader << CRLF;

   auto items = GetUsed();
   size_t count = 0;

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid() && (*obj)->Passes(selector))
      {
         auto tmr = static_cast<const Timer*>(*obj);
         stream << setw(3) << tmr->Tid();
         stream << setw(3) << tmr->Psm()->GetFactory();
         stream << spaces(2);
         SummarizeContext(stream, tmr->Psm()->GetContext());
         stream << spaces(2) << strObj(*obj) << CRLF;
         ++count;
         ThisThread::PauseOver(90);
      }
   }

   if(count == 0)
   {
      stream << spaces(2) << NoTimersExpl << CRLF;
   }

   return count;
}

//==============================================================================

constexpr size_t BtIpBufferSize = sizeof(SbIpBuffer);

//------------------------------------------------------------------------------

BtIpBufferPool::BtIpBufferPool() :
   ObjectPool(BtIpBufferObjPoolId, MemSlab, BtIpBufferSize, "BtIpBuffers")
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

   Singleton<TraceBuffer>::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void BtIpBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}
}
