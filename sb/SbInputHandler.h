//==============================================================================
//
//  SbInputHandler.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBINPUTHANDLER_H_INCLUDED
#define SBINPUTHANDLER_H_INCLUDED

#include "InputHandler.h"
#include "NwTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Input handler for messages that contain a SessionBase header.  Each
//  well-known port that receives or sends intra-network messages with a
//  SessionBase header should define a subclass.
//
class SbInputHandler : public InputHandler
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~SbInputHandler();

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because this is class is virtual.
   //
   explicit SbInputHandler(IpPort* port);

   //  Overridden to allocate an SbIpBuffer for an incoming internal message
   //  that already has a MsgHeader.  Supports unbundling (e.g. for messages
   //  arriving over TCP).
   //
   virtual IpBuffer* AllocBuff(const byte_t* source,
      MsgSize size, byte_t*& dest, MsgSize& rcvd) const override;

   //  Overridden to queue the message for an invoker thread.  Invoked by
   //  a subclass implementation of this function after it has filled in
   //  the MsgHeader.  Here is an outline of how a subclass does this:
   //
   //    auto header  = buffer->HeaderPtr();
   //    auto payload = buffer->PayloadPtr();
   //
   //    //  Construct the message header.  The SbIpBuffer constructor
   //    //  has already initialized all fields to their default values.
   //    //
   //    header->length = size;  // length of the payload
   //    header->protocol = ...  // probably hard-coded
   //    header->signal = ...    // from parsing the payload
   //    header->priority = ...  // based on the signal
   //    header->initial = ...   // based on the signal
   //    header->final = ...     // based on the signal
   //    header->rxAddr = ...    // .fid is probably hard-coded
   //    //  For a subsequent message, .rxaddr is set by finding an
   //    //  identifier in the payload and then using a database to
   //    //  map the identifier (e.g. a userid) to a LocalAddress
   //
   //    //  Invoke the base class to queue the message.  The base
   //    //  class assumes that the message has a valid MsgHeader.
   //    //  The origina message didn't, but now it does.
   //    //
   //    SbInputHandler::ReceiveBuff(buffer, faction);
   //
   virtual void ReceiveBuff
      (MsgSize size, IpBufferPtr& buff, Faction faction) const override;
};
}
#endif
