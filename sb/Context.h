//==============================================================================
//
//  Context.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include <memory>
#include <string>
#include "Factory.h"
#include "NbTypes.h"
#include "Q1Way.h"
#include "Q2Link.h"
#include "SbTypes.h"
#include "SysTypes.h"
#include "TimePoint.h"

namespace SessionBase
{
   class InvokerPool;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A Context is a facade for a group of SessionBase objects that implement
//  a session.  It provides a uniform interface between I/O and application
//  level processing.  Each incoming message is queued on a context, which is
//  in turn placed on an invoker pool's work queue.  When an invoker thread
//  eventually tells the context to process the message, the context passes
//  the message to an appropriate object for processing.
//
class Context : public NodeBase::Pooled
{
   friend std::unique_ptr< Context >::deleter_type;
   friend class InvokerPool;
   friend class InvokerThread;
   friend class MsgFactory;
   friend class MsgPort;
   friend class ProtocolSM;
   friend class PsmFactory;
   friend class NodeBase::Q2Way< Context >;
   friend class SsmFactory;
public:
   //  Returns the incoming message that is currently being processed.
   //
   static Message* ContextMsg() { return ContextMsg_; }

   //  Returns the PSM that owns the context message.
   //
   static ProtocolSM* ContextPsm();

   //  Returns the root SSM in the running context.
   //
   //  NOTE: A PSM that runs in the test environment provided by InjectCommand
   //  ====  and VerifyCommand must not carelessly cast the root SSM to the
   //        subclass that it normally expects.  This is because, in the test
   //        environment, the root SSM is a test SSM.  Ideally, a PSM should
   //        not assume specific SSM subclasses.  But where necessary, the
   //        following can be used to screen out the test SSM before a cast:
   //
   //        if(Context::ContextRoot()->Sid() != TestServiceId)...
   //
   static RootServiceSM* ContextRoot();

   //  Returns the context that is currently running.
   //
   static Context* RunningContext();

   //  Returns true if the running context is being traced.  Returns false
   //  if there is no running context.  Updates TRANS if true is returned,
   //  although it can still be nullptr.
   //
   static bool RunningContextTraced(TransTrace*& trans);

   //  Returns the type of context.
   //
   virtual ContextType Type() const = 0;

   //  Returns the context's root SSM, if any.
   //
   virtual RootServiceSM* RootSsm() const { return nullptr; }

   //  Returns the first PSM in the PSM queue.
   //
   virtual ProtocolSM* FirstPsm() const { return nullptr; }

   //  Returns the next PSM in the PSM queue.
   //
   virtual void NextPsm(ProtocolSM*& psm) const { psm = nullptr; }

   //  Returns the next port in the port queue.
   //
   virtual void NextPort(MsgPort*& port) const { port = nullptr; }

   //  Logs and deletes the objects in the current context when a fatal
   //  error occurs.  ERRSTR/ERRVAL and OFFSET are included in the log.
   //
   static void Kill(const std::string& errstr, NodeBase::debug64_t offset);
   static void Kill(NodeBase::debug64_t errval, NodeBase::debug64_t offset);

   //  Logs the objects in the context for debugging purposes.  FUNC,
   //  ERRSTR, and OFFSET are passed to Debug::SwLog.
   //
   static void Dump(NodeBase::fn_name_arg func,
      const std::string& errstr, NodeBase::debug64_t offset);

   //  Records a message with signal SID, in protocol PRID, travelling in
   //  direction DIR.  This allows the message history to be included in
   //  logs to aid in debugging.
   //
   void TraceMsg(ProtocolId prid, SignalId sid, NodeBase::MsgDirection dir);

   //  Returns a string containing the message history.
   //
   std::string strTrace() const;

   //  Returns true if the context is being traced by trace tools.  Updates
   //  TRANS (which can be nullptr) when true is returned.  Note that TraceMsg
   //  (above) is always used, whereas a trace tool (one with a FlagId in the
   //  Tool class) generally incurs far more overhead.
   //
   bool TraceOn();
   bool TraceOn(TransTrace*& trans);

   //  FLAG specifies if the context should be traced.
   //
   void SetTrace(bool flag) { traceOn_ = flag; }

   //  Sets the context message to MSG.
   //
   static void SetContextMsg(Message* msg);

   //  Called by ProtocolSM::StopTimer to stop a timer that already expired
   //  and whose timeout message is probably queued on the context.  Returns
   //  false if no such timeout message was found.
   //
   bool StopTimer(const Base& owner, TimerId tid);

   //  Corrupts the message queue for testing purposes.
   //
   void Corrupt();

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to enumerate all objects that the context owns.
   //
   void GetSubtended(std::vector< Base* >& objects) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a context that will run in FACTION.  Protected because this
   //  class is virtual.
   //
   explicit Context(NodeBase::Faction faction);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~Context();

