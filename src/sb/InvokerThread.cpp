//==============================================================================
//
//  InvokerThread.cpp.
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
#include "InvokerThread.h"
#include <chrono>
#include <cstdint>
#include <ostream>
#include <ratio>
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
word InvokerThread::RtcYieldPercent_ = 83;
const InvokerThread* InvokerThread::RunningInvoker_ = nullptr;

//------------------------------------------------------------------------------

InvokerThread::InvokerThread(Faction faction, Daemon* daemon) :
   Thread(faction, daemon),
   pool_(nullptr),
   ctx_(nullptr),
   msg_(nullptr),
   trans_(0)
{
   Debug::ft("InvokerThread.ctor");

   pool_ = Singleton<InvokerPoolRegistry>::Instance()->Pool(faction);
   Debug::Assert(pool_->BindThread(*this));
   SetInitialized();
}

//------------------------------------------------------------------------------

InvokerThread::~InvokerThread()
{
   Debug::ftnt("InvokerThread.dtor");

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
   uintptr_t local;
   auto fake = reinterpret_cast<const InvokerThread*>(&local);
   return ptrdiff(&fake->iid_, fake);
}

//------------------------------------------------------------------------------

void InvokerThread::ClearContext()
{
   Debug::ft("InvokerThread.ClearContext");

   ctx_.release();
}

//------------------------------------------------------------------------------

void InvokerThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "iid   : " << iid_.to_str() << CRLF;
   stream << prefix << "ctx   : " << ctx_.get() << CRLF;
   stream << prefix << "msg   : " << msg_ << CRLF;
   stream << prefix << "trans : " << trans_ << CRLF;
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

bool InvokerThread::Recover()
{
   Debug::ft("InvokerThread.Recover");

   //  If a restart is underway, just exit, which is what we wanted to do
   //  anyway.
   //
   if(Restart::GetStage() != Running)
   {
      return false;
   }

   if(ctx_ != nullptr)
   {
      ctx_->Dump();

      //  The implementation of unique_ptr::reset may be equivalent to this:
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

void InvokerThread::ScheduledIn(fn_name_arg func)
{
   Debug::ft("InvokerThread.ScheduledIn");

   RunningInvoker_ = this;
   trans_ = 0;
   Context::SetContextMsg(msg_);
   msg_ = nullptr;
}

//------------------------------------------------------------------------------

void InvokerThread::SetContext(Context* ctx)
{
   Debug::ft("InvokerThread.SetContext");

   //  Stupid unique_ptr tricks #2.  This can be invoked when ctx_.get() == ctx,
   //  in which case ctx_ must first be cleared to avoid a tragic deletion.  The
   //  reason for this is that unique_ptr may not check for reassignment.
   //
   ctx_.release();
   ctx_.reset(ctx);
   ++trans_;
   time0_ = SteadyTime::Now();
}

//------------------------------------------------------------------------------

void InvokerThread::Shutdown(RestartLevel level)
{
   Debug::ft("InvokerThread.Shutdown");

   //  Our destructor always invokes this, and it is also invoked if we
   //  failed to exit during a restart.
   //  o If no restart is underway, there is nothing to do.
   //  o If our context's heap will be deleted, nullify it.
   //  o If our context will survive the restart, put it back on a work
   //    queue so that it can be serviced after the restart is over.
   //
   if(level == RestartNone) return;

   Restart::Release(ctx_);
   if(ctx_ == nullptr) return;

   Singleton<PayloadInvokerPool>::Extant()->Requeue(*ctx_.release());
}
}
