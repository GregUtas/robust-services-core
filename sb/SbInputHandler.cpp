//==============================================================================
//
//  SbInputHandler.cpp
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
#include "SbInputHandler.h"
#include <sstream>
#include "Debug.h"
#include "InvokerPool.h"
#include "InvokerPoolRegistry.h"
#include "IpPort.h"
#include "Log.h"
#include "MsgHeader.h"
#include "NbTypes.h"
#include "SbIpBuffer.h"
#include "SbTypes.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name SbInputHandler_ctor = "SbInputHandler.ctor";

SbInputHandler::SbInputHandler(IpPort* port) : InputHandler(port)
{
   Debug::ft(SbInputHandler_ctor);
}

//------------------------------------------------------------------------------

fn_name SbInputHandler_dtor = "SbInputHandler.dtor";

SbInputHandler::~SbInputHandler()
{
   Debug::ft(SbInputHandler_dtor);
}

//------------------------------------------------------------------------------

fn_name SbInputHandler_AllocBuff = "SbInputHandler.AllocBuff";

IpBuffer* SbInputHandler::AllocBuff(const byte_t* source,
   MsgSize size, byte_t*& dest, MsgSize& rcvd, SysTcpSocket* socket) const
{
   Debug::ft(SbInputHandler_AllocBuff);

   if(size < sizeof(MsgHeader))
   {
      Port()->InvalidDiscarded();

      auto log = Log::Create("INVALID INCOMING MESSAGE");
      if(log == nullptr) return nullptr;
      *log << "port=" << Port()->GetPort();
      *log << " size=" << size << CRLF;
      Log::Spool(log);
      return nullptr;
   }

   auto header = reinterpret_cast< const MsgHeader* >(source);
   rcvd = sizeof(MsgHeader) + header->length;

   auto buff = new SbIpBuffer(MsgIncoming, rcvd - sizeof(MsgHeader));
   if(buff == nullptr) return nullptr;
   dest = buff->HeaderPtr();
   return buff;
}

//------------------------------------------------------------------------------

void SbInputHandler::Patch(sel_t selector, void* arguments)
{
   InputHandler::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SbInputHandler_ReceiveBuff = "SbInputHandler.ReceiveBuff";

void SbInputHandler::ReceiveBuff
   (IpBufferPtr& buff, MsgSize size, Faction faction) const
{
   Debug::ft(SbInputHandler_ReceiveBuff);

   //  Find the invoker pool associated with FACTION and pass it the buffer
   //  to have it added to that pool's work queue.
   //
   auto pool = Singleton< InvokerPoolRegistry >::Instance()->Pool(faction);
   if(pool == nullptr) return;

   auto sbbuff = SbIpBufferPtr(static_cast< SbIpBuffer* >(buff.release()));
   pool->ReceiveBuff(sbbuff, true);
}
}