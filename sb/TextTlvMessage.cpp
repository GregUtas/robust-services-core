//==============================================================================
//
//  TextTlvMessage.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "TextTlvMessage.h"
#include <ostream>
#include <string>
#include "Context.h"
#include "Debug.h"
#include "SysTypes.h"

using namespace NodeBase;
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

TextTlvMessage::TextTlvMessage(ProtocolSM* psm, size_t size) :
   TlvMessage(psm, size),
   text_(false)
{
   Debug::ft(TextTlvMessage_ctor2);
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_dtor = "TextTlvMessage.dtor";

TextTlvMessage::~TextTlvMessage()
{
   Debug::ftnt(TextTlvMessage_dtor);
}

//------------------------------------------------------------------------------

fn_name TextTlvMessage_Build = "TextTlvMessage.Build";

SbIpBufferPtr TextTlvMessage::Build()
{
   Debug::ft(TextTlvMessage_Build);

   Context::Kill(strOver(this), GetProtocol());
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

   Context::Kill(strOver(this), GetProtocol());
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
