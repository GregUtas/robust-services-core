//==============================================================================
//
//  PotsShelfFactory.cpp
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
#include "PotsShelf.h"
#include "CliText.h"
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Log.h"
#include "PotsCircuit.h"
#include "PotsLogs.h"
#include "PotsProtocol.h"
#include "SbAppIds.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsShelfFactoryText : public CliText
{
public:
   PotsShelfFactoryText();
};

fixed_string PotsShelfFactoryStr = "PS";
fixed_string PotsShelfFactoryExpl = "POTS Shelf";

PotsShelfFactoryText::PotsShelfFactoryText() :
   CliText(PotsShelfFactoryExpl, PotsShelfFactoryStr) { }

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_ctor = "PotsShelfFactory.ctor";

PotsShelfFactory::PotsShelfFactory() :
   MsgFactory(PotsShelfFactoryId, SingleMsg, PotsProtocolId, "POTS Shelf")
{
   Debug::ft(PotsShelfFactory_ctor);

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

fn_name PotsShelfFactory_dtor = "PotsShelfFactory.dtor";

PotsShelfFactory::~PotsShelfFactory()
{
   Debug::ftnt(PotsShelfFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_AllocIcMsg = "PotsShelfFactory.AllocIcMsg";

Message* PotsShelfFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(PotsShelfFactory_AllocIcMsg);

   return new Pots_NU_Message(buff);
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_AllocOgMsg = "PotsShelfFactory.AllocOgMsg";

Message* PotsShelfFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft(PotsShelfFactory_AllocOgMsg);

   return new Pots_UN_Message(nullptr, 12);
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_CreateText = "PotsShelfFactory.CreateText";

CliText* PotsShelfFactory::CreateText() const
{
   Debug::ft(PotsShelfFactory_CreateText);

   return new PotsShelfFactoryText;
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_DiscardMsg = "PotsShelfFactory.DiscardMsg";

void PotsShelfFactory::DiscardMsg(const Message& msg, Switch::PortId port)
{
   Debug::ft(PotsShelfFactory_DiscardMsg);

   msg.InvalidDiscarded();

   auto log = Log::Create(PotsLogGroup, PotsShelfIcMessage);
   if(log == nullptr) return;
   *log << Log::Tab << "signal=" << msg.GetSignal();
   *log << " port=" << port << CRLF;
   msg.Output(*log, Log::Indent, true);
   Log::Submit(log);
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_InjectMsg = "PotsShelfFactory.InjectMsg";

bool PotsShelfFactory::InjectMsg(Message& msg) const
{
   Debug::ft(PotsShelfFactory_InjectMsg);

   auto& pmsg = static_cast< Pots_UN_Message& >(msg);
   auto phi = pmsg.FindType< PotsHeaderInfo >(PotsParameter::Header);

   //  Send the message from the specified POTS circuit.
   //
   if(phi == nullptr)
   {
      Debug::SwLog(PotsShelfFactory_InjectMsg,
         "header not found", pmsg.GetSignal());
      return false;
   }

   auto tsw = Singleton< Switch >::Instance();
   auto cct = static_cast< PotsCircuit* >(tsw->GetCircuit(phi->port));

   if(cct == nullptr)
   {
      Debug::SwLog(PotsShelfFactory_InjectMsg,
         "circuit not found", pack2(phi->port, pmsg.GetSignal()));
      return false;
   }

   return cct->SendMsg(pmsg);
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_ProcessIcMsg = "PotsShelfFactory.ProcessIcMsg";

void PotsShelfFactory::ProcessIcMsg(Message& msg) const
{
   Debug::ft(PotsShelfFactory_ProcessIcMsg);

   //  Have the specified POTS circuit process the message.
   //
   auto& pmsg = static_cast< Pots_NU_Message& >(msg);
   auto phi = pmsg.FindType< PotsHeaderInfo >(PotsParameter::Header);

   if(phi == nullptr)
   {
      DiscardMsg(msg, 0);
      return;
   }

   auto tsw = Singleton< Switch >::Instance();
   auto cct = static_cast< PotsCircuit* >(tsw->GetCircuit(phi->port));

   if(cct == nullptr)
   {
      DiscardMsg(msg, phi->port);
      return;
   }

   cct->ReceiveMsg(pmsg);
}

//------------------------------------------------------------------------------

fn_name PotsShelfFactory_ReallocOgMsg = "PotsShelfFactory.ReallocOgMsg";

Message* PotsShelfFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(PotsShelfFactory_ReallocOgMsg);

   return new Pots_UN_Message(buff);
}
}
