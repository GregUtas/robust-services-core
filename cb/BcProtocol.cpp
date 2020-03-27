//==============================================================================
//
//  BcProtocol.cpp
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
#include "BcProtocol.h"
#include "BcAddress.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "CliIntParm.h"
#include "CliText.h"
#include "CliTextParm.h"
#include "MediaParameter.h"
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "BcRouting.h"
#include "CfgParmRegistry.h"
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "GlobalAddress.h"
#include "IpPort.h"
#include "IpPortCfgParm.h"
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
private:
   CipIamSignal();
   CliText* CreateText() const override;
};

class IamText : public CliText
{
public: IamText();
};

class CipCpgSignal : public CipSignal
{
   friend class Singleton< CipCpgSignal >;
private:
   CipCpgSignal();
   CliText* CreateText() const override;
};

class CpgText : public CliText
{
public: CpgText();
};

class CipAnmSignal : public CipSignal
{
   friend class Singleton< CipAnmSignal >;
private:
   CipAnmSignal();
   CliText* CreateText() const override;
};

class AnmText : public CliText
{
public: AnmText();
};

class CipRelSignal : public CipSignal
{
   friend class Singleton< CipRelSignal >;
private:
   CipRelSignal();
   CliText* CreateText() const override;
};

class RelText : public CliText
{
public: RelText();
};

class CipRouteParameter : public CipParameter
{
   friend class Singleton< CipRouteParameter >;
private:
   CipRouteParameter();
   void DisplayMsg(ostream& stream, const string& prefix,
      const byte_t* bytes, size_t count) const override;
   CliParm* CreateCliParm(Usage use) const override;
   TestRc InjectMsg(CliThread& cli, Message& msg, Usage use) const override;
   TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};

class RouteParm : public CliText
{
public: RouteParm();
};

class RouteSelParm : public CliIntParm
{
public: RouteSelParm();
};

class RouteIdParm : public CliIntParm
{
public: RouteIdParm();
};

class CipAddressParameter : public AddressParameter
{
protected:
   explicit CipAddressParameter(Id pid);
   virtual ~CipAddressParameter();
};

class CipCallingParameter : public CipAddressParameter
{
   friend class Singleton< CipCallingParameter >;
private:
   CipCallingParameter();
   CliParm* CreateCliParm(Usage use) const override;
};

class CipCalledParameter : public CipAddressParameter
{
   friend class Singleton< CipCalledParameter >;
private:
   CipCalledParameter();
   CliParm* CreateCliParm(Usage use) const override;
};

class CipOriginalCallingParameter : public CipAddressParameter
{
   friend class Singleton< CipOriginalCallingParameter >;
private:
   CipOriginalCallingParameter();
   CliParm* CreateCliParm(Usage use) const override;
};

class CipOriginalCalledParameter : public CipAddressParameter
{
   friend class Singleton< CipOriginalCalledParameter >;
private:
   CipOriginalCalledParameter();
   CliParm* CreateCliParm(Usage use) const override;
};

class CipProgressParameter : public ProgressParameter
{
   friend class Singleton< CipProgressParameter >;
private:
   CipProgressParameter();
};

class CipCauseParameter : public CauseParameter
{
   friend class Singleton< CipCauseParameter >;
private:
   CipCauseParameter();
};

class CipMediaParameter : public MediaParameter
{
   friend class Singleton< CipMediaParameter >;
private:
   CipMediaParameter();
};

//==============================================================================

class CipUdpServiceText : public CliText
{
public: CipUdpServiceText();
};

fixed_string CipUdpServiceStr = "CIP/UDP";
fixed_string CipUdpServiceExpl = "Call Interworking Protocol";

CipUdpServiceText::CipUdpServiceText() :
   CliText(CipUdpServiceExpl, CipUdpServiceStr) { }

//------------------------------------------------------------------------------

fixed_string CipUdpPortKey = "CipUdpPort";
fixed_string CipUdpPortExpl = "Call Interworking Protocol: UDP port";

fn_name CipUdpService_ctor = "CipUdpService.ctor";

