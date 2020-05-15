//==============================================================================
//
//  Context.cpp
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
#include "Context.h"
#include "SoftwareException.h"
#include <cstdint>
#include <iosfwd>
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "Formatters.h"
#include "GlobalAddress.h"
#include "InvokerPool.h"
#include "InvokerPoolRegistry.h"
#include "InvokerThread.h"
#include "LocalAddress.h"
#include "Log.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "Parameter.h"
#include "ProtocolSM.h"
#include "Q2Way.h"
#include "Restart.h"
#include "SbLogs.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "SbTracer.h"
#include "Signal.h"
#include "Singleton.h"
#include "ThisThread.h"
#include "TimerProtocol.h"
#include "TlvMessage.h"
#include "TlvParameter.h"
#include "Tool.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Context::Kill throws this to destroy the running context.
//
class SbException : public SoftwareException
{
public:
   //  ERRVAL and OFFSET are passed to Software Exception.
   //
   SbException(debug64_t errval, debug64_t offset);

   //  ERRSTR and OFFSET are passed to Software Exception.
   //
   SbException(const string& errstr, debug64_t offset);

   //  Not subclassed.
   //
   ~SbException();

   //  Overridden to display member variables.
   //
   void Display(ostream& stream, const string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   const char* what() const noexcept override;

   //  The context that was running when the exception occurred.
   //
   const Context* ctx_;
};

//------------------------------------------------------------------------------

fn_name SbException_ctor1 = "SbException.ctor";

SbException::SbException(debug64_t errval, debug64_t offset) :
   SoftwareException(errval, offset, 2),
   ctx_(nullptr)
{
   Debug::ft(SbException_ctor1);

   auto inv = InvokerThread::RunningInvoker_;
   if(inv != nullptr) ctx_ = inv->GetContext();
}

//------------------------------------------------------------------------------

fn_name SbException_ctor2 = "SbException.ctor(string)";

SbException::SbException(const string& errstr, debug64_t offset) :
   SoftwareException(errstr, offset, 2),
   ctx_(nullptr)
{
   Debug::ft(SbException_ctor2);

   auto inv = InvokerThread::RunningInvoker_;
   if(inv != nullptr) ctx_ = inv->GetContext();
}

//------------------------------------------------------------------------------

fn_name SbException_dtor = "SbException.dtor";

SbException::~SbException()
{
   Debug::ft(SbException_dtor);
}

//------------------------------------------------------------------------------

void SbException::Display(ostream& stream, const string& prefix) const
{
   SoftwareException::Display(stream, prefix);

   stream << prefix << "context : " << ctx_ << CRLF;
}

//------------------------------------------------------------------------------

fixed_string SbExceptionExpl = "SessionBase Exception";

const char* SbException::what() const noexcept
{
   return SbExceptionExpl;
}

//==============================================================================

const Context::MessageEntry Context::NilMessageEntry =
   {MsgIncoming, NIL_ID, NIL_ID};

Message* Context::ContextMsg_ = nullptr;

//------------------------------------------------------------------------------

fn_name Context_ctor = "Context.ctor";

Context::Context(Faction faction) :
   whichq_(nullptr),
   enqTime_(0),
   pool_(nullptr),
   thread_(nullptr),
   faction_(faction),
   state_(Dormant),
   prio_(INGRESS),
   traceOn_(false),
   trans_(nullptr),
   buffIndex_(0)
{
   Debug::ft(Context_ctor);

   priMsgq_.Init(Pooled::LinkDiff());
   stdMsgq_.Init(Pooled::LinkDiff());

   pool_ = Singleton< InvokerPoolRegistry >::Instance()->Pool(faction_);

   if(pool_ == nullptr)
   {
      Debug::SwLog(Context_ctor, "invoker pool not found", faction_);
   }

   for(size_t i = 0; i < TraceSize; ++i) trace_[i] = NilMessageEntry;

   if(Debug::TraceOn())
   {
      traceOn_ = Singleton< TraceBuffer >::Instance()->FilterIsOn(TraceAll);
   }
}

//------------------------------------------------------------------------------

fn_name Context_dtor = "Context.dtor";

Context::~Context()
{
   Debug::ft(Context_dtor);

   //  Purge queued objects, remove ourselves from any queue, and make sure
   //  that no one thinks we're currently running.
   //
   priMsgq_.Purge();
   stdMsgq_.Purge();

   Exqueue();

   //  The following guards against a context being deleted while its
   //  invoker thread is sleeping.  When the invoker thread wakes up, it
   //  will try to perform more work on behalf of the deleted context if
   //  we don't tell it otherwise.
   //
   if((thread_ != nullptr) && (thread_->GetContext() == this))
   {
      thread_->ClearContext();
   }
}

//------------------------------------------------------------------------------

fn_name Context_CaptureTask = "Context.CaptureTask";

void Context::CaptureTask(const Message& msg, const InvokerThread* inv)
{
   Debug::ft(Context_CaptureTask);

   auto sbt = Singleton< SbTracer >::Instance();

   if(!TraceOn()) SetTrace(sbt->MsgStatus(msg, MsgIncoming) == TraceIncluded);

   if(TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      auto warp = TimePoint::Now();

      if(buff->ToolIsOn(TransTracer))
      {
         auto rec = trans_ = new TransTrace(*this, msg, inv);
         if(!buff->Insert(rec)) trans_ = nullptr;
      }

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new MsgTrace(MsgTrace::Reception, msg, msg.Header()->route);
         buff->Insert(rec);
      }

      if(trans_ != nullptr) trans_->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

fn_name Context_Cleanup = "Context.Cleanup";

void Context::Cleanup()
{
   Debug::ft(Context_Cleanup);

   //  If the context is on a work queue, it had better be exqueued, because
   //  a work queue corruption causes a restart.
   //
   Exqueue();

   if((thread_ != nullptr) && (thread_->GetContext() == this))
   {
      thread_->ClearContext();
   }

   Pooled::Cleanup();
}

//------------------------------------------------------------------------------

fn_name Context_ContextPsm = "Context.ContextPsm";

ProtocolSM* Context::ContextPsm()
{
   Debug::ft(Context_ContextPsm);

   if(ContextMsg_ == nullptr) return nullptr;
   return ContextMsg_->Psm();
}

//------------------------------------------------------------------------------

fn_name Context_ContextRoot = "Context.ContextRoot";

RootServiceSM* Context::ContextRoot()
{
   Debug::ft(Context_ContextRoot);

   auto ctx = RunningContext();
   if(ctx == nullptr) return nullptr;
   return ctx->RootSsm();
}

//------------------------------------------------------------------------------

fn_name Context_Corrupt = "Context.Corrupt";

void Context::Corrupt()
{
   Debug::ft(Context_Corrupt);

   if(Element::RunningInLab()) stdMsgq_.Corrupt(nullptr);
}

//------------------------------------------------------------------------------

void Context::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "whichq  : " << whichq_ << CRLF;
   stream << prefix << "link    : " << CRLF;
   link_.Display(stream, prefix + spaces(2));
   stream << prefix << "priMsgq : " << CRLF;
   priMsgq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "stdMsgq : " << CRLF;
   stdMsgq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "enqTime : " << enqTime_.Ticks() << CRLF;
   stream << prefix << "pool    : " << pool_ << CRLF;
   stream << prefix << "thread  : " << thread_ << CRLF;
   stream << prefix << "faction : " << int(faction_) << CRLF;
   stream << prefix << "state   : " << state_ << CRLF;
   stream << prefix << "prio    : " << int(prio_) << CRLF;
   stream << prefix << "traceOn : " << traceOn_ << CRLF;
   stream << prefix << "trans   : " << trans_ << CRLF;
   stream << prefix << "trace : " << strTrace() << CRLF;
}

//------------------------------------------------------------------------------

fn_name Context_Dump = "Context.Dump";

void Context::Dump(fn_name_arg func, debug64_t errval, debug64_t offset)
{
   Debug::ft(Context_Dump);

   auto ctx = RunningContext();

   if(ctx != nullptr)
   {
      Debug::SwLog(func, errval, offset);
      ctx->Dump();
   }
}

//------------------------------------------------------------------------------

void Context::Dump() const
{
   auto log = Log::Create(SessionLogGroup, SessionError);
   if(log == nullptr) return;
   LogSubtended(*log, Log::Tab, NoFlags);
   Log::Submit(log);
}

//------------------------------------------------------------------------------

fn_name Context_EnqMsg = "Context.EnqMsg";

bool Context::EnqMsg(Message& msg)
{
   Debug::ft(Context_EnqMsg);

   if(IsCorrupt()) return false;

   if(msg.Header()->priority != IMMEDIATE)
      msg.Enqueue(stdMsgq_);
   else
      msg.Enqueue(priMsgq_);

   return true;
}

//------------------------------------------------------------------------------

fn_name Context_EnqPort = "Context.EnqPort";

void Context::EnqPort(MsgPort& port)
{
   Debug::ft(Context_EnqPort);

   //  This is overridden by contexts that support ports.
   //
   Kill(strOver(this), port.LocAddr().Fid());
}

//------------------------------------------------------------------------------

fn_name Context_EnqPsm = "Context.EnqPsm";

void Context::EnqPsm(ProtocolSM& psm)
{
   Debug::ft(Context_EnqPsm);

   //  This is overridden by contexts that support PSMs.
   //
   Kill(strOver(this), psm.GetFactory());
}

//------------------------------------------------------------------------------

fn_name Context_Enqueue = "Context.Enqueue";

void Context::Enqueue(Q2Way< Context >& whichq, MsgPriority prio, bool henq)
{
   Debug::ft(Context_Enqueue);

   if(IsCorrupt()) return;

   //  Only a dormant context should be placed on a work queue:
   //  o If the context has paused, it will continue to process messages
   //    (including the one just queued against it) when its invoker thread
   //    resumes execution.
   //  o If the context is ready, it is already on a work queue and need not
   //    be queued again.  A context does not move to a higher priority queue
   //    unless it receives a message of immediate priority.  All messages are
   //    handled in FIFO order, and the priority of the first one determines
   //    the queue on which the context appears.
   //
   switch(state_)
   {
   case Running:
      //  This can occur as a result of SendToSelf, where a message is sent
      //  to a context while it is running.  The context does not have to be
      //  placed on a work queue; it will dequeue the message after it has
      //  finished processing its current message.  It can also occur during
      //  a warm restart, when an invoker thread requeues the context that
      //  it couldn't service before exiting.
      //
      if(Restart::GetStage() == RestartStage::Running)
         return;
      else
         henq = true;
      //  [[fallthrough]]
   case Dormant:
      if(henq)
         whichq.Henq(*this);
      else
         whichq.Enq(*this);

      whichq_ = &whichq;
      state_ = Ready;
      prio_ = prio;
      enqTime_ = TimePoint::Now();
      pool_->Enqueued(prio_);
      return;

   case Ready:
      //
      //  If we just received a message of immediate priority, move to the
      //  immediate priority work queue if we're not already there.
      //
      if((prio == IMMEDIATE) && (prio_ != IMMEDIATE))
      {
         Exqueue();
         Enqueue(whichq, IMMEDIATE, false);
         return;
      }

      //  When a context on the ingress queue receives a subsequent message,
      //  it usually means that the request (the ingress work) has either been
      //  retransmitted or cancelled.  In the first case, the retransmitted
      //  message can be deleted.  In the second case, the entire context can
      //  be deleted.
      //
      if(prio_ == INGRESS)
      {
         auto fac = stdMsgq_.First()->RxFactory();

         if(!fac->ScreenIcMsgs(stdMsgq_))
         {
            delete this;
         }
      }
      return;

   case Paused:
      //
      //  It is legitimate to receive a message while sleeping.  It will be
      //  handled when our invoker resumes execution.  However, if this is a
      //  priority message, it must be handled immediately.
      //
      if((prio == IMMEDIATE) && (prio_ != IMMEDIATE))
      {
         thread_->ClearContext();
         SetState(Dormant);
         Enqueue(whichq, IMMEDIATE, false);
      }
      return;

   default:
      //
      //  An unknown state.
      //
      Debug::SwLog(Context_Enqueue, debug64_t(this), state_);
      delete this;
   }
}

//------------------------------------------------------------------------------

fn_name Context_ExqPort = "Context.ExqPort";

void Context::ExqPort(MsgPort& port)
{
   Debug::ft(Context_ExqPort);

   //  This is overridden by contexts that support ports.
   //
   Kill(strOver(this), port.LocAddr().Fid());
}

//------------------------------------------------------------------------------

fn_name Context_ExqPsm = "Context.ExqPsm";

void Context::ExqPsm(ProtocolSM& psm)
{
   Debug::ft(Context_ExqPsm);

   //  This is overridden by contexts that support PSMs.
   //
   Kill(strOver(this), psm.GetFactory());
}

//------------------------------------------------------------------------------

fn_name Context_Exqueue = "Context.Exqueue";

void Context::Exqueue()
{
   Debug::ft(Context_Exqueue);

   if(whichq_ == nullptr)
   {
      if(state_ == Ready)
         Debug::SwLog(Context_Exqueue, debug64_t(this), state_);
      return;
   }

   if(state_ != Ready)
      Debug::SwLog(Context_Exqueue, debug64_t(this), state_);

   whichq_->Exq(*this);
   whichq_ = nullptr;
   SetState(Dormant);
   pool_->Dequeued(prio_);
}

//------------------------------------------------------------------------------

fn_name Context_GetSubtended = "Context.GetSubtended";

void Context::GetSubtended(std::vector< Base* >& objects) const
{
   Debug::ft(Context_GetSubtended);

   Pooled::GetSubtended(objects);

   for(auto m = priMsgq_.First(); m != nullptr; priMsgq_.Next(m))
   {
      m->GetSubtended(objects);
   }

   for(auto m = stdMsgq_.First(); m != nullptr; stdMsgq_.Next(m))
   {
      m->GetSubtended(objects);
   }
}

//------------------------------------------------------------------------------

fn_name Context_HenqPsm = "Context.HenqPsm";

void Context::HenqPsm(ProtocolSM& psm)
{
   Debug::ft(Context_HenqPsm);

   //  This is overridden by contexts that support PSMs.
   //
   Kill(strOver(this), psm.GetFactory());
}

//------------------------------------------------------------------------------

fn_name Context_Kill1 = "Context.Kill";

void Context::Kill(debug64_t errval, debug64_t offset)
{
   Debug::ft(Context_Kill1);

   throw SbException(errval, offset);
}

//------------------------------------------------------------------------------

fn_name Context_Kill2 = "Context.Kill(string)";

void Context::Kill(const string& errstr, debug64_t offset)
{
   Debug::ft(Context_Kill2);

   throw SbException(errstr, offset);
}

//------------------------------------------------------------------------------

ptrdiff_t Context::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Context* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name Context_MsgCount = "Context.MsgCount";

size_t Context::MsgCount(bool priority, bool standard) const
{
   Debug::ft(Context_MsgCount);

   size_t size = 0;

   if(priority) size += priMsgq_.Size();
   if(standard) size += stdMsgq_.Size();
   return size;
}

//------------------------------------------------------------------------------

fn_name Context_new = "Context.operator new";

void* Context::operator new(size_t size)
{
   Debug::ft(Context_new);

   return Singleton< ContextPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void Context::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Context_ProcessIcMsg = "Context.ProcessIcMsg";

void Context::ProcessIcMsg(Message& msg)
{
   Debug::ft(Context_ProcessIcMsg);

   Kill(strOver(this), msg.Header()->rxAddr.fid);
}

//------------------------------------------------------------------------------

fn_name Context_ProcessMsg = "Context.ProcessMsg";

bool Context::ProcessMsg(Q1Way< Message >& msgq, const InvokerThread* inv)
{
   Debug::ft(Context_ProcessMsg);

   bool deleted = false;

   //  Note that the context message is not dequeued.  It remains at the
   //  head of the queue, and it is up to subclasses to move it to another
   //  location.
   //
   auto msg = msgq.First();
   SetContextMsg(msg);

   //  If transactions are being traced, capture this transaction.
   //
   if(Debug::TraceOn()) CaptureTask(*msg, inv);

   //  Kill the context if requested.
   //
   if(msg->Header()->kill)
   {
      Kill("killed remotely", 0);
      return true;
   }

   //  Tell the context to process the current message.
   //
   ProcessIcMsg(*msg);

   //  If the message is still at the head of the queue, delete it
   //  (this has the side effect of clearing the context message).
   //
   if(msgq.First() == msg)
      delete msg;
   else
      SetContextMsg(nullptr);

   //  If the context is idle, delete it.
   //
   auto trans = trans_;
   trans_ = nullptr;

   if(IsIdle())
   {
      delete this;
      deleted = true;
   }

   //  If transactions are being traced, capture the time when this
   //  transaction ended.
   //
   if(trans != nullptr) trans->EndOfTransaction();

   //  Return false if the context was deleted.
   //
   return !deleted;
}

//------------------------------------------------------------------------------

fn_name Context_ProcessWork = "Context.ProcessWork";

void Context::ProcessWork(InvokerThread* inv)
{
   //  SetState sets the running context, so trace this function afterwards.
   //
   SetState(Running);

   Debug::ft(Context_ProcessWork);

   thread_ = inv;
   if(thread_ == nullptr) return;

   auto delay = TimePoint::Now() - enqTime_;

   pool_->RecordDelay(prio_, delay);

   while(true)
   {
      //  If there are messages on the priority queue, process all of them.
      //
      while(!priMsgq_.Empty())
      {
         //  Process a priority message.  Return if this frees the context.
         //
         if(!ProcessMsg(priMsgq_, thread_)) return;

         //  After all priority messages are handled, enter the dormant
         //  state. If a standard message is pending, immediately reenter
         //  a work queue.
         //
         if(priMsgq_.Empty())
         {
            SetState(Dormant);
            if(!stdMsgq_.Empty()) pool_->Requeue(*this);
            return;
         }

         thread_->SetContext(this);
      }

      if(!stdMsgq_.Empty())
      {
         //  Process a standard message.  Return if this frees the context.
         //
         if(!ProcessMsg(stdMsgq_, thread_)) return;

         //  If a priority message has arrived, control passes to the top
         //  of the while(true) loop so that the priority message queue is
         //  serviced.  This shouldn't happen while we're running, but...
         //
         if(priMsgq_.Empty())
         {
            //  If there are no more messages, enter the dormant state.
            //
            if(stdMsgq_.Empty())
            {
               SetState(Dormant);
               return;
            }

            //  There is another message on the queue.  The entire queue
            //  will be processed, but we cannot run unpreemptably too
            //  long.  If we still have enough time left, process the next
            //  message, else yield so that other threads can run.  While
            //  this invoker thread is sleeping, the context could receive
            //  an immediate priority message or even be deleted: in either
            //  case, this invoker thread will no longer own this context.
            //  It is therefore necessary, before handling more work, to
            //  verify that the running thread still owns this context.
            //
            if(ThisThread::RtcPercentUsed() > InvokerThread::RtcYieldPercent())
            {
               SetState(Paused);
               ThisThread::Pause();

               inv = static_cast< InvokerThread* >(Thread::RunningThread());
               if(inv->GetContext() != this) return;
               SetState(Running);
            }
         }

         thread_->SetContext(this);
      }
      else
      {
         //  Bizarre.  We were invoked (or resumed execution after yielding)
         //  but didn't have any messages to process.
         //
         Debug::SwLog(Context_ProcessWork,
            "message queue empty", pack2(faction_, state_));

         if(IsIdle())
            delete this;
         else
            SetState(Dormant);
         return;
      }
   }
}

//------------------------------------------------------------------------------

fn_name Context_RunningContext = "Context.RunningContext";

Context* Context::RunningContext()
{
   Debug::ft(Context_RunningContext);

   auto inv = InvokerThread::RunningInvoker_;
   if(inv != nullptr) return inv->GetContext();
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Context_RunningContextTraced = "Context.RunningContextTraced";

bool Context::RunningContextTraced(TransTrace*& trans)
{
   Debug::ft(Context_RunningContextTraced);

   auto inv = InvokerThread::RunningInvoker_;

   if(inv != nullptr)
   {
      auto ctx = inv->GetContext();
      if(ctx != nullptr) return ctx->TraceOn(trans);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Context_SetContextMsg = "Context.SetContextMsg";

void Context::SetContextMsg(Message* msg)
{
   Debug::ft(Context_SetContextMsg);

   ContextMsg_ = msg;
}

//------------------------------------------------------------------------------

fn_name Context_SetState = "Context.SetState";

void Context::SetState(State state)
{
   Debug::ft(Context_SetState);

   state_ = state;

   switch(state_)
   {
   case Running:
   case Paused:
      return;

   case Dormant:
   case Ready:
      thread_ = nullptr;
      return;

   default:
      //
      //  An unknown state.
      //
      Debug::SwLog(Context_SetState, debug64_t(this), state_);
      delete this;
      return;
   }
}

//------------------------------------------------------------------------------

fn_name Context_StopTimer = "Context.StopTimer";

bool Context::StopTimer(const Base& owner, TimerId tid)
{
   Debug::ft(Context_StopTimer);

   //  Search our message queue for a timeout message that is owned by
   //  OWNER and identified by TID.  Delete the message if found.
   //
   for(auto m = stdMsgq_.First(); m != nullptr; stdMsgq_.Next(m))
   {
      if(m->GetSignal() == Signal::Timeout)
      {
         auto pptr = static_cast< TlvMessage* >
            (m)->FindParm(Parameter::Timeout);

         if(pptr != nullptr)
         {
            auto toi = reinterpret_cast< TimeoutInfo* >(pptr->bytes);

            if((toi->tid == tid) && (toi->owner == &owner))
            {
               delete m;
               return true;
            }
         }
         else
         {
            Debug::SwLog(Context_StopTimer, debug64_t(m), tid);
         }
      }
   }

   return false;
}

//------------------------------------------------------------------------------

string Context::strTrace() const
{
   std::ostringstream stream;

   size_t i = buffIndex_;
   bool one = false;

   while(true)
   {
      auto entry = trace_[i];

      if(entry.sid != NIL_ID)
      {
         if(one)
            stream << SPACE;
         else
            one = true;

         if(entry.dir == MsgIncoming) stream << '>';
         stream << int(entry.prid) << ',';
         stream << int(entry.sid);
         if(entry.dir != MsgIncoming) stream << '>';
      }

      if(++i == TraceSize) i = 0;
      if(i == buffIndex_) break;
   }

   return stream.str();
}

//------------------------------------------------------------------------------

fn_name Context_TraceMsg = "Context.TraceMsg";

void Context::TraceMsg(ProtocolId prid, SignalId sid, MsgDirection dir)
{
   Debug::ft(Context_TraceMsg);

   trace_[buffIndex_].prid = prid;
   trace_[buffIndex_].sid = sid;
   trace_[buffIndex_].dir = dir;
   if(++buffIndex_ >= TraceSize) buffIndex_ = 0;
}

//------------------------------------------------------------------------------

bool Context::TraceOn()
{
   if(!traceOn_) return false;
   if(Debug::TraceOn()) return true;
   traceOn_ = false;
   return false;
}

//------------------------------------------------------------------------------

bool Context::TraceOn(TransTrace*& trans)
{
   if(!TraceOn()) return false;
   trans = trans_;
   return true;
}

//------------------------------------------------------------------------------

fn_name Context_Type = "Context.Type";

ContextType Context::Type() const
{
   Debug::ft(Context_Type);

   Debug::SwLog(Context_Type, strOver(this), 0);
   return SingleMsg;
}
}
