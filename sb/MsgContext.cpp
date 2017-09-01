//==============================================================================
//
//  MsgContext.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MsgContext.h"
#include "Debug.h"
#include "Message.h"
#include "MsgFactory.h"
#include "SysTypes.h"

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
   Debug::ft(MsgContext_dtor);
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