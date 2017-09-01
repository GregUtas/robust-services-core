//==============================================================================
//
//  TextTlvMessage.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "TextTlvMessage.h"
#include <ostream>
#include <string>
#include "Context.h"
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name TextTlvMessage_ctor1 = "TextTlvMessage.ctor(i/c)";

TextTlvMessage::TextTlvMessage(SbIpBufferPtr& buff) :
   TlvMessage(buff),
   text_(true)
{
   Debug::ft(TextTlvMessage_ctor1);
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_ctor2 = "TextTlvMessage.ctor(o/g)";

TextTlvMessage::TextTlvMessage(ProtocolSM* psm, MsgSize size) :
   TlvMessage(psm, size),
   text_(false)
{
   Debug::ft(TextTlvMessage_ctor2);
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_dtor = "TextTlvMessage.dtor";

TextTlvMessage::~TextTlvMessage()
{
   Debug::ft(TextTlvMessage_dtor);
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_Build = "TextTlvMessage.Build";

SbIpBufferPtr TextTlvMessage::Build()
{
   Debug::ft(TextTlvMessage_Build);

   //  This is a pure virtual function.
   //
   Context::Kill(TextTlvMessage_Build, GetProtocol(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void TextTlvMessage::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   TlvMessage::Display(stream, prefix, options);

   stream << prefix << "text : " << text_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_Parse = "TextTlvMessage.Parse";

SbIpBufferPtr TextTlvMessage::Parse()
{
   Debug::ft(TextTlvMessage_Parse);

   //  This is a pure virtual function.
   //
   Context::Kill(TextTlvMessage_Parse, GetProtocol(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void TextTlvMessage::Patch(sel_t selector, void* arguments)
{
   TlvMessage::Patch (selector, arguments);
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_Receive = "TextTlvMessage.Receive";

bool TextTlvMessage::Receive()
{
   Debug::ft(TextTlvMessage_Receive);

   if(!text_) return true;
   auto buff = Parse();
   if(buff == nullptr) return false;
   Replace(buff);
   text_ = true;
   return true;
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_Send = "TextTlvMessage.Send";

bool TextTlvMessage::Send(Route route)
{
   Debug::ft(TextTlvMessage_Send);

   if(!text_)
   {
      auto buff = Build();
      if(buff == nullptr) return false;
      Replace(buff);
      text_ = true;
   }

   //  We need to skip our base class (TlvMessage::Send) because it checks
   //  the message fence, which does not exist in a text message.
   //
   return Message::Send(route);
}
}
