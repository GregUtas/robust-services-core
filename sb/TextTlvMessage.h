//==============================================================================
//
//  TextTlvMessage.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TEXTTLVMESSAGE_H_INCLUDED
#define TEXTTLVMESSAGE_H_INCLUDED

#include "TlvMessage.h"
#include "NbTypes.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Message subclass for text-based protocols that are converted to TLV format
//  just after entering the system and reconverted to text format just before
//  being sent.
//
class TextTlvMessage : public TlvMessage
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~TextTlvMessage();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates an incoming message.  Protected because this class is virtual.
   //  TEXT contains the incoming text message, which must be preceded by a
   //  valid MsgHeader.  When Parse is invoked, it parses TEXT to build the
   //  TLV version of the message in a buffer that replaces TEXT.
   //
   explicit TextTlvMessage(SbIpBufferPtr& text);

   //  Creates an outgoing message.  Protected because this class is virtual.
   //
   TextTlvMessage(ProtocolSM* psm, MsgSize size);

   //  Invokes Parse and replaces the original text message with the TLV
   //  message created by Parse.  Returns false if Parse returned nullptr.
   //  Invoked by an implementation of ProtocolSM::ProcessIcMsg.
   //
   bool Receive();

   //  Overridden to invoke Build before sending the message.
   //
   virtual bool Send(Route route) override;
private:
   //  Converts an incoming text message to TLV format.  Returns the TLV
   //  version of the message.  Invoked by Receive.
   //
   virtual SbIpBufferPtr Parse() = 0;

   //  Converts an outgoing TLV message to text format.  Returns the text
   //  version of the message, which must preserve the SessionBase header.
   //  Trace tools need the header, but it is dropped when the message is
   //  sent externally.  Invoked by Send.
   //
   virtual SbIpBufferPtr Build() = 0;

   //  Set if the message is currently in text format.
   //
   bool text_;
};
}
#endif
