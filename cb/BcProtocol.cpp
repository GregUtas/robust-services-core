//==============================================================================
//
//  BcProtocol.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "BcProtocol.h"
#include "BcAddress.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "CliText.h"
#include "MediaParameter.h"
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "BcRouting.h"
#include "CfgParmRegistry.h"
#include "CliCommand.h"
#include "CliIntParm.h"
#include "CliTextParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "GlobalAddress.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "NbAppIds.h"
#include "RootServiceSM.h"
#include "SbAppIds.h"
#include "SbCliParms.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "TimerProtocol.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
class CipIamSignal : public CipSignal
{
   friend class Singleton< CipIamSignal >;

   CipIamSignal();
   ~CipIamSignal() = default;
   CliText* CreateText() const override;
};

class CipCpgSignal : public CipSignal
{
   friend class Singleton< CipCpgSignal >;

   CipCpgSignal();
   ~CipCpgSignal() = default;
   CliText* CreateText() const override;
};

class CipAnmSignal : public CipSignal
{
   friend class Singleton< CipAnmSignal >;

   CipAnmSignal();
   ~CipAnmSignal() = default;
   CliText* CreateText() const override;
};

class CipRelSignal : public CipSignal
{
   friend class Singleton< CipRelSignal >;

   CipRelSignal();
   ~CipRelSignal() = default;
   CliText* CreateText() const override;
};

class CipRouteParameter : public CipParameter
{
   friend class Singleton< CipRouteParameter >;

   CipRouteParameter();
   ~CipRouteParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
   void DisplayMsg(ostream& stream, const string& prefix,
      const byte_t* bytes, size_t count) const override;
   TestRc InjectMsg(CliThread& cli, Message& msg, Usage use) const override;
   TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};

class RouteParm : public CliText
{
public: RouteParm();
};

class CipAddressParameter : public AddressParameter
{
protected:
   explicit CipAddressParameter(Id pid);
   virtual ~CipAddressParameter() = default;
};

class CipCallingParameter : public CipAddressParameter
{
   friend class Singleton< CipCallingParameter >;

   CipCallingParameter();
   ~CipCallingParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
};

class CipCalledParameter : public CipAddressParameter
{
   friend class Singleton< CipCalledParameter >;

   CipCalledParameter();
   ~CipCalledParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
};

class CipOriginalCallingParameter : public CipAddressParameter
{
   friend class Singleton< CipOriginalCallingParameter >;

   CipOriginalCallingParameter();
   ~CipOriginalCallingParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
};

class CipOriginalCalledParameter : public CipAddressParameter
{
   friend class Singleton< CipOriginalCalledParameter >;

   CipOriginalCalledParameter();
   ~CipOriginalCalledParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
};

class CipProgressParameter : public ProgressParameter
{
   friend class Singleton< CipProgressParameter >;

   CipProgressParameter();
   ~CipProgressParameter() = default;
};

class CipCauseParameter : public CauseParameter
{
   friend class Singleton< CipCauseParameter >;

   CipCauseParameter();
   ~CipCauseParameter() = default;
};

class CipMediaParameter : public MediaParameter
{
   friend class Singleton< CipMediaParameter >;

   CipMediaParameter();
   ~CipMediaParameter() = default;
};

//==============================================================================

fixed_string CipUdpPortKey = "CipUdpPort";
fixed_string CipUdpPortExpl = "Call Interworking Protocol: UDP port";

