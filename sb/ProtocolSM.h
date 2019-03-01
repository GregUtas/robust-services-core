//==============================================================================
//
//  ProtocolSM.h
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
#ifndef PROTOCOLSM_H_INCLUDED
#define PROTOCOLSM_H_INCLUDED

#include "ProtocolLayer.h"
#include <cstddef>
#include <cstdint>
#include "Clock.h"
#include "Q1Way.h"
#include "SbTypes.h"

namespace SessionBase
{
   class PsmContext;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Each subclass implements an instance of a protocol's state machine.  Each
//  instance runs in an SsmContext (along with a RootServiceSM and other PSMs)
//  or in a PsmContext (alone).  A protocol stack (above layer 3) consists of a
//  chain of PSMs.  An incoming message travels up the stack and is unwrapped
//  (decapsulated) by each successive PSM.  An outgoing message travels down
//  the stack and is wrapped (encapsulated) by each successive PSM.  Protocol
//  stacks should only be used to support external interfaces.
//
class ProtocolSM : public ProtocolLayer
{
   friend class Message;
   friend class PsmContext;
   friend class Q1Way< ProtocolSM >;
   friend class Timer;
public:
   //  Initial state for PSMs.  If a PSM is in this state at the end of a
   //  transaction, it is destroyed.
   //
   static const StateId Idle = 0;

   //  When a PSM add to the queue of PSMs, its priority determines where it
   //  gets inserted.  The order of PSMs is important when messages are built
   //  and sent at the end of a transaction.  Most PSMs can use the default
   //  priority.  But if a PSM needs to build messages before or after other
   //  PSMs, it can override GetPriority (see below) to do so.
   //
   typedef uint8_t Priority;

   //  The default priority for queueing PSMs.
   //
   static const Priority NormalPriority = 128;

   //  Return code from ProcessIcMsg.
   //
   enum IncomingRc
   {
      EventRaised,     // pass event to root SSM
      ReceiveMessage,  // invoke SendToUpper
      DiscardMessage   // delete message and end transaction
   };

   //  Return code from ProcessOgMsg.
   //
   enum OutgoingRc
   {
      SendMessage,   // invoke SendToLower
      PurgeMessage,  // delete message and go to the next one
      SkipMessage    // go to the next message; PSM moved or deleted this one
   };

   //  Protocol errors.
   //
   enum Error
   {
      SignalUnknown,     // signal not recognized
      SignalInvalid,     // signal not valid in this state
      ParameterUnknown,  // parameter not recognized
      ParameterInvalid,  // parameter not valid for this signal
      ParameterAbsent,   // mandatory parameter not found
      Timeout            // expected message not received
   };

   //  Starts a timer that will expire in DURATION seconds.  If the timer
   //  expires, the PSM receives a message with a signal of Signal::Timeout,
   //  followed by a parameter that contains OWNER and TID, which are echoed
   //  so that the application that started the timer can identify it (using
   //  OWNER, which might be a pointer to an SSM or PSM) and also determine
   //  its purpose (using TID).  If REPEAT us set, the timer restarts when
   //  it expires and must be explicitly stopped.  Returns true if the timer
   //  was successfully started.
   //
   bool StartTimer
      (secs_t duration, Base& owner, TimerId tid, bool repeat = false);

   //  Stops the timer identified by OWNER and TID.
   //
   void StopTimer(const Base& owner, TimerId tid);

   //  Returns the first message on the PSM's received message queue.
   //  The context message (the one being processed) is placed at the
   //  head of this queue, which may also contain saved incoming
   //  messages.  It therefore behaves as an incoming message stack:
   //  the newest message is first and the oldest message is last.
   //
   Message* FirstRcvdMsg() const { return rcvdMsgq_.First(); }

   //  Returns the first message on the PSM's queue of pending outgoing
   //  messages.
   //
   Message* FirstOgMsg() const { return ogMsgq_.First(); }

