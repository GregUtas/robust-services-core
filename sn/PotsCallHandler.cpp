//==============================================================================
//
//  PotsCallHandler.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsSessions.h"
#include <memory>
#include <sstream>
#include "Debug.h"
#include "IpPort.h"
#include "LocalAddress.h"
#include "Log.h"
#include "Message.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "PotsCircuit.h"
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
   Debug::ft(PotsCallHandler_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCallHandler_DiscardBuff = "PotsCallHandler.DiscardBuff";

void PotsCallHandler::DiscardBuff
   (const IpBufferPtr& buff, const PotsHeaderInfo* phi, word errval) const
{
   Debug::ft(PotsCallHandler_DiscardBuff);

   Port()->InvalidDiscarded();

   auto log = Log::Create("POTS CALL INVALID INCOMING BUFFER");

   if(log != nullptr)
   {
      *log << "port=" << phi->port;
      *log << " signal=" << phi->signal;
      *log << " errval=" << errval << CRLF;
      buff->Display(*log, EMPTY_STR, Flags(Vb_Mask));
      Log::Spool(log);
   }
}

//------------------------------------------------------------------------------

fn_name PotsCallHandler_ReceiveBuff = "PotsCallHandler.ReceiveBuff";

void PotsCallHandler::ReceiveBuff
   (MsgSize size, IpBufferPtr& buff, Faction faction) const
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
      header->priority = Message::Ingress;
      header->rxAddr.fid = PotsCallFactoryId;
   }
   else
   {
      header->initial = false;
      header->priority = Message::Progress;
      header->rxAddr = addr;
   }

   //  Invoke the base class implementation to queue the message.  The base
   //  class assumes that SIZE includes a header.  The original message didn't
   //  have the header, but now it does, so adjust the size accordingly.
   //
   SbExtInputHandler::ReceiveBuff(sizeof(MsgHeader) + size, buff, faction);
}
}