   //  Invoked at the end of a transaction.
   //
   virtual void EndOfTransaction() { }

   //  Returns the trace record if the context's current transaction is
   //  being traced.
   //
   TransTrace* GetTrans() const { return trans_; }

   //  Overridden to disassociate the context from any work queue and/or
   //  invoker thread during error recovery.
   //
   void Cleanup() override;
private:
   //  Overridden to obtain a context from its object pool.
   //
   static void* operator new(size_t size);

   //  Context states.
   //
   enum State
   {
      Dormant,  // has no messages to process
      Ready,    // waiting to process its messages (on a work queue)
      Paused,   // processing messages but its invoker has paused
      Running   // processing messages (invoker running)
   };

   //  The contents of each entry in the message history trace buffer.
   //
   struct MessageEntry
   {
      NodeBase::MsgDirection dir : 2;  // the message's direction
      ProtocolId prid : 7;             // the message's protocol
      SignalId sid : 7;                // the message's signal
   };

   //  The size of the message history trace buffer.
   //
   static const size_t TraceSize = 12;

   //  Adds PSM to the context, after any PSMs of higher or equal priority.
   //
   virtual void EnqPsm(ProtocolSM& psm);

   //  Adds PSM to the context, after any PSMs of higher priority.
   //
   virtual void HenqPsm(ProtocolSM& psm);

   //  Removes PSM from the context.
   //
   virtual void ExqPsm(ProtocolSM& psm);

   //  Adds PORT to the context.
   //
   virtual void EnqPort(MsgPort& port);

   //  Removes PORT from the context.
   //
   virtual void ExqPort(MsgPort& port);

   //  Invoked by ProcessWork (see below) to process a single incoming MSG.
   //
   virtual void ProcessIcMsg(Message& msg) = 0;

   //  Invoked to determine if the context should be deleted.
   //
   virtual bool IsIdle() const { return true; }

   //  Adds MSG to the context's message queue.  Returns false if the message
   //  was discarded.
   //
   bool EnqMsg(Message& msg);

   //  Returns the number of messages in the context's message queues.  The
   //  flags specify whether to include priority and/or standard messages.
   //
   size_t MsgCount(bool priority, bool standard) const;

   //  Adds the context to the work queue associated with PRIO.  HENQ is set
   //  if the context should be placed at the front of that queue.
   //
   void Enqueue(NodeBase::Q2Way< Context >& whichq,
      MsgPriority prio, bool henq);

   //  Removes the context from its work queue.
   //
   void Exqueue();

   //  The context has been dequeued from its work queue and can now process
   //  its messages.  INV is the thread running the transaction.
   //
   void ProcessWork(InvokerThread* inv);

   //  Called by ProcessWork to handle the first message in MSGQ.  INV is
   //  the thread running the transaction.  Returns true if the context still
   //  exists after processing the message, and false if it was deleted.
   //
   bool ProcessMsg(NodeBase::Q1Way< Message >& msgq, const InvokerThread* inv);

   //  Invoked to record a transaction.  MSG is the message being processed,
   //  and START is when the transaction started.
   //
   void CaptureTask(const Message& msg, const InvokerThread* inv);

   //  Sets the context's state.
   //
   void SetState(State state);

   //  Dumps the context's objects.
   //
   void Dump() const;

   //  The invoker pool work queue where the context resides.
   //
   NodeBase::Q2Way< Context >* whichq_;

   //  The queue link for the invoker pool work queue.
   //
   NodeBase::Q2Link link_;

   //  The queue of incoming messages with immediate priority.
   //
   NodeBase::Q1Way< Message > priMsgq_;

   //  The queue of incoming messages of non-immediate priority.
   //
   NodeBase::Q1Way< Message > stdMsgq_;

   //  The time when the context was enqueued.
   //
   NodeBase::TimePoint enqTime_;

   //  The invoker pool that is managing this context.
   //
   InvokerPool* pool_;

   //  The invoker that is running this context, if state_ is ready/paused.
   //
   InvokerThread* thread_;

   //  The context's scheduler faction.
   //
   NodeBase::Faction faction_;

   //  The context's processing state.
   //
   State state_;

   //  The priority of the work queue on which the context is located.
   //
   MsgPriority prio_;

   //  Set if the context is being traced.
   //
   bool traceOn_;

   //  The TransTrace record, if any, created for the current transaction.
   //
   TransTrace* trans_;

   //  The current index into the trace buffer, which wraps around.
   //
   size_t buffIndex_;

   //  The trace buffer.
   //
   MessageEntry trace_[TraceSize];

   //  A template for initializing a SignalEntry.
   //
   static const MessageEntry NilMessageEntry;

   //  The incoming message that is currently being processed.
   //
   static Message* ContextMsg_;
};
}
#endif
