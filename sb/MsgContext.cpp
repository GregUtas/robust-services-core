//==============================================================================
//
//  MsgContext.cpp
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
#include "MsgContext.h"
#include "Debug.h"
#include "Message.h"
#include "MsgFactory.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name MsgContext_ctor = "MsgContext.ctor";

MsgContext::MsgContext(Faction faction) : Context(faction)
{
   Debug::ft(MsgContext_ctor);
}

//------------------------------------------------------------------------------

fn_name MsgContext_dtor = "MsgContext.dtor";

MsgContext::~MsgContext()
{
   Debug::ftnt(MsgContext_dtor);
}

//------------------------------------------------------------------------------

fn_name MsgContext_EndOfTransaction = "MsgContext.EndOfTransaction";

void MsgContext::EndOfTransaction()
{
   Debug::ft(MsgContext_EndOfTransaction);

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

fn_name MsgContext_ProcessIcMsg = "MsgContext.ProcessIcMsg";

void MsgContext::ProcessIcMsg(Message& msg)
{
   Debug::ft(MsgContext_ProcessIcMsg);

   //  Tell the factory associated with this context to process MSG.
   //
   auto fac = msg.RxFactory();

   TraceMsg(msg.GetProtocol(), msg.GetSignal(), MsgIncoming);
   static_cast< MsgFactory* >(fac)->ProcessIcMsg(msg);
}
}