   //  Returns the first message on the PSM's queue of saved outgoing
   //  messages.  This also behaves as a stack: the most recently sent
   //  message is first and the oldest sent message is last.
   //
   Message* FirstSentMsg() const { return sentMsgq_.First(); }

   //  Returns the PSM's state.
   //
   StateId GetState() const { return state_; }

   //  Returns the PSM's factory.
   //
   FactoryId GetFactory() const override { return fid_; }

   //  Returns the PSM's protocol.
   //
   ProtocolId GetProtocol() const;

   //  Returns true when SetSender and SetReceiver must be used to set the
   //  initial addresses in an outgoing message.  This the case when
   //  o this PSM's stack does not have a port;
   //  o a port exists, but it has neither received nor sent a message;
   //  o MSG, if provided, has a nil destination factory.
   //
   bool AddressesUnknown(const Message* msg) const;

   //  Returns the peer (remote) PSM's factory.
   //
   FactoryId PeerFactory() const;

   //  Adds MSG to the end of the outgoing message queue.
   //
   void EnqOgMsg(Message& msg);

   //  Deletes the PSM.  If the PSM is not in the idle state, SendFinalMsg
   //  is invoked before it is deleted.
   //
   void Destroy();

   //  Sends a message to the PSM to kill its context.
   //
   void Kill();

   //  Overridden to return the PSM's port.
   //
   MsgPort* Port() const override;

   //  Overridden to return the PSM at the top of the stack.
   //
   ProtocolSM* UppermostPsm() const override;

   //  Overridden to invoke JoinPeer on the PSM's port, which is created if
   //  it does not exist.  On success, returns the peer layer that supports
   //  the same protocol as this PSM.  Returns nullptr if no such PSM exists
   //  or PEER is an invalid port.
   //
   ProtocolLayer* JoinPeer
      (const LocalAddress& peer, GlobalAddress& peerPrevRemAddr) override;

   //  Overridden to invoke DropPeer on the PSM's port, which is created
   //  if it does not exist.
   //
   bool DropPeer(const GlobalAddress& peerPrevRemAddr) override;

   //  Overridden to enumerate all objects that the PSM owns.
   //
   void GetSubtended(Base* objects[], size_t& count) const override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to obtain a PSM from its object pool.
   //
   static void* operator new(size_t size);
protected:
   //  Creates the uppermost PSM that will send an initial message.  FID is
   //  the factory associated with the PSM.  Protected because this class is
   //  virtual.
   //
   explicit ProtocolSM(FactoryId fid);

   //  Creates a PSM to receive (send) messages that will travel up (down) the
   //  stack from ADJ.  ADJ is the layer above if UPPER is set, else it is the
   //  layer below.  FID is the factory associated with the PSM.  Protected
   //  because this class is virtual.
   //
   ProtocolSM(FactoryId fid, ProtocolLayer& adj, bool upper);

   //  Frees any messages and timers still queued on the PSM and removes it
   //  from its queue.  Protected to restrict deletion, which should be done
   //  by Destroy (see above).  Virtual to allow subclassing.
   //
   virtual ~ProtocolSM();

   //  Handles the outgoing message queue at the end of a transaction.
   //  May be overridden, but the base class version must be invoked.
   //
   virtual void EndOfTransaction();

   //  Updates the PSM's state.  May be overridden, but the base class
   //  version must be invoked.
   //
   virtual void SetState(StateId stid);

   //  Invoked to receive MSG.  Returns any event that should be passed to
   //  the root SSM.  May be overridden, but the base class version must be
   //  invoked.
   //
   Event* ReceiveMsg(Message& msg) override;

   //  Overridden to add MSG to the outgoing message queue.
   //
   bool SendMsg(Message& msg) override;

   //  Overridden to send a final message during error recovery.
   //
   void Cleanup() override;
private:
   //  Overridden to create a port that will send an initial message.  Will
   //  be overridden by a PSM that needs to create a lower layer PSM instead
   //  of a port.
   //
   ProtocolLayer* AllocLower(const Message* msg) override;

   //  Overridden to return MSG, which is assumed to be arriving from a port.
   //  Must be overridden by a PSM whose lower layer is another PSM.
   //
   Message* UnwrapMsg(Message& msg) override;

