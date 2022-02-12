//==============================================================================
//
//  PsmFactory.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "PsmFactory.h"
#include "Debug.h"
#include "GlobalAddress.h"
#include "LocalAddress.h"
#include "Message.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "ProtocolSM.h"
#include "PsmContext.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
PsmFactory::PsmFactory(Id fid, ContextType type, ProtocolId prid,
   c_string name) : MsgFactory(fid, type, prid, name)
{
   Debug::ft("PsmFactory.ctor");
}

//------------------------------------------------------------------------------

PsmFactory::~PsmFactory()
{
   Debug::ftnt("PsmFactory.dtor");
}

//------------------------------------------------------------------------------

Context* PsmFactory::AllocContext() const
{
   Debug::ft("PsmFactory.AllocContext");

   return new PsmContext(GetFaction());
}

//------------------------------------------------------------------------------

fn_name PsmFactory_AllocIcPsm = "PsmFactory.AllocIcPsm";

ProtocolSM* PsmFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft(PsmFactory_AllocIcPsm);

   Debug::SwLog(PsmFactory_AllocIcPsm, strOver(this), Fid());
   return nullptr;
}

//------------------------------------------------------------------------------

void PsmFactory::Patch(sel_t selector, void* arguments)
{
   MsgFactory::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

Factory::Rc PsmFactory::ReceiveMsg
   (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx)
{
   Debug::ft("PsmFactory.ReceiveMsg");

   MsgPort* port = nullptr;
   auto header = msg.Header();

   if(header->rxAddr.bid != NIL_ID)
   {
      //  Find the context using the port to which the message is addressed.
      //
      port = MsgPort::Find(header->rxAddr);
      if(port == nullptr) return PortNotFound;
      ctx = port->GetContext();
      if(ctx == nullptr) return ContextNotFound;
   }
   else
   {
      //  The message isn't addressed to a port.  There are three cases:
      //  (a) Initial message.  Create a context and port.  A port is created
      //      at I/O level (that is, as soon as a message arrives) because it
      //      immediately records the peer's address.  MsgPort.FindPeer can
      //      then deliver a subsequent message to the same port and context
      //      if the dialog's initiator sends another message before it has
      //      received a reply.  Similarly, the MsgPort constructor invokes
      //      PortAllocated on the factory that received this message.  The
      //      factory can save the port in a user profile so that subsequent
      //      messages from the user can also be delivered to the port and
      //      context that were just created to handle this initial message.
      //  (b) Join message.  SsmContext.ReceiveMsg has set CTX to the context
      //      that will handle this session.  Create a port on that context.
      //  (c) Subsequent message.  The message was not addressed to a port
      //      because the sender had not yet received a reply, so it still
      //      doesn't know the address of the port that was allocated for its
      //      initial message.  This is prevented, for intraprocessor messages
      //      only, by MsgPort.UpdatePeer.  But for an interprocessor message,
      //      we must search for the port that contains the sender's address.
      //
      if(header->initial)
      {
         if(!header->join)  // case (a)
         {
            ctx = AllocContext();
            if(ctx == nullptr) return CtxAllocFailed;
            IncrContexts();
            port = new MsgPort(msg, *ctx);
         }
         else  // case (b)
         {
            if(ctx == nullptr) return ContextNotFound;
            port = new MsgPort(msg, *ctx);
         }
      }
      else  // case (c)
      {
         port = MsgPort::FindPeer(msg.GetSender());
         if(port == nullptr) return PortNotFound;
         ctx = port->GetContext();
      }

      //  We just created or found the port that will receive the
      //  message, so update the message header with its address.
      //
      msg.SetRxAddr(port->LocAddr().SbAddr());
      header->rxAddr = port->LocAddr().SbAddr();
   }

   return MsgFactory::ReceiveMsg(msg, atIoLevel, tt, ctx);
}
}
