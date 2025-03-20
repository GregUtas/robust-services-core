//==============================================================================
//
//  SbInputHandler.cpp
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
#include "SbInputHandler.h"
#include <sstream>
#include <string>
#include "Debug.h"
#include "InvokerPool.h"
#include "InvokerPoolRegistry.h"
#include "IpPort.h"
#include "Log.h"
#include "MsgHeader.h"
#include "NbTypes.h"
#include "SbIpBuffer.h"
#include "SbLogs.h"
#include "SbTypes.h"
#include "Singleton.h"

using namespace NetworkBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
SbInputHandler::SbInputHandler(IpPort* port) : InputHandler(port)
{
   Debug::ft("SbInputHandler.ctor");
}

//------------------------------------------------------------------------------

SbInputHandler::~SbInputHandler()
{
   Debug::ftnt("SbInputHandler.dtor");
}

//------------------------------------------------------------------------------

IpBuffer* SbInputHandler::AllocBuff(const byte_t* source,
   size_t size, byte_t*& dest, size_t& rcvd, SysTcpSocket* socket) const
{
   Debug::ft("SbInputHandler.AllocBuff");

   if(size < sizeof(MsgHeader))
   {
      Port()->InvalidDiscarded();

      auto log = Log::Create(SessionLogGroup, InvalidIncomingMessage);
      if(log == nullptr) return nullptr;
      *log << Log::Tab << "port=" << Port()->GetPort();
      *log << " size=" << size;
      Log::Submit(log);
      return nullptr;
   }

   auto header = reinterpret_cast<const MsgHeader*>(source);
   rcvd = sizeof(MsgHeader) + header->length;

   auto buff = new SbIpBuffer(MsgIncoming, rcvd - sizeof(MsgHeader));
   dest = buff->HeaderPtr();
   return buff;
}

//------------------------------------------------------------------------------

void SbInputHandler::Patch(sel_t selector, void* arguments)
{
   InputHandler::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void SbInputHandler::ReceiveBuff
   (IpBufferPtr& buff, size_t size, Faction faction) const
{
   Debug::ft("SbInputHandler.ReceiveBuff");

   //  Find the invoker pool associated with FACTION and pass it the buffer
   //  to have it added to that pool's work queue.
   //
   auto pool = Singleton<InvokerPoolRegistry>::Instance()->Pool(faction);
   if(pool == nullptr) return;

   SbIpBufferPtr sbbuff(static_cast<SbIpBuffer*>(buff.release()));
   pool->ReceiveBuff(sbbuff, true);
}
}