   //  Implements the incoming side of the PSM's state machine.  The PSM can
   //  set an event (EVENT) for the root SSM (usually AnalyzeMsgEvent; rarely,
   //  a protocol error event defined by the PSM) and return EventForRoot, in
   //  which case the SSM receives the event immediately.  Other choices are
   //  to return either ReceiveMessage, which passes MSG up the protocol stack,
   //  leaves EVT as nullptr, the return value is interpreted as follows:
   //  or DiscardMessage, which deletes MSG and end the transaction.
   //
   virtual IncomingRc ProcessIcMsg(Message& msg, Event*& event) = 0;

   //  Invoked just before ProcessOgMsg is invoked on each message on the
   //  outgoing message queue.  A PSM might implement this function to build
   //  messages that it wants to send or bundle messages that are already on
   //  the outgoing message queue.
   //
   virtual void PrepareOgMsgq() { }

   //  Implements the outgoing side of the PSM's state machine.  The return
   //  value indicates what to do with MSG.
   //
   virtual OutgoingRc ProcessOgMsg(Message& msg) = 0;

   //  Returns the PSM's priority in the SSM's PSM queue.  The higher the
   //  priority, the closer to the front of the queue, meaning that the
   //  PSM gets to build messages earlier.  The default version returns
   //  NormalPriority and may be overridden by a PSM that must build its
   //  messages either before or after "normal" PSMs.
   //
   virtual Priority GetPriority() const { return NormalPriority; }

   //  Invoked after MSG has been processed or sent.  Would be overridden,
   //  for example, by a PSM that has pointers into MSG.
   //
   virtual void MsgHandled(Message& msg) { }

   //  Invoked by Message::Restore when changing the context message.  Would
   //  be overridden, for example, by a PSM that has pointers into the context
   //  message.
   //
   virtual void RestoreIcMsg(Message& msg) { }

   //  Invoked by Message::Refresh when changing the location of an outgoing
   //  message's payload.  Would be overridden, for example, by a PSM that
   //  might have pointers into MSG.
   //
   virtual void RefreshMsg(Message& msg) { }

   //  Invoked when the PSM is deleted but is not yet in the idle state.
   //  The PSM must build and send a final message to its peer PSM.
   //
   virtual void SendFinalMsg() = 0;

   //  Invoked when the peer PSM's node died.  This function must inject a
   //  final message to the PSM using SendToSelf, simulating a release on
   //  behalf of its peer PSM.  The default version kills the PSM's context
   //  and must be overridden by PSMs that communicate with other nodes.
   //
   virtual void InjectFinalMsg();

   //  Performs initialization that is common to all constructors.  HENQ
   //  is set if the PSM should be queued ahead of others which have the
   //  same priority.
   //
   void Initialize(bool henq);

   //  Adds MSG to the front of the received message queue.
   //
   void HenqReceivedMsg(Message& msg);

   //  Adds MSG to the front of the outgoing message queue.  Used by
   //  Message::Relay.
   //
   void HenqOgMsg(Message& msg);

   //  Adds MSG to the head of the sent message queue.
   //
   void HenqSentMsg(Message& msg);

   //  Returns the timer (if any) that matches OWNER and TID.
   //
   Timer* FindTimer(const Base& owner, TimerId tid) const;

   //  Invokes SendFinalMsg if the PSM is not idle and has a port.
   //
   void SendFinal();

   //  The queue of received messages (actually a stack).
   //
   Q1Way< Message > rcvdMsgq_;

   //  The queue of pending outgoing messages.
   //
   Q1Way< Message > ogMsgq_;

   //  The queue of sent outgoing messages (actually a stack).
   //
   Q1Way< Message > sentMsgq_;

   //  The queue of timers running on this PSM.
   //
   Q1Way< Timer > timerq_;

   //  The factory that created this PSM.
   //
   FactoryId fid_;

   //  The PSM's state.
   //
   StateId state_;
};
}
#endif
