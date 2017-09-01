//==============================================================================
//
//  PotsCallFactory.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

using namespace MediaBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCallFactoryText : public CliText
{
public:
   PotsCallFactoryText();
};

fixed_string PotsCallFactoryStr = "PC";
fixed_string PotsCallFactoryExpl = "POTS Call (user side)";

PotsCallFactoryText::PotsCallFactoryText() :
   CliText(PotsCallFactoryExpl, PotsCallFactoryStr) { }

//------------------------------------------------------------------------------

fn_name PotsCallFactory_ctor = "PotsCallFactory.ctor";

PotsCallFactory::PotsCallFactory() :
   BcFactory(PotsCallFactoryId, PotsProtocolId, "POTS Basic Call")
{
   Debug::ft(PotsCallFactory_ctor);

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

fn_name PotsCallFactory_dtor = "PotsCallFactory.dtor";

PotsCallFactory::~PotsCallFactory()
{
   Debug::ft(PotsCallFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_AllocIcMsg = "PotsCallFactory.AllocIcMsg";

Message* PotsCallFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(PotsCallFactory_AllocIcMsg);

   return new Pots_UN_Message(buff);
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_AllocIcPsm = "PotsCallFactory.AllocIcPsm";

ProtocolSM* PotsCallFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft(PotsCallFactory_AllocIcPsm);

   auto& pmsg = static_cast< const Pots_UN_Message& >(msg);
   auto phi = pmsg.FindType< PotsHeaderInfo >(PotsParameter::Header);

   return new PotsCallPsm(lower, false, phi->port);
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_AllocOgMsg = "PotsCallFactory.AllocOgMsg";

Message* PotsCallFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft(PotsCallFactory_AllocOgMsg);

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
      Debug::SwErr(PotsCallFactory_AllocRoot, fid, 0);
   }

   return root;
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_CreateText = "PotsCallFactory.CreateText";

CliText* PotsCallFactory::CreateText() const
{
   Debug::ft(PotsCallFactory_CreateText);

   return new PotsCallFactoryText;
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_PortAllocated = "PotsCallFactory.PortAllocated";

void PotsCallFactory::PortAllocated
   (const MsgPort& port, const Message* msg) const
{
   Debug::ft(PotsCallFactory_PortAllocated);

   //  Record this port's address in the user's profile.  This will allow
   //  subsequent messages to be routed to the same context, even if it is
   //  still on the ingress work queue.
   //
   PotsCircuit* cct = nullptr;
   auto tsw = Singleton< Switch >::Instance();

   if(msg != nullptr)
   {
      auto pmsg = static_cast< const PotsMessage* >(msg);
      auto phi = pmsg->FindType< PotsHeaderInfo >(PotsParameter::Header);
      cct = static_cast< PotsCircuit* >(tsw->GetCircuit(phi->port));
   }
   else
   {
      auto ppsm = static_cast < const PotsCallPsm* >(port.Upper());
      cct = static_cast< PotsCircuit* >(tsw->GetCircuit(ppsm->TsPort()));
   }

   auto prof = cct->Profile();
   prof->SetObjAddr(port);
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_ReallocOgMsg = "PotsCallFactory.ReallocOgMsg";

Message* PotsCallFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(PotsCallFactory_ReallocOgMsg);

   return new Pots_NU_Message(buff);
}

//------------------------------------------------------------------------------

fn_name PotsCallFactory_ScreenIcMsgs = "PotsCallFactory.ScreenIcMsgs";

bool PotsCallFactory::ScreenIcMsgs(Q1Way< Message >& msgq)
{
   Debug::ft(PotsCallFactory_ScreenIcMsgs);

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

fn_name PotsCallFactory_SendRelease = "PotsCallFactory.SendRelease";

void PotsCallFactory::SendRelease(const Message& msg1)
{
   Debug::ft(PotsCallFactory_SendRelease);

   auto& icmsg = static_cast< const PotsMessage& >(msg1);
   auto icphi = icmsg.FindType< PotsHeaderInfo >(PotsParameter::Header);
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

fn_name PotsCallFactory_VerifyRoute = "PotsCallFactory.VerifyRoute";

Cause::Ind PotsCallFactory::VerifyRoute(RouteResult::Id rid) const
{
   Debug::ft(PotsCallFactory_VerifyRoute);

   //  There is no point in sending a CIP IAM if the destination DN is not
   //  registered.
   //
   if(Singleton< PotsProfileRegistry >::Instance()->Profile(rid) == nullptr)
   {
      return Cause::UnallocatedNumber;
   }

   return Cause::NilInd;
}
}
