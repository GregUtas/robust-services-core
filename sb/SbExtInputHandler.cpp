//==============================================================================
//
//  SbExtInputHandler.cpp
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
#include "SbExtInputHandler.h"
#include "Debug.h"
#include "NbTypes.h"
#include "SbIpBuffer.h"

using namespace NetworkBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
SbExtInputHandler::SbExtInputHandler(IpPort* port) : SbInputHandler(port)
{
   Debug::ft("SbExtInputHandler.ctor");
}

//------------------------------------------------------------------------------

SbExtInputHandler::~SbExtInputHandler()
{
   Debug::ftnt("SbExtInputHandler.dtor");
}

//------------------------------------------------------------------------------

IpBuffer* SbExtInputHandler::AllocBuff(const byte_t* source,
   size_t size, byte_t*& dest, size_t& rcvd, SysTcpSocket* socket) const
{
   Debug::ft("SbExtInputHandler.AllocBuff");

   auto buff = new SbIpBuffer(MsgIncoming, size);
   dest = buff->PayloadPtr();
   return buff;
}

//------------------------------------------------------------------------------

void SbExtInputHandler::Patch(sel_t selector, void* arguments)
{
   SbInputHandler::Patch(selector, arguments);
}
}
