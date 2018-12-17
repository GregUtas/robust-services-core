//==============================================================================
//
//  PsmContext.cpp
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
#include "PsmContext.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Event.h"
#include "Formatters.h"
#include "GlobalAddress.h"
#include "Message.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "ProtocolSM.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name PsmContext_ctor = "PsmContext.ctor";

PsmContext::PsmContext(Faction faction) : MsgContext(faction)
{
   Debug::ft(PsmContext_ctor);

   portq_.Init(Pooled::LinkDiff());
   psmq_.Init(Pooled::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name PsmContext_dtor = "PsmContext.dtor";

PsmContext::~PsmContext()
{
   Debug::ft(PsmContext_dtor);

   //  Delete all PSMs and ports.  PSMs are deleted ahead of ports so that
   //  SendFinalMsg can send a message down the stack.
   //
   while(!psmq_.Empty())
   {
      auto p = psmq_.First();
      p->Destroy();
   }

   portq_.Purge();
}

//------------------------------------------------------------------------------

void PsmContext::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Context::Display(stream, prefix, options);

   stream << prefix << "portq : " << CRLF;
   portq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "psmq  : " << CRLF;
   psmq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name PsmContext_EndOfTransaction = "PsmContext.EndOfTransaction";

void PsmContext::EndOfTransaction()
{
   Debug::ft(PsmContext_EndOfTransaction);

   MsgContext::EndOfTransaction();

   //  Prompt all PSMs to send any pending outgoing messages.
   //
   for(auto p = FirstPsm(); p != nullptr; NextPsm(p))
   {
      p->EndOfTransaction();
   }

   //  Destroy any PSMs that are now in the idle state.  This must be deferred
   //  until now to support protocol stacks.  When a PSM in a stack is idle,
   //  any non-idle PSMs in the stack get idled.  This should not occur until
   //  EndOfTransaction is invoked on all PSMs, so that any outgoing message
   //  can first traverse its entire stack.  Given that the call to Destroy
   //  can make additional PSMs idle, always look for idle PSMs from the head
   //  of the queue, and keep iterating until no more are found and deleted.
   //
   for(auto found = true; found; NO_OP)
   {
      found = false;

      for(auto p = FirstPsm(); p != nullptr; NextPsm(p))
      {
         if(p->GetState() == ProtocolSM::Idle)
         {
            p->Destroy();
            found = true;
            break;
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name PsmContext_EnqPort = "PsmContext.EnqPort";

void PsmContext::EnqPort(MsgPort& port)
{
   Debug::ft(PsmContext_EnqPort);

   portq_.Enq(port);
}

//------------------------------------------------------------------------------

fn_name PsmContext_EnqPsm = "PsmContext.EnqPsm";

void PsmContext::EnqPsm(ProtocolSM& psm)
{
   Debug::ft(PsmContext_EnqPsm);

   //  Queue the PSM before any PSMs of lower priority.
   //
   auto prio = psm.GetPriority();
   ProtocolSM* prev = nullptr;

   for(auto curr = psmq_.First(); curr != nullptr; curr = psmq_.Next(*curr))
   {
      if(curr->GetPriority() < prio) break;
      prev = curr;
   }

   psmq_.Insert(prev, psm);
}

//------------------------------------------------------------------------------

fn_name PsmContext_ExqPort = "PsmContext.ExqPort";

void PsmContext::ExqPort(MsgPort& port)
{
   Debug::ft(PsmContext_ExqPort);

   if(!portq_.Exq(port))
   {
      Debug::SwLog(PsmContext_ExqPort, port.LocAddr().Fid(), 0);
   }
}

//------------------------------------------------------------------------------

fn_name PsmContext_ExqPsm = "PsmContext.ExqPsm";

void PsmContext::ExqPsm(ProtocolSM& psm)
{
   Debug::ft(PsmContext_ExqPsm);

   if(!psmq_.Exq(psm))
   {
      Debug::SwLog(PsmContext_ExqPsm, psm.GetFactory(), 0);
   }
}

//------------------------------------------------------------------------------

fn_name PsmContext_FindPort = "PsmContext.FindPort";

MsgPort* PsmContext::FindPort(const Message& msg) const
{
   Debug::ft(PsmContext_FindPort);

   auto header = msg.Header();

   for(auto p = portq_.First(); p != nullptr; portq_.Next(p))
   {
      if(p->ObjAddr() == header->rxAddr)
      {
         return p;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name PsmContext_GetSubtended = "PsmContext.GetSubtended";

void PsmContext::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(PsmContext_GetSubtended);

   MsgContext::GetSubtended(objects, count);

   for(auto p = psmq_.First(); p != nullptr; psmq_.Next(p))
   {
      p->GetSubtended(objects, count);
   }

   for(auto p = portq_.First(); p != nullptr; portq_.Next(p))
   {
      p->GetSubtended(objects, count);
   }
}

//------------------------------------------------------------------------------

fn_name PsmContext_HenqPsm = "PsmContext.HenqPsm";

void PsmContext::HenqPsm(ProtocolSM& psm)
{
   Debug::ft(PsmContext_HenqPsm);

   //  Queue the PSM before any PSMs of equal or lower priority.
   //
   auto prio = psm.GetPriority();
   ProtocolSM* prev = nullptr;

   for(auto curr = psmq_.First(); curr != nullptr; curr = psmq_.Next(*curr))
   {
      if(curr->GetPriority() <= prio) break;
      prev = curr;
   }

   psmq_.Insert(prev, psm);
}

//------------------------------------------------------------------------------

void PsmContext::Patch(sel_t selector, void* arguments)
{
   MsgContext::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name PsmContext_ProcessIcMsg = "PsmContext.ProcessIcMsg";

void PsmContext::ProcessIcMsg(Message& msg)
{
   Debug::ft(PsmContext_ProcessIcMsg);

   //  Find or create the port that will receive MSG.
   //
   MsgPort* port = FindPort(msg);
   if(port == nullptr) return;

   //  Tell the port to process MSG.  If this returns an event, delete it,
   //  because there is no root SSM to receive it.
   //
   TraceMsg(msg.GetProtocol(), msg.GetSignal(), MsgIncoming);
   auto evt = port->ReceiveMsg(msg);
   delete evt;
   EndOfTransaction();
}
}
