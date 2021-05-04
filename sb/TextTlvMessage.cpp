//==============================================================================
//
//  TextTlvMessage.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
TextTlvMessage::TextTlvMessage(SbIpBufferPtr& buff) :
   TlvMessage(buff),
   text_(true)
{
   Debug::ft("TextTlvMessage.ctor(i/c)");
}

//------------------------------------------------------------------------------

TextTlvMessage::TextTlvMessage(ProtocolSM* psm, size_t size) :
   TlvMessage(psm, size),
   text_(false)
{
   Debug::ft("TextTlvMessage.ctor(o/g)");
}

//------------------------------------------------------------------------------

TextTlvMessage::~TextTlvMessage()
{
   Debug::ftnt("TextTlvMessage.dtor");
}

//------------------------------------------------------------------------------

SbIpBufferPtr TextTlvMessage::Build()
{
   Debug::ft("TextTlvMessage.Build");

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

SbIpBufferPtr TextTlvMessage::Parse()
{
   Debug::ft("TextTlvMessage.Parse");

   Context::Kill(strOver(this), GetProtocol());
   return nullptr;
}

//------------------------------------------------------------------------------

void TextTlvMessage::Patch(sel_t selector, void* arguments)
{
   TlvMessage::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool TextTlvMessage::Receive()
{
   Debug::ft("TextTlvMessage.Receive");

   if(!text_) return true;
   auto buff = Parse();
   if(buff == nullptr) return false;
   Replace(buff);
   text_ = true;
   return true;
}

//------------------------------------------------------------------------------

bool TextTlvMessage::Send(Route route)
{
   Debug::ft("TextTlvMessage.Send");

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
