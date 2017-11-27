//==============================================================================
//
//  Message.h
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
#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include <cstdint>
#include "NbTypes.h"
#include "SbTypes.h"
#include "SysTypes.h"

namespace NetworkBase
{
   class SysIpL3Addr;
}

namespace SessionBase
{
   class BuffTrace;
   class GlobalAddress;
   class MsgContext;
   struct LocalAddress;
}

using namespace NetworkBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A Message wraps a physical message (byte_t*) to provide a higher level
//  of abstraction for SessionBase applications.  In a stateful context,
//  all messages enter and exit through MsgPorts.  In a stateless context,
//  a Factory sends and receives messages.
//
//  The contents of an incoming message should be considered read-only.
//
class Message : public Pooled
{
   friend class Context;
   friend class MsgContext;
   friend class ProtocolLayer;
   friend class ProtocolSM;
   friend class PsmFactory;
public:
   //  Message priorities.
   //
   typedef uint8_t Priority;

   static const Priority Ingress     = 0;  // from user starting a new session
   static const Priority Egress      = 1;  // to user receiving a new session
   static const Priority Progress    = 2;  // to an existing session
   static const Priority Immediate   = 3;  // between SSMs serving same user
   static const Priority MaxPriority = 3;

   //  Message routes.  If a protocol is only used intraprocessor (Internal), it
   //  does not require an IpPort or InputHandler.
   //
   typedef uint8_t Route;

   static const Route External = 0;  // remove MsgHeader; send over IP stack
   static const Route IpStack  = 1;  // keep MsgHeader; force over IP stack
   static const Route Internal = 2;  // keep MsgHeader; bypass IP stack

   //  Message locations (not currently used; only defined as documentation).
   //
   enum Location
   {
      NotQueued,     // being built by a factory
      ContextQ,      // has arrived at a context
      PsmIncomingQ,  // has arrived at a PSM
      PsmOutgoingQ,  // has been sent by a PSM
      PsmPendingQ    // being built by a PSM
   };

   //  Return code for message inspection.  The debug code is returned when
   //  an error is detected (index: byte index into payload; sid: signal id;
   //  pid: parameter id).
   //
   enum InspectRc
   {                   //                              debug32_t (hex)
      Ok,              // signal and parameters OK          0000 0000
      IllegalSignal,   // illegal signal found              0000  sid
      IllegalParm,     // illegal parameter found          index  pid
      IncompleteParm,  // parameter too short              index  pid
      MissingParm,     // mandatory parameter missing       0000  pid
      Overflow,        // last parameter extends past end  index  pid
      Trampled         // last parameter trampled fence    index  pid
   };

   ///////////////////////////////////////////////////////////////////////////
   //
   //  The following functions are for general use.

   //  Returns the message's protocol.
   //
   ProtocolId GetProtocol() const;

   //  Returns the message's signal.
   //
   SignalId GetSignal() const;

   //  Sets the signal for an outgoing message.
   //
   void SetSignal(SignalId sid);

   //  Sets the join flag for an outgoing message.
   //
   void SetJoin(bool join);

   //  Returns the factory associated with the message's recipient.
   //
   Factory* RxFactory() const;

   //  Returns the message header.
   //
   //  NOTE: When adding parameters to a message, it is dangerous to save
   //  ====  the pointer returned by this function in a stack variable.
   //        The reason is that the header resides in a buffer that can
   //        be *relocated* to make space for the next parameter.
   //
   MsgHeader* Header() const;

   //  Scans a message to ensure that its signal and parameters are valid.
   //  Returns Ok on success.  On failure, updates ERRVAL with debug info.
   //
   virtual InspectRc InspectMsg(debug32_t& errval) const;

   //  Invoked when an incoming message is discarded.
   //
   void InvalidDiscarded() const;

   ///////////////////////////////////////////////////////////////////////////
   //
   //  The following functions only apply to stateful contexts, where
   //  messages are queued on PSMs.

