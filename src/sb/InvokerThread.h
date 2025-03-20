//==============================================================================
//
//  InvokerThread.h
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
#ifndef INVOKERTHREAD_H_INCLUDED
#define INVOKERTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include <memory>
#include "Context.h"
#include "NbTypes.h"
#include "RegCell.h"
#include "SbTypes.h"
#include "SteadyTime.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  An InvokerThread calls InvokerPool::ProcessWork to dequeue and execute
//  SessionBase application work.
//
class InvokerThread : public NodeBase::Thread
{
   friend class Context;
   friend class InvokerDaemon;
   friend class InvokerPool;
   friend class NodeBase::Registry<InvokerThread>;
   friend class SbException;
public:
   //  After a transaction, Thread::RtcPercentUsed is called to see how long
   //  the invoker has run as a percentage of the run-to-completion timeout.
   //  If the result is greater than RtcYieldPercent_, the invoker yields so
   //  that it will not risk being killed for running unpreemptably too long.
   //  This value should be based on how many transactions the invoker can
   //  usually handle each time it is scheduled in.
   //
   static NodeBase::word RtcYieldPercent() { return RtcYieldPercent_; }

   //  Returns the time when the current transaction started.
   //
   NodeBase::SteadyTime::Point Time0() const { return time0_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates an invoker thread that is managed by DAEMON and runs in
   //  FACTION.  Private to restrict creation.
   //
   InvokerThread(NodeBase::Faction faction, NodeBase::Daemon* daemon);

   //  Private to restrict deletion.  Not subclassed.
   //
   ~InvokerThread();

   //  Sets the context that the thread is currently serving.  Called
   //  at the beginning of each transaction.
   //
   void SetContext(Context* ctx);

   //  Clears the context after a transaction is completed.
   //
   void ClearContext();

   //  Returns the context that the thread is currently serving.
   //
   Context* GetContext() const { return ctx_.get(); }

   //  Returns the offset to iid_.
   //
   static ptrdiff_t CellDiff2();

   //  Overridden to return a name for the thread.
   //
   NodeBase::c_string AbbrName() const override;

   //  Overridden to deny blocking by the last unblocked invoker and to track
   //  the currently running invoker.
   //
   bool BlockingAllowed
      (NodeBase::BlockingReason why, NodeBase::fn_name_arg func) override;

   //  Overridden to support the tracing of individual contexts.
   //
   NodeBase::TraceStatus CalcStatus(bool dynamic) const override;

   //  Overridden to dequeue work from the appropriate invoker pool and
   //  process it.
   //
   void Enter() override;

   //  Overridden to log and delete the objects involved in a serious
   //  error before reentering the thread.
   //
   bool Recover() override;

   //  Overridden to track the currently running invoker.
   //
   void ScheduledIn(NodeBase::fn_name_arg func) override;

   //  Overridden to handle any context assigned to the context.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  The thread's identifier in its InvokerPool.
   //
   NodeBase::RegCell iid_;

   //  The pool to which the thread belongs.
   //
   InvokerPool* pool_;

   //  The context that the invoker is currently serving.
   //
   std::unique_ptr<Context> ctx_;

   //  Remembers ctx_'s context message before a blocking operation and
   //  restores it afterwards.
   //
   Message* msg_;

   //  The number of transactions handled before yielding.
   //
   size_t trans_;

   //  The time when the current transaction began.
   //
   NodeBase::SteadyTime::Point time0_;

   //  Percentage of run-to-completion timeout that must remain for invoker
   //  to begin another transaction instead of yielding.
   //
   static NodeBase::word RtcYieldPercent_;

   //  The invoker that is currently running.
   //
   static const InvokerThread* RunningInvoker_;
};
}
#endif