CipUdpService::CipUdpService()
{
   Debug::ft("CipUdpService.ctor");

   auto port = std::to_string(CipIpPort);
   portCfg_.reset(new IpPortCfgParm
      (CipUdpPortKey, port.c_str(), CipUdpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*portCfg_);
}

//------------------------------------------------------------------------------

CipUdpService::~CipUdpService()
{
   Debug::ftnt("CipUdpService.dtor");
}

//------------------------------------------------------------------------------

InputHandler* CipUdpService::CreateHandler(IpPort* port) const
{
   Debug::ft("CipUdpService.CreateHandler");

   return new CipHandler(port);
}

//------------------------------------------------------------------------------

fixed_string CipUdpServiceStr = "CIP/UDP";
fixed_string CipUdpServiceExpl = "Call Interworking Protocol";

CliText* CipUdpService::CreateText() const
{
   Debug::ft("CipUdpService.CreateText");

   return new CliText(CipUdpServiceExpl, CipUdpServiceStr);
}

//==============================================================================

fixed_string CipTcpPortKey = "CipTcpPort";
fixed_string CipTcpPortExpl = "Call Interworking Protocol: TCP port";

CipTcpService::CipTcpService()
{
   Debug::ft("CipTcpService.ctor");

   auto port = std::to_string(CipIpPort);
   portCfg_.reset(new IpPortCfgParm
      (CipTcpPortKey, port.c_str(), CipTcpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*portCfg_);
}

//------------------------------------------------------------------------------

CipTcpService::~CipTcpService()
{
   Debug::ftnt("CipTcpService.dtor");
}

//------------------------------------------------------------------------------

InputHandler* CipTcpService::CreateHandler(IpPort* port) const
{
   Debug::ft("CipTcpService.CreateHandler");

   return new CipHandler(port);
}

//------------------------------------------------------------------------------

fixed_string CipTcpServiceStr = "CIP/TCP";
fixed_string CipTcpServiceExpl = "Call Interworking Protocol";

CliText* CipTcpService::CreateText() const
{
   Debug::ft("CipTcpService.CreateText");

   return new CliText(CipTcpServiceExpl, CipTcpServiceStr);
}

//------------------------------------------------------------------------------

void CipTcpService::GetAppSocketSizes(size_t& rxSize, size_t& txSize) const
{
   Debug::ft("CipTcpService.GetAppSocketSizes");

   //  Setting txSize to 0 prevents buffering of outgoing messages.
   //
   rxSize = 2048;
   txSize = 0;
}

//==============================================================================

CipProtocol::CipProtocol() : TlvProtocol(CipProtocolId, TimerProtocolId)
{
   Debug::ft("CipProtocol.ctor");

   //  Create the CIP signals and parameters.
   //
   Singleton< CipIamSignal >::Instance();
   Singleton< CipCpgSignal >::Instance();
   Singleton< CipAnmSignal >::Instance();
   Singleton< CipRelSignal >::Instance();

   Singleton< CipRouteParameter >::Instance();
   Singleton< CipCallingParameter >::Instance();
   Singleton< CipCalledParameter >::Instance();
   Singleton< CipOriginalCallingParameter >::Instance();
   Singleton< CipOriginalCalledParameter >::Instance();
   Singleton< CipProgressParameter >::Instance();
   Singleton< CipCauseParameter >::Instance();
   Singleton< CipMediaParameter >::Instance();
}

//------------------------------------------------------------------------------

CipProtocol::~CipProtocol()
{
   Debug::ftnt("CipProtocol.dtor");
}

//==============================================================================

CipSignal::CipSignal(Id sid) : Signal(CipProtocolId, sid) { }

//------------------------------------------------------------------------------

CipIamSignal::CipIamSignal() : CipSignal(IAM) { }

fixed_string IamTextStr = "I";
fixed_string IamTextExpl = "IAM";

CliText* CipIamSignal::CreateText() const
{
   return new CliText(IamTextExpl, IamTextStr);
}

//------------------------------------------------------------------------------

CipCpgSignal::CipCpgSignal() : CipSignal(CPG) { }

fixed_string CpgTextStr = "C";
fixed_string CpgTextExpl = "CPG";

CliText* CipCpgSignal::CreateText() const
{
   return new CliText(CpgTextExpl, CpgTextStr);
}

//------------------------------------------------------------------------------

CipAnmSignal::CipAnmSignal() : CipSignal(ANM) { }

fixed_string AnmTextStr = "A";
fixed_string AnmTextExpl = "ANM";

CliText* CipAnmSignal::CreateText() const
{
   return new CliText(AnmTextExpl, AnmTextStr);
}

//------------------------------------------------------------------------------

CipRelSignal::CipRelSignal() : CipSignal(REL) { }

fixed_string RelTextStr = "R";
fixed_string RelTextExpl = "REL";

CliText* CipRelSignal::CreateText() const
{
   return new CliText(RelTextExpl, RelTextStr);
}

//==============================================================================

CipParameter::CipParameter(Id pid) : TlvParameter(CipProtocolId, pid) { }

//==============================================================================

CipRouteParameter::CipRouteParameter() : CipParameter(Route)
{
   BindUsage(CipSignal::IAM, Mandatory);
}

//------------------------------------------------------------------------------

CliParm* CipRouteParameter::CreateCliParm(Usage use) const
{
   return new RouteParm;
}

//------------------------------------------------------------------------------

void CipRouteParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const RouteResult* >(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

Parameter::TestRc CipRouteParameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft("CipRouteParameter.InjectMsg");

   id_t        idx;
   word        fid, rid;
   RouteResult route;
   auto&       tlvmsg = static_cast< TlvMessage& >(msg);

   //  All fields in this parameter are mandatory.
   //
   if(!cli.Command()->GetTextIndex(idx, cli)) return StreamMissingMandatoryParm;
   if(!cli.Command()->GetIntParm(fid, cli)) return StreamMissingMandatoryParm;
   if(!cli.Command()->GetIntParm(rid, cli)) return StreamMissingMandatoryParm;

   route.selector = fid;
   route.identifier = rid;

   if(tlvmsg.AddType(route, Pid()) == nullptr)
   {
      *cli.obuf << ParameterNotAdded << CRLF;
      return MessageFailedToAddParm;
   }

   return Ok;
}

//------------------------------------------------------------------------------

Parameter::TestRc CipRouteParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft("CipRouteParameter.VerifyMsg");

   TestRc       rc;
   auto&        tlvmsg = static_cast< const TlvMessage& >(msg);
   RouteResult* route;
   id_t         idx;
   word         fid, rid;

   rc = tlvmsg.VerifyParm(Pid(), use, route);
   if(rc != Ok) return rc;
   if(use == Illegal) return Ok;

   //  The parameter is present.  All fields are mandatory.
   //
   if(!cli.Command()->GetTextIndex(idx, cli)) return StreamMissingMandatoryParm;
   if(!cli.Command()->GetIntParm(fid, cli)) return StreamMissingMandatoryParm;
   if(!cli.Command()->GetIntParm(rid, cli)) return StreamMissingMandatoryParm;

   if(route->selector != fid) return ParmValueMismatch;
   if(route->identifier != rid) return ParmValueMismatch;

   return Ok;
}

//------------------------------------------------------------------------------

fixed_string RouteSelExpl = "selector (FactoryId)";

fixed_string RouteIdExpl = "identifier (factory-specific)";

fixed_string RouteParmStr = "r";
fixed_string RouteParmExpl = "RouteResult";

RouteParm::RouteParm() : CliText(RouteParmExpl, RouteParmStr)
{
   BindParm(*new CliIntParm(RouteSelExpl, 0, Factory::MaxId));
   BindParm(*new CliIntParm(RouteIdExpl, WORD_MIN, WORD_MAX));
}

//==============================================================================

CipAddressParameter::CipAddressParameter(Id pid) :
   AddressParameter(CipProtocolId, pid) { }

//==============================================================================

CipCallingParameter::CipCallingParameter() :
   CipAddressParameter(CipParameter::Calling)
{
   BindUsage(CipSignal::IAM, Mandatory);
}

//------------------------------------------------------------------------------

fixed_string CallingExpl = "calling DN (digit string)";

CliParm* CipCallingParameter::CreateCliParm(Usage use) const
{
   return new CliTextParm(CallingExpl, false, 0);
}

//==============================================================================

CipCalledParameter::CipCalledParameter() :
   CipAddressParameter(CipParameter::Called)
{
   BindUsage(CipSignal::IAM, Mandatory);
}

//------------------------------------------------------------------------------

fixed_string CalledExpl = "called DN (digit string)";

CliParm* CipCalledParameter::CreateCliParm(Usage use) const
{
   return new CliTextParm(CalledExpl, false, 0);
}

//==============================================================================

CipOriginalCallingParameter::CipOriginalCallingParameter() :
   CipAddressParameter(CipParameter::OriginalCalling)
{
   BindUsage(CipSignal::IAM, Optional);
}

//------------------------------------------------------------------------------

fixed_string OriginalCallingExpl = "original calling DN (digit string)";
fixed_string OriginalCallingTag = "oclg";

CliParm* CipOriginalCallingParameter::CreateCliParm(Usage use) const
{
   return new CliTextParm(OriginalCallingExpl, true, 0, OriginalCallingTag);
}

//==============================================================================

CipOriginalCalledParameter::CipOriginalCalledParameter() :
   CipAddressParameter(CipParameter::OriginalCalled)
{
   BindUsage(CipSignal::IAM, Optional);
}

//------------------------------------------------------------------------------

fixed_string OriginalCalledExpl = "original called DN (digit string)";
fixed_string OriginalCalledTag = "ocld";

CliParm* CipOriginalCalledParameter::CreateCliParm(Usage use) const
{
   return new CliTextParm(OriginalCalledExpl, true, 0, OriginalCalledTag);
}

//==============================================================================

CipProgressParameter::CipProgressParameter() :
   ProgressParameter(CipProtocolId, CipParameter::Progress)
{
   BindUsage(CipSignal::CPG, Mandatory);
}

//==============================================================================

CipCauseParameter::CipCauseParameter() :
   CauseParameter(CipProtocolId, CipParameter::Cause)
{
   BindUsage(CipSignal::REL, Mandatory);
}

//==============================================================================

CipMediaParameter::CipMediaParameter() :
   MediaParameter(CipProtocolId, CipParameter::Media)
{
   BindUsage(CipSignal::IAM, Optional);
   BindUsage(CipSignal::CPG, Optional);
   BindUsage(CipSignal::ANM, Optional);
   BindUsage(CipSignal::REL, Optional);
}

//==============================================================================

CipMessage::CipMessage(SbIpBufferPtr& buff) : TlvMessage(buff)
{
   Debug::ft("CipMessage.ctor(i/c)");
}

//------------------------------------------------------------------------------

CipMessage::CipMessage(ProtocolSM* psm, size_t size) : TlvMessage(psm, size)
{
   Debug::ft("CipMessage.ctor(o/g)");
}

//------------------------------------------------------------------------------

CipMessage::~CipMessage()
{
   Debug::ftnt("CipMessage.dtor");
}

//------------------------------------------------------------------------------

DigitString* CipMessage::AddAddress(const DigitString& ds, CipParameter::Id pid)
{
   Debug::ft("CipMessage.AddAddress");

   return AddType(ds, pid);
}

//------------------------------------------------------------------------------

CauseInfo* CipMessage::AddCause(const CauseInfo& cause)
{
   Debug::ft("CipMessage.AddCause");

   return AddType(cause, CipParameter::Cause);
}

//------------------------------------------------------------------------------

MediaInfo* CipMessage::AddMedia(const MediaInfo& media)
{
   Debug::ft("CipMessage.AddMedia");

   return AddType(media, CipParameter::Media);
}

//------------------------------------------------------------------------------

ProgressInfo* CipMessage::AddProgress(const ProgressInfo& progress)
{
   Debug::ft("CipMessage.AddProgress");

   return AddType(progress, CipParameter::Progress);
}

//------------------------------------------------------------------------------

RouteResult* CipMessage::AddRoute(const RouteResult& route)
{
   Debug::ft("CipMessage.AddRoute");

   return AddType(route, CipParameter::Route);
}

//==============================================================================

BcPsm::BcPsm(FactoryId fid) : MediaPsm(fid),
   iamTimer_(false)
{
   Debug::ft("BcPsm.ctor(o/g)");
}

//------------------------------------------------------------------------------

BcPsm::BcPsm(FactoryId fid, ProtocolLayer& adj, bool upper) :
   MediaPsm(fid, adj, upper),
   iamTimer_(false)
{
   Debug::ft("BcPsm.ctor(subseq)");
}

//------------------------------------------------------------------------------

BcPsm::~BcPsm()
{
   Debug::ftnt("BcPsm.dtor");
}

//------------------------------------------------------------------------------

void BcPsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MediaPsm::Display(stream, prefix, options);

   stream << prefix << "iamTimer : " << iamTimer_ << CRLF;
}