   //  Returns the PSM where the message is queued.
   //
   ProtocolSM* Psm() const { return psm_; }

   //  Increments the message's save count so that it will not be deleted.
   //  If this function is not invoked,
   //  o an incoming message is deleted at the end of its transaction, and
   //  o an outgoing message is deleted immediately after it is sent.
   //
   virtual void Save();

   //  Decrements the message's save count.  If the count drops to zero, the
   //  message is deleted unless its handled_ flag is false.
   //
   virtual void Unsave();

   //  Moves a sent (and saved) outgoing message to PSM's outgoing message
   //  queue so that it can be retransmitted.  If PSM is nullptr, it stays
   //  on the same PSM.  By specifiying a PSM other than the one where the
   //  message is queued, the message can effectively be broadcast (saved
   //  by one PSM, then retrieved by the next).
   //
   virtual bool Retrieve(ProtocolSM* psm);

   //  Moves an incoming message to OGPSM and makes it an outgoing message
   //  so that it can be relayed to another SSM's context.  If the two PSMs
   //  use different message subclasses, the incoming message must first be
   //  morphed to the subclass of the outgoing message.
   //
   virtual bool Relay(ProtocolSM& ogPsm);

   //  Sends an outgoing message back to its own PSM.
   //
   virtual bool SendToSelf();

   //  Finds the next message that is queued after this one and whose signal
   //  matches SID.
   //
   Message* FindSignal(SignalId sid) const;

   //  Returns the next message in the queue where this message resides.
   //
   Message* NextMsg() const;

   ///////////////////////////////////////////////////////////////////////////
   //
   //  In stateful contexts, only PSMs and ports normally use the following
   //  functions.  In stateless contexts, where factories send and receive
   //  messages, application software uses these functions.

   //  Returns the IP address of the message receiver.
   //
   const SysIpL3Addr& RxIpAddr() const;

   //  Returns the IP address of the message sender.
   //
   const SysIpL3Addr& TxIpAddr() const;

   //  Returns the local address of the message receiver.
   //
   const LocalAddress& RxSbAddr() const;

   //  Returns the local address of the message sender.
   //
   const LocalAddress& TxSbAddr() const;

   //  Returns the message's destination address.
   //
   GlobalAddress GetReceiver() const;

   //  Returns the message's source address.
   //
   GlobalAddress GetSender() const;

   //  Returns the number of bytes in the payload and updates BYTES to
   //  reference that start of the payload.  BYTES is set to nullptr
   //  when returning 0.  The payload excludes the message header.
   //
   MsgSize Payload(byte_t*& bytes) const;

   //  Sets the protocol for an outgoing message.
   //
   void SetProtocol(ProtocolId prid);

   //  Sets the priority for an outgoing message.
   //
   void SetPriority(Priority prio);

   //  Sets the destination address for an outgoing message.
   //
   virtual void SetReceiver(const GlobalAddress& receiver);

   //  Sets the source address for an outgoing message.
   //
   virtual void SetSender(const GlobalAddress& sender);

   //  Returns the message's direction.
   //
   MsgDirection Dir() const;

   //  Frees the IP buffer and removes the message from its queue.  The
   //  framework deletes messages when appropriate, so an application
   //  only needs to delete an outgoing message to cancel it prior to
   //  invocation of its PSM's ProcessOgMsg function.  Virtual to allow
   //  subclassing.
   //
   virtual ~Message();

   //  Sends the message to its destination.  ROUTE specifies whether the
   //  message header should be dropped and whether an intraprocessor
   //  message should bypass the IP stack.  May be overridden, but the
   //  base class version must be invoked.
   //
   //  NOTE: In a protocol stack, use SendToLower instead of Send.  Note
   //  ====  also that Send deletes the message unless Save() was invoked
   //        or the message was moved to an intraprocessor destination.
   //        An application must therefore not reference a message after
   //        sending it, unless it knows that the message was saved.
   //
   virtual bool Send(Route route);

