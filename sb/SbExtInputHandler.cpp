//==============================================================================
//
//  SbExtInputHandler.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SbExtInputHandler.h"
#include "Debug.h"
#include "NbTypes.h"
#include "SbIpBuffer.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name SbExtInputHandler_ctor = "SbExtInputHandler.ctor";

SbExtInputHandler::SbExtInputHandler(IpPort* port) : SbInputHandler(port)
{
   Debug::ft(SbExtInputHandler_ctor);
}

//------------------------------------------------------------------------------

fn_name SbExtInputHandler_dtor = "SbExtInputHandler.dtor";

SbExtInputHandler::~SbExtInputHandler()
{
   Debug::ft(SbExtInputHandler_dtor);
}

//------------------------------------------------------------------------------

fn_name SbExtInputHandler_AllocBuff = "SbExtInputHandler.AllocBuff";

IpBuffer* SbExtInputHandler::AllocBuff
   (const byte_t* source, MsgSize size, byte_t*& dest, MsgSize& rcvd) const
{
   Debug::ft(SbExtInputHandler_AllocBuff);

   auto buff = new SbIpBuffer(MsgIncoming, size);
   if(buff == nullptr) return nullptr;
   dest = buff->PayloadPtr();
   return buff;
}

//------------------------------------------------------------------------------

void SbExtInputHandler::Patch(sel_t selector, void* arguments)
{
   SbInputHandler::Patch(selector, arguments);
}
}