//------------------------------------------------------------------------------

void BcPsm::EnsureMediaMsg()
{
   Debug::ft("BcPsm.EnsureMediaMsg");

   //  A media update can be included in any message, so an outgoing
   //  message only needs to be created if one doesn't already exist.
   //
   if((FirstOgMsg() == nullptr) && (GetState() != Idle))
   {
      auto msg = new CipMessage(this, 16);

      ProgressInfo cpi;

      msg->SetSignal(CipSignal::CPG);
      cpi.progress = Progress::MediaUpdate;
      msg->AddProgress(cpi);
   }
}

//------------------------------------------------------------------------------

CipMessage* BcPsm::FindRcvdMsg(CipSignal::Id sid) const
{
   Debug::ft("BcPsm.FindRcvdMsg");

   for(auto m = FirstRcvdMsg(); m != nullptr; m = m->NextMsg())
   {
      if(m->GetSignal() == sid) return static_cast< CipMessage* >(m);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void BcPsm::InjectFinalMsg()
{
   Debug::ft("BcPsm.InjectFinalMsg");

   auto msg = new CipMessage(this, 16);
   CauseInfo cci;

   msg->SetSignal(CipSignal::REL);
   cci.cause = Cause::TemporaryFailure;
   msg->AddCause(cci);
   msg->SendToSelf();
}

//------------------------------------------------------------------------------

fn_name BcPsm_ProcessIcMsg = "BcPsm.ProcessIcMsg";

ProtocolSM::IncomingRc BcPsm::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft(BcPsm_ProcessIcMsg);

   auto          state = GetState();
   auto&         tmsg = static_cast< TlvMessage& >(msg);
   auto          sig = tmsg.GetSignal();
   TimeoutInfo*  toi = nullptr;
   ProgressInfo* cpi = nullptr;
   auto          err = true;
   debug64_t     error;

   MediaPsm::UpdateIcMedia(tmsg, CipParameter::Media);

   switch(sig)
   {
   case Signal::Timeout:
      //
      //  The CIP PSM runs a timer while waiting for a response to an IAM.
      //
      toi = tmsg.FindType< TimeoutInfo >(Parameter::Timeout);

      if(toi->owner == this)
      {
         iamTimer_ = false;
         event = RootSsm()->RaiseProtocolError(*this, Timeout);
         return EventRaised;
      }

      err = false;
      break;

   case CipSignal::IAM:
      if(state == Idle)
      {
         SetState(IamRcvd);
         err = false;
      }
      break;

   case CipSignal::CPG:
      cpi = tmsg.FindType< ProgressInfo >(CipParameter::Progress);

      switch(cpi->progress)
      {
      case Progress::EndOfSelection:
         if(state == IamSent)
         {
            SetState(EosRcvd);
            err = false;
         }
         break;

      case Progress::Alerting:
         switch(state)
         {
         case IamSent:
         case EosRcvd:
            SetState(AltRcvd);
            err = false;
         }
         break;

      case Progress::Suspend:
         if(state == AnmRcvd)
         {
            SetState(SusRcvd);
            err = false;
         }
         break;

      case Progress::Resume:
         if(state == SusRcvd)
         {
            SetState(AnmRcvd);
            err = false;
         }
         break;

      case Progress::MediaUpdate:
         if(state != Idle) return DiscardMessage;
         break;
      }
      break;

   case CipSignal::ANM:
      switch(state)
      {
      case IamSent:
      case EosRcvd:
      case AltRcvd:
         SetState(AnmRcvd);
         err = false;
      }
      break;

   case CipSignal::REL:
      if(state != Idle)
      {
         SetState(Idle);
         err = false;
      }
   }

   if(iamTimer_)
   {
      StopTimer(*this, CipProtocol::IamTimeoutId);
      iamTimer_ = false;
   }

   if(err)
   {
      if(cpi == nullptr)
         error = pack2(0, sig);
      else
         error = pack2(cpi->progress, sig);

      Debug::SwLog(BcPsm_ProcessIcMsg, "unexpected signal", error);
      event = RootSsm()->RaiseProtocolError(*this, SignalInvalid);
      return EventRaised;
   }

   event = new AnalyzeMsgEvent(msg);
   return EventRaised;
}

//------------------------------------------------------------------------------

fn_name BcPsm_ProcessOgMsg = "BcPsm.ProcessOgMsg";

ProtocolSM::OutgoingRc BcPsm::ProcessOgMsg(Message& msg)
{
   Debug::ft(BcPsm_ProcessOgMsg);

   auto          state = GetState();
   auto&         tmsg = static_cast< TlvMessage& >(msg);
   auto          sig = msg.GetSignal();
   ProgressInfo* cpi = nullptr;
   auto          err = true;
   debug64_t     error;

   switch(sig)
   {
   case CipSignal::IAM:
      if(state == Idle)
      {
         SetState(IamSent);

         if(UsesIamTimer())
         {
            iamTimer_ = StartTimer
               (CipProtocol::IamTimeout, *this, CipProtocol::IamTimeoutId);
         }
         err = false;
      }
      break;

   case CipSignal::CPG:
      cpi = tmsg.FindType< ProgressInfo >(CipParameter::Progress);

      switch(cpi->progress)
      {
      case Progress::EndOfSelection:
         if(state == IamRcvd)
         {
            SetState(EosSent);
            if(Debug::SwFlagOn(CipIamTimeoutFlag)) return PurgeMessage;
            err = false;
         }
         break;

      case Progress::Alerting:
         switch(state)
         {
         case IamRcvd:
         case EosSent:
            SetState(AltSent);
            if(Debug::SwFlagOn(CipIamTimeoutFlag)) return PurgeMessage;
            err = false;
         }
         break;

      case Progress::Suspend:
         if(state == AnmSent)
         {
            SetState(SusSent);
            err = false;
         }
         break;

      case Progress::Resume:
         if(state == SusSent)
         {
            SetState(AnmSent);
            err = false;
         }
         break;

      case Progress::MediaUpdate:
         if(state != Idle)
         {
            if(msg.NextMsg() != nullptr) return PurgeMessage;
            err = false;
         }
         break;
      }
      break;

   case CipSignal::ANM:
      switch(state)
      {
      case IamRcvd:
      case EosSent:
      case AltSent:
         SetState(AnmSent);
         err = false;
      }
      break;

   case CipSignal::REL:
      if(state != Idle)
      {
         SetState(Idle);
         err = false;
      }
      break;
   }

   if(err)
   {
      if(cpi == nullptr)
         error = pack2(0, sig);
      else
         error = pack2(cpi->progress, sig);

      Debug::SwLog(BcPsm_ProcessOgMsg, "unexpected signal", error);
      return PurgeMessage;
   }

   MediaPsm::UpdateOgMedia(tmsg, CipParameter::Media);

   //d If this message is the first in a dialog, it must provide the
   //  source and destination addresses.
   //
   if(AddressesUnknown(&msg))
   {
      auto host = IpPortRegistry::HostAddress();
      auto peer = IpPortRegistry::HostAddress();
//s   auto cip = Singleton< CipUdpService >::Instance();
      auto cip = Singleton< CipTcpService >::Instance();
      GlobalAddress locAddr(host, cip->Port(), CipObcFactoryId);
      GlobalAddress remAddr(peer, cip->Port(), CipTbcFactoryId);

      msg.SetSender(locAddr);
      msg.SetReceiver(remAddr);
   }

   return SendMessage;
}

//------------------------------------------------------------------------------

void BcPsm::SendFinalMsg()
{
   Debug::ft("BcPsm.SendFinalMsg");

   if(GetState() == Idle) return;

   auto msg = new CipMessage(this, 16);
   CauseInfo cci;

   msg->SetSignal(CipSignal::REL);
   cci.cause = Cause::TemporaryFailure;
   msg->AddCause(cci);
   SendToLower(*msg);
}

//==============================================================================

CipPsm::CipPsm() : BcPsm(CipObcFactoryId)
{
   Debug::ft("CipPsm.ctor(IAM)");
}

//------------------------------------------------------------------------------

CipPsm::CipPsm(FactoryId fid, ProtocolLayer& adj, bool upper) :
   BcPsm(fid, adj, upper)
{
   Debug::ft("CipPsm.ctor(layer)");
}

//------------------------------------------------------------------------------

CipPsm::~CipPsm()
{
   Debug::ftnt("CipPsm.dtor");
}

//------------------------------------------------------------------------------

SysTcpSocket* CipPsm::CreateAppSocket()
{
   Debug::ft("CipPsm.CreateAppSocket");

   if(!Debug::SwFlagOn(CipAlwaysOverIpFlag)) return nullptr;

   auto reg = Singleton< IpPortRegistry >::Instance();
   auto port = reg->GetPort(CipIpPort, IpTcp);
   if(port == nullptr) return nullptr;
   return port->CreateAppSocket();
}

//------------------------------------------------------------------------------

Message::Route CipPsm::Route() const
{
   Debug::ft("CipPsm.Route");

   if(Debug::SwFlagOn(CipAlwaysOverIpFlag)) return Message::IpStack;
   return Message::Internal;
}

//==============================================================================

CipHandler::CipHandler(IpPort* port) : SbInputHandler(port)
{
   Debug::ft("CipHandler.ctor");
}

//------------------------------------------------------------------------------

CipHandler::~CipHandler()
{
   Debug::ftnt("CipHandler.dtor");
}

//==============================================================================

CipFactory::CipFactory(Id fid, c_string name) :
   SsmFactory(fid, CipProtocolId, name)
{
   Debug::ft("CipFactory.ctor");
}

//------------------------------------------------------------------------------

CipFactory::~CipFactory()
{
   Debug::ftnt("CipFactory.dtor");
}

//------------------------------------------------------------------------------

Message* CipFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("CipFactory.AllocIcMsg");

   return new CipMessage(buff);
}

