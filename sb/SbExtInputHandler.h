//==============================================================================
//
//  SbExtInputHandler.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBEXTINPUTHANDLER_H_INCLUDED
#define SBEXTINPUTHANDLER_H_INCLUDED

#include "SbInputHandler.h"
#include "NwTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Input handler for external protocols received by SessionBase applications.
//  Subclasses implement ReceiveBuff to construct a MsgHeader for the incoming
//  message and then invoke SbExtInputHandler::ReceiveBuff (inherited from our
//  base class) to queue the message for processing.  Message unbundling (e.g.
//  of messages arriving over TCP) is not supported, as unbundling procedures
//  are protocol specific.  Where a bundled message ends is usually determined
//  either by a header that contains a message length, or by an end-of-message
//  marker (e.g. <CR><LF> in a text-oriented protocol).
//
class SbExtInputHandler : public SbInputHandler
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~SbExtInputHandler();

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Registers the input handler against PORT.  Protected because this
   //  class is virtual.
   //
   explicit SbExtInputHandler(IpPort* port);

   //  Overridden to allocate an SbIpBuffer for an incoming external message
   //  whose MsgHeader must be built.
   //
   virtual IpBuffer* AllocBuff(const byte_t* source,
      MsgSize size, byte_t*& dest, MsgSize& rcvd) const override;
};
}
#endif
