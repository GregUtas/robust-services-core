//==============================================================================
//
//  PotsCallHandler.cpp
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
#include "PotsSessions.h"
#include <memory>
#include <sstream>
#include <string>
#include "Debug.h"
#include "IpPort.h"
#include "LocalAddress.h"
#include "Log.h"
#include "Message.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "PotsCircuit.h"
#include "PotsLogs.h"
#include "PotsProfile.h"
#include "PotsProtocol.h"
#include "SbAppIds.h"
#include "SbInvokerPools.h"
#include "SbIpBuffer.h"
#include "Singleton.h"
#include "Switch.h"
#include "TlvParameter.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsCallHandler_ctor = "PotsCallHandler.ctor";

PotsCallHandler::PotsCallHandler(IpPort* port) : SbExtInputHandler(port)
{
   Debug::ft(PotsCallHandler_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCallHandler_dtor = "PotsCallHandler.dtor";

PotsCallHandler::~PotsCallHandler()
{
   Debug::ftnt(PotsCallHandler_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCallHandler_DiscardBuff = "PotsCallHandler.DiscardBuff";

void PotsCallHandler::DiscardBuff
   (const IpBufferPtr& buff, const PotsHeaderInfo* phi, word errval) const
{
   Debug::ft(PotsCallHandler_DiscardBuff);

   Port()->InvalidDiscarded();

   auto log = Log::Create(PotsLogGroup, PotsCallIcBuffer);

   if(log != nullptr)
   {
      *log << Log::Tab << "port=" << phi->port;
      *log << " signal=" << phi->signal << " errval=" << errval << CRLF;
      buff->Display(*log, Log::Tab, VerboseOpt);
      Log::Submit(log);
   }
}

//------------------------------------------------------------------------------

fn_name PotsCallHandler_ReceiveBuff = "PotsCallHandler.ReceiveBuff";

void PotsCallHandler::ReceiveBuff
   (IpBufferPtr& buff, size_t size, Faction faction) const
{
   Debug::ft(PotsCallHandler_ReceiveBuff);

   auto sbuff = static_cast< SbIpBuffer* >(buff.get());
   auto header = sbuff->Header();
   auto pptr = reinterpret_cast< TlvParmPtr >(sbuff->PayloadPtr());
   auto phi = reinterpret_cast< PotsHeaderInfo* >(pptr->bytes);
   auto cct = Singleton< Switch >::Instance()->GetCircuit(phi->port);

   //  Verify that the circuit exists, that it is a POTS circuit, and that
   //  it has a profile.
   //
   if(cct == nullptr)
   {
      DiscardBuff(buff, phi, 0);
      return;
   }

   if(!cct->Supports(PotsProtocolId))
   {
      DiscardBuff(buff, phi, phi->port);
      return;
   }

   auto prof = static_cast< PotsCircuit* >(cct)->Profile();

   if(prof == nullptr)
   {
      DiscardBuff(buff, phi, -1);
      return;
   }

   //  Construct the message header.  If the PSM registered in the profile
   //  doesn't exist, this is an initial message, else it is a progress
   //  message.
   //
   header->route = Message::External;
   header->protocol = PotsProtocolId;
   header->signal = phi->signal;
   header->length = size;
   header->final = false;
   header->injected = true;
   header->txAddr.fid = PotsShelfFactoryId;

   auto addr = prof->ObjAddr();

   if(MsgPort::Find(addr) == nullptr)
   {
      if(Singleton< PayloadInvokerPool >::Instance()->RejectIngressWork())
      {
         //  The system is overloaded.  Discard the message unless it is an
         //  onhook, which is accepted in order to exit the Lockout state.
         //
         if(header->signal != PotsSignal::Onhook)
         {
            Port()->IngressDiscarded();
            return;
         }
      }

      header->initial = true;
      header->priority = INGRESS;
      header->rxAddr.fid = PotsCallFactoryId;
   }
   else
   {
      header->initial = false;
      header->priority = PROGRESS;
      header->rxAddr = addr;
   }

   //  Invoke the base class implementation to queue the message.  The base
   //  class assumes that SIZE includes a header.  The original message didn't
   //  have the header, but now it does, so adjust the size accordingly.
   //
   SbExtInputHandler::ReceiveBuff(buff, sizeof(MsgHeader) + size, faction);
}
}
