//==============================================================================
//
//  Factory.h
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
#ifndef FACTORY_H_INCLUDED
#define FACTORY_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include "NbTypes.h"
#include "RegCell.h"
#include "SbTypes.h"
#include "Signal.h"
#include "SysTypes.h"

namespace NodeBase
{
   template< typename T > class Q1Way;
}

namespace SessionBase
{
   class FactoryStats;
   class TransTrace;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A Factory creates the messages, PSMs, and/or SSMs that support a service
//  or protocol.  This is a virtual base class.  Applications subclass from
//  MsgFactory, PsmFactory, or SsmFactory.
//
class Factory : public NodeBase::Protected
{
   friend class InvokerPool;
   friend class NodeBase::Registry< Factory >;
public:
   //  Allows "Id" to refer to a factory identifier in this class hierarchy.
   //
   typedef FactoryId Id;

   //> Highest valid factory identifier.
   //
   static const Id MaxId = UINT8_MAX;

   //  Outcomes when receiving a message.
   //
   enum Rc
   {
      InputOk,              // message received successfully
      MsgHeaderMissing,     // message does not have a valid message header
      MsgPriorityInvalid,   // message priority out of range
      FactoryNotFound,      // message was addressed to an unknown factory
      PortNotFound,         // message was addressed to an unknown port
      ContextNotFound,      // context could not be found
      FactoryNotReceiving,  // factory did not implement ReceiveMsg
      CtxAllocFailed,       // failed to create context
      MsgAllocFailed,       // failed to create message for context
      PortAllocFailed,      // failed to create PSM for context
      ContextCorrupt        // message rejected because context was corrupt
   };

   //  Returns the factory's identifier.
   //
   Id Fid() const { return Id(fid_.GetId()); }

   //  Returns the protocol that the factory supports.
   //
   ProtocolId GetProtocol() const { return prid_; }

   //  Returns true if the factory can legally receive SID, which is assumed
   //  to be in its protocol.
   //
   bool IsLegalIcSignal(SignalId sid) const;

   //  Returns true if the factory can legally send SID, which is assumed to
   //  be in its protocol.
   //
   bool IsLegalOgSignal(SignalId sid) const;

   //  Returns the type of context that the factory uses.
   //
   ContextType GetType() const { return type_; }

   //  Returns the factory's scheduler faction.  The default version returns
   //  PayloadFaction and must be overridden by factories that wish to run
   //  in a different faction.
   //
   NodeBase::Faction GetFaction() const { return faction_; }

   //  Returns a string that identifies the factory.
   //
   NodeBase::c_string Name() const { return name_; }

   //  Creates a subclass of CliText so that the factory can be specified
   //  with a string.  The default version returns nullptr and must be
   //  overridden by factories that support CLI commands.
   //
   virtual NodeBase::CliText* CreateText() const;

   //  Allocates an outgoing message that a test tool will inject after
   //  setting the signal to SID.  The default version returns nullptr
   //  and must be overridden by factories that support InjectCommand.
   //
   virtual Message* AllocOgMsg(SignalId sid) const;

   //  Sends MSG on behalf of InjectCommand.  The message's header must be
   //  filled in so that it will arrive at the correct destination.  Returns
   //  true if MSG was successfully sent.  The default version returns false
   //  and must be overridden by factories that support InjectCommand.
   //
   virtual bool InjectMsg(Message& msg) const;

   //  Allocates and returns an outgoing message to rewrap BUFF.  The default
   //  version returns nullptr and must be overridden by factories that support
   //  VerifyCommand.
   //
   virtual Message* ReallocOgMsg(SbIpBufferPtr& buff) const;

   //  Invoked when the first ingress MSG is received.  Updates PRIO if MSG
   //  should go on a higher priority work queue and/or returns true if MSG
   //  should be placed at the front of that queue.  The default version
   //  simply returns false and leaves PRIO at INGRESS.
   //
   virtual bool ScreenFirstMsg(const Message& msg, MsgPriority& prio) const;