//------------------------------------------------------------------------------

Message* CipFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft("CipFactory.AllocOgMsg");

   return new CipMessage(nullptr, 16);
}

//------------------------------------------------------------------------------

Message* CipFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft("CipFactory.ReallocOgMsg");

   return new CipMessage(buff);
}

//==============================================================================

CipObcFactory::CipObcFactory() :
   CipFactory(CipObcFactoryId, "Outgoing CIP Calls")
{
   Debug::ft("CipObcFactory.ctor");

   AddOutgoingSignal(CipSignal::IAM);
   AddOutgoingSignal(CipSignal::CPG);
   AddOutgoingSignal(CipSignal::REL);

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(CipSignal::CPG);
   AddIncomingSignal(CipSignal::ANM);
   AddIncomingSignal(CipSignal::REL);
}

//------------------------------------------------------------------------------

CipObcFactory::~CipObcFactory()
{
   Debug::ftnt("CipObcFactory.dtor");
}

//------------------------------------------------------------------------------

ProtocolSM* CipObcFactory::AllocOgPsm(const Message& msg) const
{
   Debug::ft("CipObcFactory.AllocOgPsm");

   return new CipPsm;
}

//------------------------------------------------------------------------------

fixed_string CipObcFactoryStr = "CO";
fixed_string CipObcFactoryExpl = "CIP Originator (network side)";

