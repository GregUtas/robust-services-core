//==============================================================================
//
//  MsgContext.cpp
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
#include "MsgContext.h"
#include "Debug.h"
#include "Message.h"
#include "MsgFactory.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
MsgContext::MsgContext(Faction faction) : Context(faction)
{
   Debug::ft("MsgContext.ctor");
}

//------------------------------------------------------------------------------

MsgContext::~MsgContext()
{
   Debug::ftnt("MsgContext.dtor");
}

//------------------------------------------------------------------------------

void MsgContext::EndOfTransaction()
{
   Debug::ft("MsgContext.EndOfTransaction");

   //  The context message has been handled.
   //
   auto msg = ContextMsg();
   if(msg != nullptr) msg->Handled(false);
}

//------------------------------------------------------------------------------

void MsgContext::Patch(sel_t selector, void* arguments)
{
   Context::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void MsgContext::ProcessIcMsg(Message& msg)
{
   Debug::ft("MsgContext.ProcessIcMsg");

   //  Tell the factory associated with this context to process MSG.
   //
   auto fac = msg.RxFactory();

   TraceMsg(msg.GetProtocol(), msg.GetSignal(), MsgIncoming);
   static_cast<MsgFactory*>(fac)->ProcessIcMsg(msg);
}
}