CipUdpService::CipUdpService() : port_(NilIpPort)
{
   Debug::ft(CipUdpService_ctor);

   auto port = std::to_string(CipIpPort);
   cfgPort_.reset(new IpPortCfgParm
      (CipUdpPortKey, port.c_str(), &port_, CipUdpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*cfgPort_);
}

//------------------------------------------------------------------------------

fn_name CipUdpService_dtor = "CipUdpService.dtor";

CipUdpService::~CipUdpService()
{
   Debug::ft(CipUdpService_dtor);
}

//------------------------------------------------------------------------------

fn_name CipUdpService_CreateHandler = "CipUdpService.CreateHandler";

InputHandler* CipUdpService::CreateHandler(IpPort* port) const
{
   Debug::ft(CipUdpService_CreateHandler);

   return new CipHandler(port);
}

//------------------------------------------------------------------------------

fn_name CipUdpService_CreateText = "CipUdpService.CreateText";

CliText* CipUdpService::CreateText() const
{
   Debug::ft(CipUdpService_CreateText);

   return new CipUdpServiceText;
}

//==============================================================================

class CipTcpServiceText : public CliText
{
public: CipTcpServiceText();
};

fixed_string CipTcpServiceStr = "CIP/TCP";
fixed_string CipTcpServiceExpl = "Call Interworking Protocol";

CipTcpServiceText::CipTcpServiceText() :
   CliText(CipTcpServiceExpl, CipTcpServiceStr) { }

//------------------------------------------------------------------------------

fixed_string CipTcpPortKey = "CipTcpPort";
fixed_string CipTcpPortExpl = "Call Interworking Protocol: TCP port";

fn_name CipTcpService_ctor = "CipTcpService.ctor";

CipTcpService::CipTcpService() : port_(NilIpPort)
{
   Debug::ft(CipTcpService_ctor);

   auto port = std::to_string(CipIpPort);
   cfgPort_.reset(new IpPortCfgParm
      (CipTcpPortKey, port.c_str(), &port_, CipTcpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*cfgPort_);
}

//------------------------------------------------------------------------------

fn_name CipTcpService_dtor = "CipTcpService.dtor";

CipTcpService::~CipTcpService()
{
   Debug::ft(CipTcpService_dtor);
}

//------------------------------------------------------------------------------

fn_name CipTcpService_CreateHandler = "CipTcpService.CreateHandler";

InputHandler* CipTcpService::CreateHandler(IpPort* port) const
{
   Debug::ft(CipTcpService_CreateHandler);

   return new CipHandler(port);
}

//------------------------------------------------------------------------------

fn_name CipTcpService_CreateText = "CipTcpService.CreateText";

CliText* CipTcpService::CreateText() const
{
   Debug::ft(CipTcpService_CreateText);

   return new CipTcpServiceText;
}

//------------------------------------------------------------------------------

fn_name CipTcpService_GetAppSocketSizes = "CipTcpService.GetAppSocketSizes";

void CipTcpService::GetAppSocketSizes(size_t& rxSize, size_t& txSize) const
{
   Debug::ft(CipTcpService_GetAppSocketSizes);

   //  Setting txSize to 0 prevents buffering of outgoing messages.
   //
   rxSize = 2048;
   txSize = 0;
}

//==============================================================================

fn_name CipProtocol_ctor = "CipProtocol.ctor";

CipProtocol::CipProtocol() : TlvProtocol(CipProtocolId, TimerProtocolId)
{
   Debug::ft(CipProtocol_ctor);

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

fn_name CipProtocol_dtor = "CipProtocol.dtor";

CipProtocol::~CipProtocol()
{
   Debug::ft(CipProtocol_dtor);
}

//==============================================================================

CipSignal::CipSignal(Id sid) : Signal(CipProtocolId, sid) { }

//------------------------------------------------------------------------------

CipIamSignal::CipIamSignal() : CipSignal(IAM) { }

CliText* CipIamSignal::CreateText() const
{
   return new IamText;
}

fixed_string IamTextStr = "I";
fixed_string IamTextExpl = "IAM";

IamText::IamText() : CliText(IamTextExpl, IamTextStr) { }

//------------------------------------------------------------------------------

CipCpgSignal::CipCpgSignal() : CipSignal(CPG) { }

CliText* CipCpgSignal::CreateText() const
{
   return new CpgText;
}

fixed_string CpgTextStr = "C";
fixed_string CpgTextExpl = "CPG";

CpgText::CpgText() : CliText(CpgTextExpl, CpgTextStr) { }

//------------------------------------------------------------------------------

CipAnmSignal::CipAnmSignal() : CipSignal(ANM) { }

CliText* CipAnmSignal::CreateText() const
{
   return new AnmText;
}

fixed_string AnmTextStr = "A";
fixed_string AnmTextExpl = "ANM";

AnmText::AnmText() : CliText(AnmTextExpl, AnmTextStr) { }

//------------------------------------------------------------------------------

CipRelSignal::CipRelSignal() : CipSignal(REL) { }

CliText* CipRelSignal::CreateText() const
{
   return new RelText;
}

fixed_string RelTextStr = "R";
fixed_string RelTextExpl = "REL";

RelText::RelText() : CliText(RelTextExpl, RelTextStr) { }

//==============================================================================

CipParameter::CipParameter(Id pid) : TlvParameter(CipProtocolId, pid) { }

//==============================================================================

CipRouteParameter::CipRouteParameter() : CipParameter(Route)
{
   BindUsage(CipSignal::IAM, Mandatory);
}

//------------------------------------------------------------------------------

fixed_string RouteSelExpl = "selector (FactoryId)";

RouteSelParm::RouteSelParm() : CliIntParm(RouteSelExpl, 0, Factory::MaxId) { }

fixed_string RouteIdExpl = "identifier (factory-specific)";

RouteIdParm::RouteIdParm() : CliIntParm(RouteIdExpl, WORD_MIN, WORD_MAX) { }

fixed_string RouteParmStr = "r";
fixed_string RouteParmExpl = "RouteResult";

RouteParm::RouteParm() : CliText(RouteParmExpl, RouteParmStr)
{
   BindParm(*new RouteSelParm);
   BindParm(*new RouteIdParm);
}

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

fn_name CipRouteParameter_InjectMsg = "CipRouteParameter.InjectMsg";

Parameter::TestRc CipRouteParameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft(CipRouteParameter_InjectMsg);

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

fn_name CipRouteParameter_VerifyMsg = "CipRouteParameter.VerifyMsg";

Parameter::TestRc CipRouteParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(CipRouteParameter_VerifyMsg);

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

//==============================================================================

CipAddressParameter::CipAddressParameter(Id pid) :
   AddressParameter(CipProtocolId, pid) { }

//------------------------------------------------------------------------------

CipAddressParameter::~CipAddressParameter() = default;

//==============================================================================

CipCallingParameter::CipCallingParameter() :
   CipAddressParameter(CipParameter::Calling)
{
   BindUsage(CipSignal::IAM, Mandatory);
}

//------------------------------------------------------------------------------

class CallingParm : public CliTextParm
{
public: CallingParm();
};

fixed_string CallingExpl = "calling DN (digit string)";

CallingParm::CallingParm() : CliTextParm(CallingExpl) { }

CliParm* CipCallingParameter::CreateCliParm(Usage use) const
{
   return new CallingParm;
}

//==============================================================================

CipCalledParameter::CipCalledParameter() :
   CipAddressParameter(CipParameter::Called)
{
   BindUsage(CipSignal::IAM, Mandatory);
}

//------------------------------------------------------------------------------

class CalledParm : public CliTextParm
{
public: CalledParm();
};

fixed_string CalledExpl = "called DN (digit string)";

CalledParm::CalledParm() : CliTextParm(CalledExpl) { }

CliParm* CipCalledParameter::CreateCliParm(Usage use) const
{
   return new CalledParm;
}

//==============================================================================

CipOriginalCallingParameter::CipOriginalCallingParameter() :
   CipAddressParameter(CipParameter::OriginalCalling)
{
   BindUsage(CipSignal::IAM, Optional);
}

//------------------------------------------------------------------------------

class OriginalCallingParm : public CliTextParm
{
public: OriginalCallingParm();
};

fixed_string OriginalCallingExpl = "original calling DN (digit string)";
fixed_string OriginalCallingTag = "oclg";

OriginalCallingParm::OriginalCallingParm() :
   CliTextParm(OriginalCallingExpl, true, 32, OriginalCallingTag) { }

CliParm* CipOriginalCallingParameter::CreateCliParm(Usage use) const
{
   return new OriginalCallingParm;
}

//==============================================================================

CipOriginalCalledParameter::CipOriginalCalledParameter() :
   CipAddressParameter(CipParameter::OriginalCalled)
{
   BindUsage(CipSignal::IAM, Optional);
}

//------------------------------------------------------------------------------

class OriginalCalledParm : public CliTextParm
{
public: OriginalCalledParm();
};

fixed_string OriginalCalledExpl = "original called DN (digit string)";
fixed_string OriginalCalledTag = "ocld";

OriginalCalledParm::OriginalCalledParm() :
   CliTextParm(OriginalCalledExpl, true, 32, OriginalCalledTag) { }

CliParm* CipOriginalCalledParameter::CreateCliParm(Usage use) const
{
   return new OriginalCalledParm;
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

fn_name CipMessage_ctor1 = "CipMessage.ctor(i/c)";

CipMessage::CipMessage(SbIpBufferPtr& buff) : TlvMessage(buff)
{
   Debug::ft(CipMessage_ctor1);
}

//------------------------------------------------------------------------------

fn_name CipMessage_ctor2 = "CipMessage.ctor(o/g)";

CipMessage::CipMessage(ProtocolSM* psm, size_t size) : TlvMessage(psm, size)
{
   Debug::ft(CipMessage_ctor2);
}

//------------------------------------------------------------------------------

fn_name CipMessage_dtor = "CipMessage.dtor";

CipMessage::~CipMessage()
{
   Debug::ft(CipMessage_dtor);
}

//------------------------------------------------------------------------------

fn_name CipMessage_AddAddress = "CipMessage.AddAddress";

DigitString* CipMessage::AddAddress(const DigitString& ds, CipParameter::Id pid)
{
   Debug::ft(CipMessage_AddAddress);

   return AddType(ds, pid);
}

//------------------------------------------------------------------------------

fn_name CipMessage_AddCause = "CipMessage.AddCause";

CauseInfo* CipMessage::AddCause(const CauseInfo& cause)
{
   Debug::ft(CipMessage_AddCause);

   return AddType(cause, CipParameter::Cause);
}

//------------------------------------------------------------------------------

fn_name CipMessage_AddMedia = "CipMessage.AddMedia";

MediaInfo* CipMessage::AddMedia(const MediaInfo& media)
{
   Debug::ft(CipMessage_AddMedia);

   return AddType(media, CipParameter::Media);
}

//------------------------------------------------------------------------------

fn_name CipMessage_AddProgress = "CipMessage.AddProgress";

ProgressInfo* CipMessage::AddProgress(const ProgressInfo& progress)
{
   Debug::ft(CipMessage_AddProgress);

   return AddType(progress, CipParameter::Progress);
}

//------------------------------------------------------------------------------

fn_name CipMessage_AddRoute = "CipMessage.AddRoute";

RouteResult* CipMessage::AddRoute(const RouteResult& route)
{
   Debug::ft(CipMessage_AddRoute);

   return AddType(route, CipParameter::Route);
}

//==============================================================================

fn_name BcPsm_ctor1 = "BcPsm.ctor(o/g)";

BcPsm::BcPsm(FactoryId fid) : MediaPsm(fid),
   iamTimer_(false)
{
   Debug::ft(BcPsm_ctor1);
}

//------------------------------------------------------------------------------

fn_name BcPsm_ctor2 = "BcPsm.ctor(subseq)";

BcPsm::BcPsm(FactoryId fid, ProtocolLayer& adj, bool upper) :
   MediaPsm(fid, adj, upper),
   iamTimer_(false)
{
   Debug::ft(BcPsm_ctor2);
}

//------------------------------------------------------------------------------

fn_name BcPsm_dtor = "BcPsm.dtor";

BcPsm::~BcPsm()
{
   Debug::ft(BcPsm_dtor);
}

//------------------------------------------------------------------------------

void BcPsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MediaPsm::Display(stream, prefix, options);

   stream << prefix << "iamTimer : " << iamTimer_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name BcPsm_EnsureMediaMsg = "BcPsm.EnsureMediaMsg";

void BcPsm::EnsureMediaMsg()
{
   Debug::ft(BcPsm_EnsureMediaMsg);

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

fn_name BcPsm_FindRcvdMsg = "BcPsm.FindRcvdMsg";

CipMessage* BcPsm::FindRcvdMsg(CipSignal::Id sid) const
{
   Debug::ft(BcPsm_FindRcvdMsg);

   for(auto m = FirstRcvdMsg(); m != nullptr; m = m->NextMsg())
   {
      if(m->GetSignal() == sid) return static_cast< CipMessage* >(m);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name BcPsm_InjectFinalMsg = "BcPsm.InjectFinalMsg";

void BcPsm::InjectFinalMsg()
{
   Debug::ft(BcPsm_InjectFinalMsg);

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

      Debug::SwLog(BcPsm_ProcessIcMsg, error, state);
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

      Debug::SwLog(BcPsm_ProcessOgMsg, error, state);
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

fn_name BcPsm_SendFinalMsg = "BcPsm.SendFinalMsg";

void BcPsm::SendFinalMsg()
{
   Debug::ft(BcPsm_SendFinalMsg);

   if(GetState() == Idle) return;

   auto msg = new CipMessage(this, 16);
   CauseInfo cci;

   msg->SetSignal(CipSignal::REL);
   cci.cause = Cause::TemporaryFailure;
   msg->AddCause(cci);
   SendToLower(*msg);
}

//==============================================================================

fn_name CipPsm_ctor1 = "CipPsm.ctor(IAM)";

CipPsm::CipPsm() : BcPsm(CipObcFactoryId)
{
   Debug::ft(CipPsm_ctor1);
}

//------------------------------------------------------------------------------

fn_name CipPsm_ctor2 = "CipPsm.ctor(layer)";

CipPsm::CipPsm(FactoryId fid, ProtocolLayer& adj, bool upper) :
   BcPsm(fid, adj, upper)
{
   Debug::ft(CipPsm_ctor2);
}

//------------------------------------------------------------------------------

fn_name CipPsm_dtor = "CipPsm.dtor";

CipPsm::~CipPsm()
{
   Debug::ft(CipPsm_dtor);
}

//------------------------------------------------------------------------------

fn_name CipPsm_CreateAppSocket = "CipPsm.CreateAppSocket";

SysTcpSocket* CipPsm::CreateAppSocket()
{
   Debug::ft(CipPsm_CreateAppSocket);

   if(!Debug::SwFlagOn(CipAlwaysOverIpFlag)) return nullptr;

   auto reg = Singleton< IpPortRegistry >::Instance();
   auto port = reg->GetPort(CipIpPort, IpTcp);
   if(port == nullptr) return nullptr;
   return port->CreateAppSocket();
}

//------------------------------------------------------------------------------

fn_name CipPsm_Route = "CipPsm.Route";

Message::Route CipPsm::Route() const
{
   Debug::ft(CipPsm_Route);

   if(Debug::SwFlagOn(CipAlwaysOverIpFlag)) return Message::IpStack;
   return Message::Internal;
}

//==============================================================================

fn_name CipHandler_ctor = "CipHandler.ctor";

CipHandler::CipHandler(IpPort* port) : SbInputHandler(port)
{
   Debug::ft(CipHandler_ctor);
}

//------------------------------------------------------------------------------

fn_name CipHandler_dtor = "CipHandler.dtor";

CipHandler::~CipHandler()
{
   Debug::ft(CipHandler_dtor);
}

//==============================================================================

fn_name CipFactory_ctor = "CipFactory.ctor";

CipFactory::CipFactory(Id fid, c_string name) :
   SsmFactory(fid, CipProtocolId, name)
{
   Debug::ft(CipFactory_ctor);
}

//------------------------------------------------------------------------------

fn_name CipFactory_dtor = "CipFactory.dtor";

CipFactory::~CipFactory()
{
   Debug::ft(CipFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name CipFactory_AllocIcMsg = "CipFactory.AllocIcMsg";

Message* CipFactory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(CipFactory_AllocIcMsg);

   return new CipMessage(buff);
}

//------------------------------------------------------------------------------

fn_name CipFactory_AllocOgMsg = "CipFactory.AllocOgMsg";

Message* CipFactory::AllocOgMsg(SignalId sid) const
{
   Debug::ft(CipFactory_AllocOgMsg);

   return new CipMessage(nullptr, 16);
}

//------------------------------------------------------------------------------

fn_name CipFactory_ReallocOgMsg = "CipFactory.ReallocOgMsg";

Message* CipFactory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(CipFactory_ReallocOgMsg);

   return new CipMessage(buff);
}

//==============================================================================

class CipObcFactoryText : public CliText
{
public:
   CipObcFactoryText();
};

fixed_string CipObcFactoryStr = "CO";
fixed_string CipObcFactoryExpl = "CIP Originator (network side)";

CipObcFactoryText::CipObcFactoryText() :
   CliText(CipObcFactoryExpl, CipObcFactoryStr) { }

//------------------------------------------------------------------------------

fn_name CipObcFactory_ctor = "CipObcFactory.ctor";

CipObcFactory::CipObcFactory() :
   CipFactory(CipObcFactoryId, "Outgoing CIP Calls")
{
   Debug::ft(CipObcFactory_ctor);

   AddOutgoingSignal(CipSignal::IAM);
   AddOutgoingSignal(CipSignal::CPG);
   AddOutgoingSignal(CipSignal::REL);

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(CipSignal::CPG);
   AddIncomingSignal(CipSignal::ANM);
   AddIncomingSignal(CipSignal::REL);
}

//------------------------------------------------------------------------------

fn_name CipObcFactory_dtor = "CipObcFactory.dtor";

CipObcFactory::~CipObcFactory()
{
   Debug::ft(CipObcFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name CipObcFactory_AllocOgPsm = "CipObcFactory.AllocOgPsm";

ProtocolSM* CipObcFactory::AllocOgPsm(const Message& msg) const
{
   Debug::ft(CipObcFactory_AllocOgPsm);

   return new CipPsm;
}

//------------------------------------------------------------------------------

fn_name CipObcFactory_CreateText = "CipObcFactory.CreateText";

CliText* CipObcFactory::CreateText() const
{
   Debug::ft(CipObcFactory_CreateText);

   return new CipObcFactoryText;
}

//==============================================================================

class CipTbcFactoryText : public CliText
{
public:
   CipTbcFactoryText();
};

fixed_string CipTbcFactoryStr = "CT";
fixed_string CipTbcFactoryExpl = "CIP Terminator (network side)";

CipTbcFactoryText::CipTbcFactoryText() :
   CliText(CipTbcFactoryExpl, CipTbcFactoryStr) { }

//------------------------------------------------------------------------------

fn_name CipTbcFactory_ctor = "CipTbcFactory.ctor";

CipTbcFactory::CipTbcFactory() :
   CipFactory(CipTbcFactoryId, "Incoming CIP Calls")
{
   Debug::ft(CipTbcFactory_ctor);

   AddOutgoingSignal(CipSignal::CPG);
   AddOutgoingSignal(CipSignal::ANM);
   AddOutgoingSignal(CipSignal::REL);

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(CipSignal::IAM);
   AddIncomingSignal(CipSignal::CPG);
   AddIncomingSignal(CipSignal::REL);
}

//------------------------------------------------------------------------------

fn_name CipTbcFactory_dtor = "CipTbcFactory.dtor";

CipTbcFactory::~CipTbcFactory()
{
   Debug::ft(CipTbcFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name CipTbcFactory_AllocIcPsm = "CipTbcFactory.AllocIcPsm";

ProtocolSM* CipTbcFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft(CipTbcFactory_AllocIcPsm);

   return new CipPsm(CipTbcFactoryId, lower, false);
}

//------------------------------------------------------------------------------

fn_name CipTbcFactory_AllocRoot = "CipTbcFactory.AllocRoot";

RootServiceSM* CipTbcFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft(CipTbcFactory_AllocRoot);

   auto& tmsg = static_cast< const CipMessage& >(msg);
   auto rte = tmsg.FindType< RouteResult >(CipParameter::Route);
   if(rte == nullptr) return nullptr;

   auto reg = Singleton< FactoryRegistry >::Instance();
   auto fac = static_cast< SsmFactory* >(reg->GetFactory(rte->selector));
   return fac->AllocRoot(msg, psm);
}

//------------------------------------------------------------------------------

fn_name CipTbcFactory_CreateText = "CipTbcFactory.CreateText";

CliText* CipTbcFactory::CreateText() const
{
   Debug::ft(CipTbcFactory_CreateText);

   return new CipTbcFactoryText;
}
}
