//==============================================================================
//
//  PotsShelfFactory.cpp
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
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "CliText.h"
#include "Debug.h"
#include "Log.h"
#include "PotsCircuit.h"
#include "PotsLogs.h"
#include "PotsProtocol.h"
#include "SbAppIds.h"
#include "SbTypes.h"
#include "Singleton.h"
#include "Switch.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Invoked when an invalid message is found.
//
static void DiscardMsg(const Message& msg, Switch::PortId port)
{
   Debug::ft("PotsBase.DiscardMsg");

   msg.InvalidDiscarded();

   auto log = Log::Create(PotsLogGroup, PotsShelfIcMessage);
   if(log == nullptr) return;
   *log << Log::Tab << "signal=" << msg.GetSignal();
   *log << " port=" << port << CRLF;
   msg.Output(*log, Log::Indent, true);
   Log::Submit(log);
}

//------------------------------------------------------------------------------

PotsShelfFactory::PotsShelfFactory() :
   MsgFactory(PotsShelfFactoryId, SingleMsg, PotsProtocolId, "POTS Shelf")
{
   Debug::ft("PotsShelfFactory.ctor");

   //  The factory receives and sends the following subset of signals
   //  defined by the POTS protocol.
   //
   AddIncomingSignal(PotsSignal::Supervise);
   AddIncomingSignal(PotsSignal::Lockout);
   AddIncomingSignal(PotsSignal::Release);

   AddOutgoingSignal(PotsSignal::Offhook);
   AddOutgoingSignal(PotsSignal::Alerting);
   AddOutgoingSignal(PotsSignal::Digits);
   AddOutgoingSignal(PotsSignal::Flash);
   AddOutgoingSignal(PotsSignal::Onhook);
}

//------------------------------------------------------------------------------

PotsShelfFactory::~PotsShelfFactory()
{
   Debug::ftnt("PotsShelfFactory.dtor");
}

//------------------------------------------------------------------------------

Message* PotsShelfFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("PotsShelfFactory.AllocIcMsg");

   return new Pots_NU_Message(buff);
}

//------------------------------------------------------------------------------

Message* PotsShelfFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft("PotsShelfFactory.AllocOgMsg");

   return new Pots_UN_Message(nullptr, 12);
}

//------------------------------------------------------------------------------

fixed_string PotsShelfFactoryStr = "PS";
fixed_string PotsShelfFactoryExpl = "POTS Shelf";

CliText* PotsShelfFactory::CreateText() const
{
   Debug::ft("PotsShelfFactory.CreateText");

   return new CliText(PotsShelfFactoryExpl, PotsShelfFactoryStr);
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_InjectMsg = "PotsShelfFactory.InjectMsg";

bool PotsShelfFactory::InjectMsg(Message& msg) const
{
   Debug::ft(PotsShelfFactory_InjectMsg);

   auto& pmsg = static_cast<Pots_UN_Message&>(msg);
   auto phi = pmsg.FindType<PotsHeaderInfo>(PotsParameter::Header);

   //  Send the message from the specified POTS circuit.
   //
   if(phi == nullptr)
   {
      Debug::SwLog(PotsShelfFactory_InjectMsg,
         "header not found", pmsg.GetSignal());
      return false;
   }

   auto tsw = Singleton<Switch>::Instance();
   auto cct = static_cast<PotsCircuit*>(tsw->GetCircuit(phi->port));

   if(cct == nullptr)
   {
      Debug::SwLog(PotsShelfFactory_InjectMsg,
         "circuit not found", pack2(phi->port, pmsg.GetSignal()));
      return false;
   }

   return cct->SendMsg(pmsg);
}

//------------------------------------------------------------------------------

void PotsShelfFactory::ProcessIcMsg(Message& msg) const
{
   Debug::ft("PotsShelfFactory.ProcessIcMsg");

   //  Have the specified POTS circuit process the message.
   //
   auto& pmsg = static_cast<Pots_NU_Message&>(msg);
   auto phi = pmsg.FindType<PotsHeaderInfo>(PotsParameter::Header);

   if(phi == nullptr)
   {
      DiscardMsg(msg, 0);
      return;
   }

   auto tsw = Singleton<Switch>::Instance();
   auto cct = static_cast<PotsCircuit*>(tsw->GetCircuit(phi->port));

   if(cct == nullptr)
   {
      DiscardMsg(msg, phi->port);
      return;
   }

   cct->ReceiveMsg(pmsg);
}

//------------------------------------------------------------------------------

Message* PotsShelfFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("PotsShelfFactory.ReallocOgMsg");

   return new Pots_UN_Message(buff);
}
}
