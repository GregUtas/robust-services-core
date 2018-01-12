//==============================================================================
//
//  InvokerThread.h
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
#ifndef INVOKERTHREAD_H_INCLUDED
#define INVOKERTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include <memory>
#include "Clock.h"
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

namespace SessionBase
{
   class Context;
   class InvokerPool;
   class Message;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  An InvokerThread calss InvokerPool::ProcessWork to dequeue and execute
//  SessionBase application work.
//
class InvokerThread : public Thread
{
   friend class Context;
   friend class InvokerPool;
   friend class Registry< InvokerThread >;
   friend class SbException;
public:
   //  After a transaction, Thread::RtcPercentUsed is called to see how long
   //  the invoker has run as a percentage of the run-to-completion timeout.
   //  If the result is greater than RtcYieldPercent_, the invoker yields so
   //  that it will not risk being killed for running unpreemptably too long.
   //  This value should be based on how many transactions the invoker can
   //  usually handle each time it is scheduled in.
   //
   static word RtcYieldPercent() { return RtcYieldPercent_; }

   //  Returns the tick time when the current transaction started.
   //
   ticks_t Ticks0() const { return ticks0_; }

   //  Returns the offset to iid_.
   //
   static ptrdiff_t CellDiff2();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Used by InvokerPool to create an invoker that runs in FACTION.
   //  Private to restrict creation.
   //
   explicit InvokerThread(Faction faction);

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

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to dequeue work from the appropriate invoker pool and
   //  process it.
   //
   virtual void Enter() override;

   //  Overridden to deny blocking by the last unblocked invoker and to track
   //  the currently running invoker.
   //
   virtual bool BlockingAllowed
      (BlockingReason why, fn_name_arg func) override;

   //  Overridden to track the currently running invoker.
   //
   virtual void ScheduledIn(fn_name_arg func) override;

   //  Overridden to support the tracing of individual contexts.
   //
   virtual TraceStatus CalcStatus(bool dynamic) const override;

   //  Overridden to log and delete the objects involved in a serious
   //  error before reentering the thread.
   //
   virtual RecoveryAction Recover() override;

   //  The thread's identifier in its InvokerPool.
   //
   RegCell iid_;

   //  The pool to which the thread belongs.
   //
   InvokerPool* pool_;

   //  The context that the invoker is currently serving.
   //
   std::unique_ptr< Context > ctx_;

   //  Remembers ctx_'s context message before a blocking operation and
   //  restores it afterwards.
   //
   Message* msg_;

   //  The number of transactions handled before yielding.
   //
   size_t trans_;

   //  The time when the current transaction began.
   //
   ticks_t ticks0_;

   //  Percentage of run-to-completion timeout that must remain for invoker
   //  to begin another transaction instead of yielding.
   //
   static word RtcYieldPercent_;

   //  The invoker that is currently running.
   //
   static const InvokerThread* RunningInvoker_;
};
}
#endif