CliText* CipObcFactory::CreateText() const
{
   Debug::ft("CipObcFactory.CreateText");

   return new CliText(CipObcFactoryExpl, CipObcFactoryStr);
}

//==============================================================================

CipTbcFactory::CipTbcFactory() :
   CipFactory(CipTbcFactoryId, "Incoming CIP Calls")
{
   Debug::ft("CipTbcFactory.ctor");

   AddOutgoingSignal(CipSignal::CPG);
   AddOutgoingSignal(CipSignal::ANM);
   AddOutgoingSignal(CipSignal::REL);

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(CipSignal::IAM);
   AddIncomingSignal(CipSignal::CPG);
   AddIncomingSignal(CipSignal::REL);
}

//------------------------------------------------------------------------------

CipTbcFactory::~CipTbcFactory()
{
   Debug::ftnt("CipTbcFactory.dtor");
}

//------------------------------------------------------------------------------

ProtocolSM* CipTbcFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft("CipTbcFactory.AllocIcPsm");

   return new CipPsm(CipTbcFactoryId, lower, false);
}

//------------------------------------------------------------------------------

RootServiceSM* CipTbcFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft("CipTbcFactory.AllocRoot");

   auto& tmsg = static_cast< const CipMessage& >(msg);
   auto rte = tmsg.FindType< RouteResult >(CipParameter::Route);
   if(rte == nullptr) return nullptr;

   auto reg = Singleton< FactoryRegistry >::Instance();
   auto fac = static_cast< SsmFactory* >(reg->GetFactory(rte->selector));
   return fac->AllocRoot(msg, psm);
}

//------------------------------------------------------------------------------

fixed_string CipTbcFactoryStr = "CT";
fixed_string CipTbcFactoryExpl = "CIP Terminator (network side)";

CliText* CipTbcFactory::CreateText() const
{
   Debug::ft("CipTbcFactory.CreateText");

   return new CliText(CipTbcFactoryExpl, CipTbcFactoryStr);
}
}