   //  Makes a saved incoming message the context message.
   //
   virtual bool Restore();

   //  Returns the message's buffer.
   //
   const SbIpBuffer* Buffer() const { return buff_.get(); }

   //  Returns a string for displaying PRIO.
   //
   static const char* strPriority(Priority p);

   //  Records BT as the trace record that captured this message.
   //
   void SetTrace(const BuffTrace* bt) { bt_ = bt; }

   //  Overridden to enumerate all objects that the message owns.
   //
   virtual void GetSubtended(Base* objects[], size_t& count) const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to obtain a message from its object pool.
   //
   static void* operator new(size_t size);
protected:
   //  Creates an incoming message to wrap BUFF.  Protected because this
   //  class is virtual.
   //
   explicit Message(SbIpBufferPtr& buff);

   //  Creates an outgoing message, for which an SbIpBuffer is allocated,
   //  and queues it on PSM (which is nullptr if running in a stateless
   //  context).  SIZE is the anticipated size of the payload (the message
   //  parameters, excluding the header) and is used when allocating the
   //  physical message buffer.  The message is not limited to SIZE, but
   //  real-time performance improves if the buffer need not be expanded.
   //  Protected because this class is virtual.
   //
   Message(ProtocolSM* psm, MsgSize size);

   //  Replaces the current buffer, which contains the payload, with BUFF.
   //
   void Replace(SbIpBufferPtr& buff);

   //  Invoked if the message's payload is moved to a new location.  This
   //  allows the message to regenerate any pointers into its payload.
   //  The default version invokes RefreshMsg on the message's PSM, if any.
   //
   virtual void Refresh();

   //  Converts an incoming message to an outgoing message or vice versa.
   //  May be overridden, but the base class version must be invoked.
   //
   virtual void ChangeDir(MsgDirection nextDir);

   //  Invoked on the context incoming message when its processing is finished
   //  (at the end of a transaction, or when another message is about to become
   //  the incoming message).  Invoked on an outgoing message after it is sent.
   //  Destroys the message unless it has been saved or RETAIN is set.  May be
   //  overridden, but the base class version must be invoked.
   //
   virtual void Handled(bool retain);

   //  Invoked when Send wants to log an error.  Also invokes Handled.
   //
   virtual bool SendFailure(debug64_t errval, debug32_t offset);

   //  Returns a non-const pointer to the message's buffer.
   //
   SbIpBuffer* WriteBuffer() const { return buff_.get(); }
private:
   //  Ensures that the message is no longer seen as the context message.
   //
   virtual void ClearContext() const;

   //  Adds the message to the end of WHICHQ.
   //
   void Enqueue(Q1Way< Message >& whichq);

   //  Adds the message to the beginning of WHICHQ.
   //
   void Henqueue(Q1Way< Message >& whichq);

   //  Removes the message from its current queue.
   //
   void Exqueue();

   //  Sets the receiver's address after it is allocated by an incoming
   //  initial message.
   //
   void SetRxAddr(const LocalAddress& rxaddr);

   //  Sets the PSM on which the message is queued.
   //
   void SetPsm(ProtocolSM* psm);

   //  Captures a message when its PSM is being traced.  ROUTE indicates
   //  how the message was sent.
   //
   void Capture(Route route) const;

   //  The buffer that carries the message's payload.
   //
   SbIpBufferPtr buff_;

   //  The trace record, if any, that captured the message.
   //
   const BuffTrace* bt_;

   //  Whether the message has been handled.  It is set
   //  o for an incoming message, when the transaction ends or Restore
   //    is invoked;
   //  o for an outgoing message, when the message has been sent.
   //
   bool handled_;

   //  The net number of requests to save the message.
   //
   uint8_t saves_;

   //  The PSM that owns the message (may be nullptr).
   //
   ProtocolSM* psm_;

   //  The queue where the message resides (may be nullptr).
   //
   Q1Way< Message >* whichq_;
};
}
#endif
