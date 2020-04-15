//==============================================================================
//
//  InputHandler.cpp
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
#include "InputHandler.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "Memory.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name InputHandler_ctor = "InputHandler.ctor";

InputHandler::InputHandler(IpPort* port) : port_(port)
{
   Debug::ft(InputHandler_ctor);

   Debug::Assert(port_->BindHandler(*this));
}

//------------------------------------------------------------------------------

fn_name InputHandler_dtor = "InputHandler.dtor";

InputHandler::~InputHandler()
{
   Debug::ft(InputHandler_dtor);

   port_->UnbindHandler(*this);
}

//------------------------------------------------------------------------------

fn_name InputHandler_AllocBuff = "InputHandler.AllocBuff";

IpBuffer* InputHandler::AllocBuff(const byte_t* source,
   size_t size, byte_t*& dest, size_t& rcvd, SysTcpSocket* socket) const
{
   Debug::ft(InputHandler_AllocBuff);

   auto buffer = new IpBuffer(MsgIncoming, 0, size);
   dest = buffer->HeaderPtr();
   return buffer;
}

//------------------------------------------------------------------------------

void InputHandler::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "port : " << port_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name InputHandler_HostToNetwork = "InputHandler.HostToNetwork";

byte_t* InputHandler::HostToNetwork
   (IpBuffer& buff, byte_t* src, size_t size) const
{
   Debug::ft(InputHandler_HostToNetwork);

   return src;
}

//------------------------------------------------------------------------------

fn_name InputHandler_NetworkToHost = "InputHandler.NetworkToHost";

void InputHandler::NetworkToHost
   (IpBuffer& buff, byte_t* dest, const byte_t* src, size_t size) const
{
   Debug::ft(InputHandler_NetworkToHost);

   Memory::Copy(dest, src, size);
}

//------------------------------------------------------------------------------

void InputHandler::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name InputHandler_ReceiveBuff = "InputHandler.ReceiveBuff";

void InputHandler::ReceiveBuff
   (IpBufferPtr& buff, size_t size, Faction faction) const
{
   Debug::ft(InputHandler_ReceiveBuff);

   Debug::SwLog(InputHandler_ReceiveBuff, strOver(this), faction);
}
}
