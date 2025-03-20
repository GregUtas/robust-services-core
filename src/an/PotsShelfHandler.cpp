//==============================================================================
//
//  PotsShelfHandler.cpp
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
#include "PotsShelf.h"
#include <memory>
#include <sstream>
#include <string>
#include "Circuit.h"
#include "Debug.h"
#include "LocalAddress.h"
#include "Log.h"
#include "Message.h"
#include "MsgHeader.h"
#include "PotsLogs.h"
#include "PotsProtocol.h"
#include "PotsTrafficThread.h"
#include "SbAppIds.h"
#include "SbIpBuffer.h"
#include "SbTypes.h"
#include "Singleton.h"
#include "Switch.h"
#include "TlvParameter.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsShelfHandler::PotsShelfHandler(IpPort* port) :
   SbExtInputHandler(port)
{
   Debug::ft("PotsShelfHandler.ctor");
}

//------------------------------------------------------------------------------

PotsShelfHandler::~PotsShelfHandler()
{
   Debug::ftnt("PotsShelfHandler.dtor");
}

//------------------------------------------------------------------------------

void PotsShelfHandler::ReceiveBuff
   (IpBufferPtr& buff, size_t size, Faction faction) const
{
   Debug::ft("PotsShelfHandler.ReceiveBuff");

   auto sbuff = static_cast<SbIpBuffer*>(buff.get());
   auto header = sbuff->Header();
   auto pptr = reinterpret_cast<TlvParm*>(sbuff->PayloadPtr());
   auto phi = reinterpret_cast<PotsHeaderInfo*>(pptr->bytes);

   //  Verify that the message is addressed to an existing POTS circuit.
   //
   auto cct = Singleton<Switch>::Instance()->GetCircuit(phi->port);

   if(cct == nullptr) return;

   if(!cct->Supports(PotsProtocolId))
   {
      buff->InvalidDiscarded();

      auto log = Log::Create(PotsLogGroup, PotsShelfIcBuffer);

      if(log != nullptr)
      {
         *log << Log::Tab << "port=" << phi->port;
         *log << "sig=" << phi->signal;
         Log::Submit(log);
      }

      return;
   }

   header->route = Message::External;
   header->protocol = PotsProtocolId;
   header->signal = phi->signal;
   header->length = size;

   header->initial = false;
   header->final = (phi->signal != PotsSignal::Supervise);
   header->priority = PROGRESS;
   header->rxAddr.fid = PotsShelfFactoryId;
   header->txAddr.fid = PotsCallFactoryId;

   //  If traffic is running, give the shelf absolute priority over the
   //  call server so that the call server will enter overload first.
   //
   auto thr = Singleton<PotsTrafficThread>::Extant();

   if((thr != nullptr) && (thr->GetRate() > 0))
   {
      header->priority = IMMEDIATE;
   }

   //  Invoke the base class implementation to queue the message.  The base
   //  class assumes that SIZE includes a header.  The original message didn't
   //  have the header, but now it does, so adjust the length accordingly.
   //
   SbExtInputHandler::ReceiveBuff(buff, sizeof(MsgHeader) + size, faction);
}
}
