//==============================================================================
//
//  InvokerThread.cpp.
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
#include "InvokerThread.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "InvokerPoolRegistry.h"
#include "Restart.h"
#include "SbDaemons.h"
#include "SbInvokerPools.h"
#include "Singleton.h"
#include "ToolTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
word InvokerThread::RtcYieldPercent_ = 90;
const InvokerThread* InvokerThread::RunningInvoker_ = nullptr;

//------------------------------------------------------------------------------

fn_name InvokerThread_ctor = "InvokerThread.ctor";

InvokerThread::InvokerThread(Faction faction, Daemon* daemon) :
   Thread(faction, daemon),
   pool_(nullptr),
   ctx_(nullptr),
   msg_(nullptr),
   trans_(0),
   ticks0_(0)
{
   Debug::ft(InvokerThread_ctor);

   pool_ = Singleton< InvokerPoolRegistry >::Instance()->Pool(faction);
   Debug::Assert(pool_->BindThread(*this));
   SetInitialized();
}

//------------------------------------------------------------------------------

fn_name InvokerThread_dtor = "InvokerThread.dtor";

InvokerThread::~InvokerThread()
{
   Debug::ft(InvokerThread_dtor);

   if(RunningInvoker_ == this) RunningInvoker_ = nullptr;
   pool_->UnbindThread(*this);

   //  If we have a context, handle it the same way as during a restart.
   //
   Shutdown(Restart::GetLevel());
}

//------------------------------------------------------------------------------

c_string InvokerThread::AbbrName() const
{
   return InvokerDaemonName;
}

//------------------------------------------------------------------------------

fn_name InvokerThread_BlockingAllowed = "InvokerThread.BlockingAllowed";

bool InvokerThread::BlockingAllowed(BlockingReason why, fn_name_arg func)
{
   Debug::ft(InvokerThread_BlockingAllowed);

   //  An invoker thread can sleep at will, but an application must not block
   //  the last invoker thread that is ready to service the work queues.
   //
   switch(why)
   {
   case BlockedOnClock:
      break;

   case BlockedOnDatabase:
      if(pool_->ReadyCount() <= 1) return false;
      break;

   default:
      Debug::SwLog(InvokerThread_BlockingAllowed, "invalid reason", why);
      return false;
   }

   msg_ = Context::ContextMsg();
   Context::SetContextMsg(nullptr);
   pool_->ScheduledOut();
   RunningInvoker_ = nullptr;
   return true;
}

//------------------------------------------------------------------------------

TraceStatus InvokerThread::CalcStatus(bool dynamic) const
{
   if(dynamic && (ctx_ != nullptr) && ctx_->TraceOn()) return TraceIncluded;
   return Thread::CalcStatus(dynamic);
}

//------------------------------------------------------------------------------

ptrdiff_t InvokerThread::CellDiff2()
{
   int local;
   auto fake = reinterpret_cast< const InvokerThread* >(&local);
   return ptrdiff(&fake->iid_, fake);
}

//------------------------------------------------------------------------------

fn_name InvokerThread_ClearContext = "InvokerThread.ClearContext";

void InvokerThread::ClearContext()
{
   Debug::ft(InvokerThread_ClearContext);

   ctx_.release();
}

//------------------------------------------------------------------------------

void InvokerThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "iid    : " << iid_.to_str() << CRLF;
   stream << prefix << "ctx    : " << ctx_.get() << CRLF;
   stream << prefix << "msg    : " << msg_ << CRLF;
   stream << prefix << "trans  : " << trans_ << CRLF;
   stream << prefix << "ticks0 : " << ticks0_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name InvokerThread_Enter = "InvokerThread.Enter";

void InvokerThread::Enter()
{
   Debug::ft(InvokerThread_Enter);

   //  Make ourselves the running invoker and tell our pool to process work.
   //
   RunningInvoker_ = this;
   pool_->ProcessWork();

   //  ProcessWork is not supposed to return.
   //
   RunningInvoker_ = nullptr;
   Debug::SwLog(InvokerThread_Enter, "ProcessWork should not return", Tid());
}

//------------------------------------------------------------------------------

void InvokerThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name InvokerThread_Recover = "InvokerThread.Recover";

bool InvokerThread::Recover()
{
   Debug::ft(InvokerThread_Recover);

   //  If a restart is underway, just exit, which is what we wanted to do
   //  anyway.
   //
   if(Restart::GetLevel() >= RestartWarm)
   {
      return false;
   }

   if(ctx_ != nullptr)
   {
      ctx_->Dump();

      //  The implementation of unique_ptr.reset may be equivalent to this:
      //    item = this->ptr_;
      //    this->ptr_ = nullptr;
      //    delete item;
      //  That is, the unique_ptr does not point to the item being deleted while
      //  its destructor is executing.  Consequently, Context::RunningContext,
      //  which depends on our ctx_ field, cannot find the running context (the
      //  one being deleted).  So instead of simply invoking ctx_.reset, we do
      //  the following so that ctx_ will be valid during deletion:
      //
      auto ctx = ctx_.get();
      delete ctx;
      ctx_.release();
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name InvokerThread_ScheduledIn = "InvokerThread.ScheduledIn";

void InvokerThread::ScheduledIn(fn_name_arg func)
{
   Debug::ft(InvokerThread_ScheduledIn);

   RunningInvoker_ = this;
   trans_ = 0;
   Context::SetContextMsg(msg_);
   msg_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name InvokerThread_SetContext = "InvokerThread.SetContext";

void InvokerThread::SetContext(Context* ctx)
{
   Debug::ft(InvokerThread_SetContext);

   //  Stupid unique_ptr tricks #2.  This can be invoked when ctx_.get() == ctx,
   //  in which case ctx_ must first be cleared to avoid a tragic deletion.  The
   //  reason for this is that unique_ptr may not check for reassignment.
   //
   ctx_.release();
   ctx_.reset(ctx);
   ++trans_;
   ticks0_ = Clock::TicksNow();
}

//------------------------------------------------------------------------------

fn_name InvokerThread_Shutdown = "InvokerThread.Shutdown";

void InvokerThread::Shutdown(RestartLevel level)
{
   Debug::ft(InvokerThread_Shutdown);

   //  Our destructor always invokes this, and it is also invoked if we
   //  failed to exit during a restart.
   //  o If no restart is underway, there is nothing to do.
   //  o During a cold restart, our context's heap will be deleted, so
   //    nullify it.
   //  o During a warm restart, put our context back on a work queue so
   //    that it can be serviced after the restart is over.
   //
   switch(level)
   {
   case RestartNil:
      break;

   case RestartWarm:
      if(ctx_ == nullptr) return;
      Singleton< PayloadInvokerPool >::Instance()->Requeue(*ctx_.release());
      break;

   default:
      ctx_.release();
   }
}
}
