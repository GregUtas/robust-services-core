//==============================================================================
//
//  InputHandler.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "InputHandler.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "IpBuffer.h"
#include "IpPort.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
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

IpBuffer* InputHandler::AllocBuff
   (const byte_t* source, MsgSize size, byte_t*& dest, MsgSize& rcvd) const
{
   Debug::ft(InputHandler_AllocBuff);

   auto buffer = new IpBuffer(MsgIncoming, 0, size);
   if(buffer == nullptr) return nullptr;
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

void InputHandler::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name InputHandler_ReceiveBuff = "InputHandler.ReceiveBuff";

void InputHandler::ReceiveBuff
   (MsgSize size, IpBufferPtr& buff, Faction faction) const
{
   Debug::ft(InputHandler_ReceiveBuff);

   Debug::SwErr(InputHandler_ReceiveBuff, faction, 0);
}
}
