//==============================================================================
//
//  PotsCallFactory.cpp
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
#include "PotsSessions.h"
#include "BcCause.h"
#include "CliText.h"
#include "Debug.h"
#include "LocalAddress.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "PotsCircuit.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "PotsProtocol.h"
#include "Q1Way.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "Switch.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Sends a Release message when discarding an offhook-onhook message pair.
//  MSG1 is the offhook message.
//
static void SendRelease(const Message& msg1)
{
   Debug::ft("PotsBase.SendRelease");

   const auto& icmsg = static_cast<const PotsMessage&>(msg1);
   auto icphi = icmsg.FindType<PotsHeaderInfo>(PotsParameter::Header);
   auto ogmsg = new Pots_NU_Message(nullptr, 20);

   ogmsg->SetSignal(PotsSignal::Release);

   PotsHeaderInfo ogphi;
   ogphi.port = icphi->port;
   ogphi.signal = PotsSignal::Release;
   ogmsg->AddHeader(ogphi);

   CauseInfo cause;
   cause.cause = Cause::NormalCallClearing;
   ogmsg->AddCause(cause);
   ogmsg->Send(Message::External);
}

//------------------------------------------------------------------------------

PotsCallFactory::PotsCallFactory() :
   BcFactory(PotsCallFactoryId, PotsProtocolId, "POTS Basic Call")
{
   Debug::ft("PotsCallFactory.ctor");

   AddOutgoingSignal(PotsSignal::Supervise);
   AddOutgoingSignal(PotsSignal::Lockout);
   AddOutgoingSignal(PotsSignal::Release);
   AddOutgoingSignal(PotsSignal::Facility);

   //  A user-side PSM in a basic call only receives Facility, Progress, and
   //  Release signals when a multiplexer has been inserted between the call
   //  and the POTS circuit.
   //
   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(PotsSignal::Offhook);
   AddIncomingSignal(PotsSignal::Alerting);
   AddIncomingSignal(PotsSignal::Digits);
   AddIncomingSignal(PotsSignal::Flash);
   AddIncomingSignal(PotsSignal::Onhook);
   AddIncomingSignal(PotsSignal::Facility);
   AddIncomingSignal(PotsSignal::Progress);
   AddIncomingSignal(PotsSignal::Release);
}

//------------------------------------------------------------------------------

PotsCallFactory::~PotsCallFactory()
{
   Debug::ftnt("PotsCallFactory.dtor");
}

//------------------------------------------------------------------------------

Message* PotsCallFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("PotsCallFactory.AllocIcMsg");

   return new Pots_UN_Message(buff);
}

//------------------------------------------------------------------------------

ProtocolSM* PotsCallFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft("PotsCallFactory.AllocIcPsm");

   const auto& pmsg = static_cast<const Pots_UN_Message&>(msg);
   auto phi = pmsg.FindType<PotsHeaderInfo>(PotsParameter::Header);

   return new PotsCallPsm(lower, false, phi->port);
}

//------------------------------------------------------------------------------

Message* PotsCallFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft("PotsCallFactory.AllocOgMsg");

   return new Pots_NU_Message(nullptr, 32);
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_AllocRoot = "PotsCallFactory.AllocRoot";

RootServiceSM* PotsCallFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft(PotsCallFactory_AllocRoot);

   PotsBcSsm* root = nullptr;

   //  MSG's receiving factory distinguishes whether a POTS subscriber is
   //  o originating or receiving a call: create a POTS SSM
   //  o redirecting a call: create a POTS proxy SSM
   //
   auto fid = msg.Header()->rxAddr.fid;

   switch(fid)
   {
   case PotsCallFactoryId:
   case CipTbcFactoryId:
      root = new PotsBcSsm(PotsCallServiceId, msg, psm);
      break;

   case ProxyCallFactoryId:
      root = new PotsBcSsm(PotsProxyServiceId, msg, psm);
      break;

   default:
      Debug::SwLog(PotsCallFactory_AllocRoot, "invalid FactoryId", fid);
   }

   return root;
}

//------------------------------------------------------------------------------

fixed_string PotsCallFactoryStr = "PC";
fixed_string PotsCallFactoryExpl = "POTS Call (user side)";

CliText* PotsCallFactory::CreateText() const
{
   Debug::ft("PotsCallFactory.CreateText");

   return new CliText(PotsCallFactoryExpl, PotsCallFactoryStr);
}

//------------------------------------------------------------------------------

void PotsCallFactory::PortAllocated
   (const MsgPort& port, const Message* msg) const
{
   Debug::ft("PotsCallFactory.PortAllocated");

   //  Record this port's address in the user's profile.  This will allow
   //  subsequent messages to be routed to the same context, even if it is
   //  still on the ingress work queue.
   //
   PotsCircuit* cct = nullptr;
   auto tsw = Singleton<Switch>::Instance();

   if(msg != nullptr)
   {
      auto pmsg = static_cast<const PotsMessage*>(msg);
      auto phi = pmsg->FindType<PotsHeaderInfo>(PotsParameter::Header);
      cct = static_cast<PotsCircuit*>(tsw->GetCircuit(phi->port));
   }
   else
   {
      auto ppsm = static_cast<const PotsCallPsm*>(port.Upper());
      cct = static_cast<PotsCircuit*>(tsw->GetCircuit(ppsm->TsPort()));
   }

   auto prof = cct->Profile();
   prof->SetObjAddr(port);
}

//------------------------------------------------------------------------------

Message* PotsCallFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("PotsCallFactory.ReallocOgMsg");

   return new Pots_NU_Message(buff);
}

//------------------------------------------------------------------------------

bool PotsCallFactory::ScreenFirstMsg
   (const Message& msg, MsgPriority& prio) const
{
   Debug::ft("PotsCallFactory.ScreenFirstMsg");

   return true;
}

//------------------------------------------------------------------------------

bool PotsCallFactory::ScreenIcMsgs(Q1Way<Message>& msgq)
{
   Debug::ft("PotsCallFactory.ScreenIcMsgs");

   auto msg1 = msgq.First();

   if(msg1->GetSignal() == PotsSignal::Offhook)
   {
      auto msg2 = msg1->NextMsg();

      switch(msg2->GetSignal())
      {
      case PotsSignal::Onhook:
         //
         //  An offhook followed by an onhook is a noop, so delete the context
         //  after sending a release.
         //
         SendRelease(*msg1);
         RecordDeletion(true);
         return false;

      case PotsSignal::Offhook:
         //
         //  Discard a retransmitted offhook.
         //
         RecordDeletion(false);
         delete msg2;
      }
   }

   return true;
}

//------------------------------------------------------------------------------

Cause::Ind PotsCallFactory::VerifyRoute(RouteResult::Id rid) const
{
   Debug::ft("PotsCallFactory.VerifyRoute");

   //  There is no point in sending a CIP IAM if the destination DN is not
   //  registered.
   //
   if(Singleton<PotsProfileRegistry>::Instance()->Profile(rid) == nullptr)
   {
      return Cause::UnallocatedNumber;
   }

   return Cause::NilInd;
}
}
