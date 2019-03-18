//==============================================================================
//
//  SbExtInputHandler.cpp
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
#include "SbExtInputHandler.h"
#include "Debug.h"
#include "NbTypes.h"
#include "SbIpBuffer.h"
#include "SysTypes.h"

using namespace NetworkBase;
using namespace NodeBase;

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

IpBuffer* SbExtInputHandler::AllocBuff(const byte_t* source,
   size_t size, byte_t*& dest, size_t& rcvd, SysTcpSocket* socket) const
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