   //  Invoked when a context on the ingress work queue receives a subsequent
   //  message.  MSGQ provides access to the context's message queue.  If the
   //  subsequent message was a retransmission of the original request, this
   //  allows the factory to discard it.  If the subsequent message cancelled
   //  the original request, the factory can return false, in which case the
   //  entire context is deleted (as the work is now a noop).  The default
   //  implementation simply returns true and may be overridden as required.
   //
   virtual bool ScreenIcMsgs(NodeBase::Q1Way< Message >& msgq);

   //  Generates statistics when a message associated with the factory is
   //  received or sent.  INCOMING is true for an incoming message, INTER
   //  is true for an interprocessor message, and SIZE is the message's
   //  length.
   //
   void RecordMsg(bool incoming, bool inter, size_t size) const;

   //  Returns the number of messages deleted without being processed during
   //  the current statistics interval.
   //
   size_t DiscardedMessageCount() const;

   //  Returns the number of contexts deleted without being processed during
   //  the current statistics interval.
   //
   size_t DiscardedContextCount() const;

   //  Displays statistics.  May be overridden to include factory-specific
   //  statistics, but the base class version must be invoked.
   //
   virtual void DisplayStats
      (std::ostream& stream, const NodeBase::Flags& options) const;

   //  Returns the offset to fid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the corresponding member variables and adds the factory to
   //  FactoryRegistry.  Protected because this class is virtual.
   //
   Factory(Id fid, ContextType type, ProtocolId prid, NodeBase::c_string name);

   //  Removes the factory from FactoryRegistry.  Protected because
   //  subclasses should be singletons.
   //
   virtual ~Factory();

   //  Adds SID, which is assumed to be in the factory's protocol, as a
   //  legal incoming signal.  Invoked by a subclass constructor.
   //
   void AddIncomingSignal(SignalId sid);

   //  Adds SID, which is assumed to be in the factory's protocol, as a
   //  legal outgoing signal.  Invoked by a subclass constructor.
   //
   void AddOutgoingSignal(SignalId sid);

   //  Sets the factory's scheduler faction.  By default, a factory runs
   //  in PayloadFaction, so this is invoked by a constructor for a factory
   //  that needs to run in a different faction.
   //
   void SetFaction(NodeBase::Faction faction) { faction_ = faction; }

   //  Creates the type of context that the factory uses.
   //
   virtual Context* AllocContext() const;

   //  Increments the number of contexts created by the factory.
   //
   void IncrContexts() const;

   //  Generates statistics when a message or context on the ingress queue is
   //  deleted before being processed.  Set CONTEXT if a context was deleted.
   //
   void RecordDeletion(bool context) const;
private:
   //  Deleted to prohibit copying.
   //
   Factory(const Factory& that) = delete;
   Factory& operator=(const Factory& that) = delete;

   //  Allocates an incoming message to wrap BUFF and returns it in MSG.
   //  Returning nullptr indicates that BUFF should be discarded, either
   //  because it is invalid or because message allocation failed.  Must
   //  be overridden by subclasses.
   //
   virtual Message* AllocIcMsg(SbIpBufferPtr& buff) const;

   //  Queues MSG on the appropriate CTX, which must be created if MSG is
   //  an initial message.  TT is nullptr unless atIoLevel is true and the
   //  transaction is being traced.  Protected to restrict usage.
   //
   virtual Rc ReceiveMsg
      (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx);

   //  The factory's identifier.
   //
   NodeBase::RegCell fid_;

   //  The type of context that the factory uses.
   //
   const ContextType type_;

   //  The scheduler faction in which the factory runs.
   //
   NodeBase::Faction faction_;

   //  The protocol that the factory supports.
   //
   const ProtocolId prid_;

   //  The factory's name.
   //
   NodeBase::fixed_string name_;

   //  The signals that are legal for the factory to receive.
   //
   bool icSignals_[Signal::MaxId + 1];

   //  The signals that are legal for the factory to send.
   //
   bool ogSignals_[Signal::MaxId + 1];

   //  The factory's statistics.
   //
   std::unique_ptr< FactoryStats > stats_;
};
}
#endif
