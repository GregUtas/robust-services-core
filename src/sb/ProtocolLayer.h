//==============================================================================
//
//  ProtocolLayer.h
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
#ifndef PROTOCOLLAYER_H_INCLUDED
#define PROTOCOLLAYER_H_INCLUDED

#include "Pooled.h"
#include "Message.h"
#include "NwTypes.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Virtual base class for each layer in a protocol stack.  An incoming message
//  creates a stack from bottom to top, and an outgoing message creates it from
//  top to bottom.  Each stack has a MsgPort at its base, with one or more PSMs
//  (ProtocolSMs) above.
//
class ProtocolLayer : public NodeBase::Pooled
{
public:
   //  Invoked to pass MSG down the stack.
   //
   bool SendToLower(Message& msg);

   //  Returns the root SSM, if any, in the layer's context.
   //
   RootServiceSM* RootSsm() const;

   //  Returns the MsgPort at the bottom of the stack.
   //
   virtual MsgPort* Port() const = 0;

   //  Returns the PSM at the top of the stack.
   //
   virtual ProtocolSM* UppermostPsm() const = 0;

   //  Returns the layer's context.
   //
   Context* GetContext() const { return ctx_; }

   //  Returns true if this layer is at the top of the stack.
   //
   bool IsUppermost() const { return upper_ == nullptr; }

   //  Returns true if this layer is at the bottom of the stack.
   //
   bool IsLowermost() const { return lower_ == nullptr; }

   //  Returns the layer above this one.
   //
   ProtocolLayer* Upper() const { return upper_; }

   //  Returns the layer below this one.
   //
   ProtocolLayer* Lower() const { return lower_; }

   //  Creates a protocol stack without sending a message.  The purpose
   //  is to obtain a MsgPort whose local address can be saved in a user
   //  profile so that messages from the user can be routed to this PSM.
   //  Use MsgPort.LocAddr().SbAddr() to obtain this local address.  Note
   //  that MsgPort.LocAddr() will have a nil IP address until the port
   //  sends or receives a message.
   //
   MsgPort* EnsurePort();

   //  Returns the layer's factory.
   //
   virtual FactoryId GetFactory() const = 0;

   //  Returns the route over which an outgoing message should be sent.
   //
   virtual Message::Route Route() const = 0;

   //  Allocates an application socket when sending an initial message.
   //  The default version returns nullptr and may be overridden by a
   //  PSM whose application requires a dedicated socket (e.g. for TCP).
   //  Alternatively, the PSM can create the socket directly, before it
   //  sends an initial message.  If a socket is allocated, it must be
   //  registered with the PSM's peer GlobalAddress.
   //
   virtual NetworkBase::SysTcpSocket* CreateAppSocket();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates the first layer in a stack that will run in CTX.  If CTX is
   //  nullptr, the running context is used.  Protected because this class
   //  is virtual.
   //
   explicit ProtocolLayer(Context* ctx = nullptr);

   //  Creates a subsequent layer.  ADJ is the layer above if UPPER is set,
   //  else it is the layer below.  Protected because this class is virtual.
   //
   ProtocolLayer(ProtocolLayer& adj, bool upper);

   //  Virtual to allow subclassing.  Private to restrict deletion.
   //
   virtual ~ProtocolLayer();

   //  Invoked to pass MSG up the stack.  Returns any event that should be
   //  passed to the root SSM.
   //
   Event* SendToUpper(Message& msg);

   //  Invoked on a layer when an adjacent layer is deleted.  The default
   //  version clears upper_ or lower_ as appropriate and must be invoked
   //  by any override.
   //
   virtual void AdjacentDeleted(bool upper);
private:
   //  Creates the layer above for an incoming initial message.  The default
   //  version returns nullptr and must be overridden by a layer that is not
   //  at the top of a protocol stack.
   //
   virtual ProtocolLayer* AllocUpper(const Message& msg);

   //  Creates the layer below for an outgoing initial message.  If the stack
   //  is being created by ProtocolSM::CreateStack, MSG will be nullptr.  The
   //  default version returns nullptr and must be overridden by a layer that
   //  is not at the bottom of a stack.
   //
   virtual ProtocolLayer* AllocLower(const Message* msg);

   //  Invoked to pass MSG up the stack to this layer.  Updates EVT to any
   //  event that should be passed to the root SSM.
   //
   virtual Event* ReceiveMsg(Message& msg) = 0;

   //  Invoked to pass MSG down the stack to this layer.  Returns false on
   //  failure.
   //
   virtual bool SendMsg(Message& msg) = 0;

   //  Invoked to decapsulate MSG, which is travelling up a protocol stack
   //  and will now be passed to this layer.  The default version kills the
   //  context and must be overridden by a layer that is not at the bottom
   //  of a stack.
   //
   virtual Message* UnwrapMsg(Message& msg);

   //  Invoked to encapsulate MSG, which is travelling down a protocol stack
   //  and will now be passed to this layer.  The default version kills the
   //  context and must be overridden by a layer that is not at the top of a
   //  stack.
   //
   virtual Message* WrapMsg(Message& msg);

   //  Creates a the layer below unless it already exists.  MSG, if it exists,
   //  is the message being sent down the stack.
   //
   void EnsureLower(const Message* msg);

   //  The context to which the layer belongs.
   //
   Context* ctx_;

   //  The layer above, if any.
   //
   ProtocolLayer* upper_;

   //  The layer below, if any.
   //
   ProtocolLayer* lower_;
};
}
#endif
