//==============================================================================
//
//  PotsProtocol.cpp
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
#include "PotsProtocol.h"
#include "BcAddress.h"
#include "BcProgress.h"
#include "CliBoolParm.h"
#include "CliIntParm.h"
#include "CliText.h"
#include "CliTextParm.h"
#include "MediaParameter.h"
#include <cstddef>
#include <sstream>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "PotsCliParms.h"
#include "SbAppIds.h"
#include "SbCliParms.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Classes for individual POTS signals.
//
class PotsAlertingSignal : public PotsSignal
{
   friend class Singleton< PotsAlertingSignal >;
private:
   PotsAlertingSignal();
   virtual CliText* CreateText() const override;
};

class AlertingText : public CliText
{
public: AlertingText();
};

class PotsDigitsSignal : public PotsSignal
{
   friend class Singleton< PotsDigitsSignal >;
private:
   PotsDigitsSignal();
   virtual CliText* CreateText() const override;
};

class DigitsText : public CliText
{
public: DigitsText();
};

class PotsFacilitySignal : public PotsSignal
{
   friend class Singleton< PotsFacilitySignal >;
private:
   PotsFacilitySignal();
   virtual CliText* CreateText() const override;
};

class FacilityText : public CliText
{
public: FacilityText();
};

class PotsFlashSignal : public PotsSignal
{
   friend class Singleton< PotsFlashSignal >;
private:
   PotsFlashSignal();
   virtual CliText* CreateText() const override;
};

class FlashText : public CliText
{
public: FlashText();
};

class PotsReleaseSignal : public PotsSignal
{
   friend class Singleton< PotsReleaseSignal >;
private:
   PotsReleaseSignal();
   virtual CliText* CreateText() const override;
};

class ReleaseText : public CliText
{
public: ReleaseText();
};

class PotsLockoutSignal : public PotsSignal
{
   friend class Singleton< PotsLockoutSignal >;
private:
   PotsLockoutSignal();
   virtual CliText* CreateText() const override;
};

class LockoutText : public CliText
{
public: LockoutText();
};

class PotsOffhookSignal : public PotsSignal
{
   friend class Singleton< PotsOffhookSignal >;
private:
   PotsOffhookSignal();
   virtual CliText* CreateText() const override;
};

class PotsOnhookSignal : public PotsSignal
{
   friend class Singleton< PotsOnhookSignal >;
private:
   PotsOnhookSignal();
   virtual CliText* CreateText() const override;
};

class OffhookText : public CliText
{
public: OffhookText();
};

class OnhookText : public CliText
{
public: OnhookText();
};

class PotsProgressSignal : public PotsSignal
{
   friend class Singleton< PotsProgressSignal >;
private:
   PotsProgressSignal();
   virtual CliText* CreateText() const override;
};

class ProgressText : public CliText
{
public: ProgressText();
};

class PotsSuperviseSignal : public PotsSignal
{
   friend class Singleton< PotsSuperviseSignal >;
private:
   PotsSuperviseSignal();
   virtual CliText* CreateText() const override;
};

class SuperviseText : public CliText
{
public: SuperviseText();
};

//  Classes for individual POTS parameters.
//
class PotsCauseParameter : public CauseParameter
{
   friend class Singleton< PotsCauseParameter >;
private:
   PotsCauseParameter();
};

class PotsDigitsParameter : public AddressParameter
{
   friend class Singleton< PotsDigitsParameter >;
private:
   PotsDigitsParameter();
   virtual CliParm* CreateCliParm(Usage use) const override;
};

class DigitsParm : public CliTextParm
{
public: DigitsParm();
};

class PotsFacilityParameter : public PotsParameter
{
   friend class Singleton< PotsFacilityParameter >;
private:
   virtual void DisplayMsg(ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;
   virtual CliParm* CreateCliParm(Usage use) const override;
   virtual TestRc InjectMsg
      (CliThread& cli, Message& msg, Usage use) const override;
   virtual TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
   PotsFacilityParameter();
};

class FacilityMandParm : public CliText
{
public: FacilityMandParm();
};

class FacilityOptParm : public CliText
{
public: FacilityOptParm();
};

class ServiceIdParm : public CliIntParm
{
public: ServiceIdParm();
};

class FacilityIndParm : public CliIntParm
{
public: FacilityIndParm();
};

class PotsHeaderParameter : public PotsParameter
{
   friend class Singleton< PotsHeaderParameter >;
private:
   virtual void DisplayMsg(ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;
   virtual CliParm* CreateCliParm(Usage use) const override;
   virtual TestRc InjectMsg
      (CliThread& cli, Message& msg, Usage use) const override;
   virtual TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
   PotsHeaderParameter();
};

class HeaderParm : public CliIntParm
{
public: HeaderParm();
};

class PotsMediaParameter : public MediaParameter
{
   friend class Singleton< PotsMediaParameter >;
private:
   PotsMediaParameter();
};

class PotsProgressParameter : public ProgressParameter
{
   friend class Singleton< PotsProgressParameter >;
private:
   PotsProgressParameter();
};

class PotsRingParameter : public PotsParameter
{
   friend class Singleton< PotsRingParameter >;
private:
   virtual void DisplayMsg(ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;
   virtual CliParm* CreateCliParm(Usage use) const override;
   virtual TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
   PotsRingParameter();
};

class RingParm : public CliBoolParm
{
public: RingParm();
};

class PotsScanParameter : public PotsParameter
{
   friend class Singleton< PotsScanParameter >;
private:
   virtual void DisplayMsg(ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;
   virtual CliParm* CreateCliParm(Usage use) const override;
   virtual TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
   PotsScanParameter();
};

class ScanParm : public CliTextParm
{
public: ScanParm();
};

//==============================================================================

fn_name PotsProtocol_ctor = "PotsProtocol.ctor";

PotsProtocol::PotsProtocol() : TlvProtocol(PotsProtocolId, TimerProtocolId)
{
   Debug::ft(PotsProtocol_ctor);

   //  Create POTS signals and parameters.
   //
   Singleton< PotsOffhookSignal >::Instance();
   Singleton< PotsDigitsSignal >::Instance();
   Singleton< PotsAlertingSignal >::Instance();
   Singleton< PotsFlashSignal >::Instance();
   Singleton< PotsOnhookSignal >::Instance();
   Singleton< PotsFacilitySignal >::Instance();
   Singleton< PotsProgressSignal >::Instance();
   Singleton< PotsSuperviseSignal >::Instance();
   Singleton< PotsLockoutSignal >::Instance();
   Singleton< PotsReleaseSignal >::Instance();

   Singleton< PotsHeaderParameter >::Instance();
   Singleton< PotsDigitsParameter >::Instance();
   Singleton< PotsRingParameter >::Instance();
   Singleton< PotsScanParameter >::Instance();
   Singleton< PotsMediaParameter >::Instance();
   Singleton< PotsCauseParameter >::Instance();
   Singleton< PotsProgressParameter >::Instance();
   Singleton< PotsFacilityParameter >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsProtocol_dtor = "PotsProtocol.dtor";

PotsProtocol::~PotsProtocol()
{
   Debug::ft(PotsProtocol_dtor);
}

//==============================================================================

PotsSignal::PotsSignal(Id sid) : Signal(PotsProtocolId, sid) { }

PotsSignal::~PotsSignal() { }

//------------------------------------------------------------------------------

PotsAlertingSignal::PotsAlertingSignal() : PotsSignal(Alerting) { }

CliText* PotsAlertingSignal::CreateText() const
{
   return new AlertingText;
}

fixed_string AlertingTextStr = "A";
fixed_string AlertingTextExpl = "alerting";

AlertingText::AlertingText() : CliText(AlertingTextExpl, AlertingTextStr) { }

//------------------------------------------------------------------------------

PotsDigitsSignal::PotsDigitsSignal() : PotsSignal(Digits) { }

CliText* PotsDigitsSignal::CreateText() const
{
   return new DigitsText;
}

fixed_string DigitsTextStr = "D";
fixed_string DigitsTextExpl = "digits";

DigitsText::DigitsText() : CliText(DigitsTextExpl, DigitsTextStr) { }

//------------------------------------------------------------------------------

PotsFacilitySignal::PotsFacilitySignal() : PotsSignal(Facility) { }

CliText* PotsFacilitySignal::CreateText() const
{
   return new FacilityText;
}

fixed_string FacilitySigStr = "F";
fixed_string FacilitySigExpl = "facility";

FacilityText::FacilityText() : CliText(FacilitySigExpl, FacilitySigStr) { }

//------------------------------------------------------------------------------

PotsFlashSignal::PotsFlashSignal() : PotsSignal(Flash) { }

CliText* PotsFlashSignal::CreateText() const
{
   return new FlashText;
}

fixed_string FlashTextStr = "L";
fixed_string FlashTextExpl = "flash ('link')";

FlashText::FlashText() : CliText(FlashTextExpl, FlashTextStr) { }

//------------------------------------------------------------------------------

PotsLockoutSignal::PotsLockoutSignal() : PotsSignal(Lockout) { }

CliText* PotsLockoutSignal::CreateText() const
{
   return new LockoutText;
}

fixed_string LockoutTextStr = "L";
fixed_string LockoutTextExpl = "lockout";

LockoutText::LockoutText() : CliText(LockoutTextExpl, LockoutTextStr) { }

//------------------------------------------------------------------------------

PotsOffhookSignal::PotsOffhookSignal() : PotsSignal(Offhook) { }

CliText* PotsOffhookSignal::CreateText() const
{
   return new OffhookText;
}

fixed_string OffhookTextStr = "B";
fixed_string OffhookTextExpl = "offhook ('begin')";

OffhookText::OffhookText() : CliText(OffhookTextExpl, OffhookTextStr) { }

//------------------------------------------------------------------------------

PotsOnhookSignal::PotsOnhookSignal() : PotsSignal(Onhook) { }

CliText* PotsOnhookSignal::CreateText() const
{
   return new OnhookText;
}

fixed_string OnhookTextStr = "E";
fixed_string OnhookTextExpl = "onhook ('end')";

OnhookText::OnhookText() : CliText(OnhookTextExpl, OnhookTextStr) { }

//------------------------------------------------------------------------------

PotsProgressSignal::PotsProgressSignal() : PotsSignal(Progress) { }

CliText* PotsProgressSignal::CreateText() const
{
   return new ProgressText;
}

fixed_string ProgressTextStr = "P";
fixed_string ProgressTextExpl = "progress";

ProgressText::ProgressText() : CliText(ProgressTextExpl, ProgressTextStr) { }

//------------------------------------------------------------------------------

PotsReleaseSignal::PotsReleaseSignal() : PotsSignal(Release) { }

CliText* PotsReleaseSignal::CreateText() const
{
   return new ReleaseText;
}

fixed_string ReleaseTextStr = "R";
fixed_string ReleaseTextExpl = "release";

ReleaseText::ReleaseText() : CliText(ReleaseTextExpl, ReleaseTextStr) { }

//------------------------------------------------------------------------------

PotsSuperviseSignal::PotsSuperviseSignal() : PotsSignal(Supervise) { }

CliText* PotsSuperviseSignal::CreateText() const
{
   return new SuperviseText;
}

fixed_string SuperviseTextStr = "S";
fixed_string SuperviseTextExpl = "supervise";

SuperviseText::SuperviseText() :
   CliText(SuperviseTextExpl, SuperviseTextStr) { }

//==============================================================================

PotsParameter::PotsParameter(Id pid) : TlvParameter(PotsProtocolId, pid) { }

//------------------------------------------------------------------------------

PotsParameter::~PotsParameter() { }

//==============================================================================

PotsCauseParameter::PotsCauseParameter() :
   CauseParameter(PotsProtocolId, PotsParameter::Cause)
{
   BindUsage(PotsSignal::Supervise, Optional);
   BindUsage(PotsSignal::Facility, Optional);
   BindUsage(PotsSignal::Release, Mandatory);
}

//==============================================================================

PotsDigitsParameter::PotsDigitsParameter() :
   AddressParameter(PotsProtocolId, PotsParameter::Digits)
{
   BindUsage(PotsSignal::Digits, Mandatory);
}

//------------------------------------------------------------------------------

fixed_string DigitsExpl = "digit string: (0..9|*|#)*";

DigitsParm::DigitsParm() : CliTextParm(DigitsExpl) { }

CliParm* PotsDigitsParameter::CreateCliParm(Usage use) const
{
   return new DigitsParm;
}

//==============================================================================

PotsFacilityParameter::PotsFacilityParameter() : PotsParameter(Facility)
{
   BindUsage(PotsSignal::Facility, Mandatory);
   BindUsage(PotsSignal::Supervise, Optional);
}

//------------------------------------------------------------------------------

fixed_string ServiceIdExpl = "sid: ServiceId";

ServiceIdParm::ServiceIdParm() :
   CliIntParm(ServiceIdExpl, 0, Service::MaxId) { }

fixed_string FacilityIndExpl = "ind: Facility::Ind";

FacilityIndParm::FacilityIndParm() :
   CliIntParm(FacilityIndExpl, 0, UINT8_MAX) { }

fixed_string FacilityParmStr = "f";
fixed_string FacilityParmExpl = "facility info";

FacilityMandParm::FacilityMandParm() :
   CliText(FacilityParmExpl, FacilityParmStr)
{
   BindParm(*new ServiceIdParm);
   BindParm(*new FacilityIndParm);
}

FacilityOptParm::FacilityOptParm() :
   CliText(FacilityParmExpl, FacilityParmStr, true)
{
   BindParm(*new ServiceIdParm);
   BindParm(*new FacilityIndParm);
}

CliParm* PotsFacilityParameter::CreateCliParm(Usage use) const
{
   if(use == Mandatory) return new FacilityMandParm;
   return new FacilityOptParm;
}

//------------------------------------------------------------------------------

void PotsFacilityParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const PotsFacilityInfo* >(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

fn_name PotsFacilityParameter_InjectMsg = "PotsFacilityParameter.InjectMsg";

Parameter::TestRc PotsFacilityParameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft(PotsFacilityParameter_InjectMsg);

   id_t             index;
   word             sid, ind;
   PotsFacilityInfo info;
   auto&            pmsg = static_cast< PotsMessage& >(msg);

   switch(cli.Command()->GetTextIndexRc(index, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      return Ok;
   case CliParm::Ok:
      break;
   default:
      if(use == Mandatory) return IllegalValueInStream;
      return Ok;
   }

   switch(cli.Command()->GetIntParmRc(sid, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      return Ok;
      break;
   case CliParm::Ok:
      break;
   default:
      return IllegalValueInStream;
   }

   info.sid = sid;

   switch(cli.Command()->GetIntParmRc(ind, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      return Ok;
      break;
   case CliParm::Ok:
      break;
   default:
      return IllegalValueInStream;
   }

   info.ind = ind;

   if(pmsg.AddFacility(info) == nullptr)
   {
      *cli.obuf << ParameterNotAdded << CRLF;
      return MessageFailedToAddParm;
   }

   return Ok;
}

//------------------------------------------------------------------------------

fn_name PotsFacilityParameter_VerifyMsg = "PotsFacilityParameter.VerifyMsg";

Parameter::TestRc PotsFacilityParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(PotsFacilityParameter_VerifyMsg);

   TestRc            rc;
   auto&             pmsg = static_cast< const PotsMessage& >(msg);
   PotsFacilityInfo* info;
   id_t              index;
   word              sid, ind;

   rc = pmsg.VerifyParm(PotsParameter::Facility, use, info);
   if(rc != Ok) return rc;
   if(use == Illegal) return Ok;

   //  Look for our parameter's string (FacilityParmStr, above).  It's an
   //  error if (a) we don't find it when it's mandatory or present in the
   //  message; (b) if we find it when it's not present in the message.
   //
   switch(cli.Command()->GetTextIndexRc(index, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      if(info != nullptr) return OptionalParmPresent;
      return Ok;
   case CliParm::Ok:
      if(info == nullptr) return OptionalParmMissing;
      break;
   default:
      if(use == Mandatory) return IllegalValueInStream;
      return Ok;
   }

   //  If we get here, we are verifying a parameter that is in the message.
   //  At this point, both fields are mandatory.
   //
   if(!cli.Command()->GetIntParm(sid, cli)) return StreamMissingMandatoryParm;
   if(!cli.Command()->GetIntParm(ind, cli)) return StreamMissingMandatoryParm;

   if(info->sid != sid) return ParmValueMismatch;
   if(info->ind != ind) return ParmValueMismatch;

   return Ok;
}

//------------------------------------------------------------------------------

fn_name PotsFacilityInfo_ctor = "PotsFacilityInfo.ctor";

PotsFacilityInfo::PotsFacilityInfo() :
   sid(NIL_ID),
   ind(Facility::NilInd)
{
   Debug::ft(PotsFacilityInfo_ctor);
}

//------------------------------------------------------------------------------

void PotsFacilityInfo::Display(ostream& stream, const string& prefix) const
{
   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);

   stream << prefix << "sid : " << sid;
   stream << " (" << strClass(svc, false) << ')' << CRLF;
   stream << prefix << "ind : " << int(ind) << CRLF;
}

//==============================================================================

PotsHeaderParameter::PotsHeaderParameter() : PotsParameter(Header)
{
   BindUsage(PotsSignal::Offhook, Mandatory);
   BindUsage(PotsSignal::Digits, Mandatory);
   BindUsage(PotsSignal::Alerting, Mandatory);
   BindUsage(PotsSignal::Flash, Mandatory);
   BindUsage(PotsSignal::Onhook, Mandatory);
   BindUsage(PotsSignal::Facility, Mandatory);
   BindUsage(PotsSignal::Progress, Mandatory);
   BindUsage(PotsSignal::Supervise, Mandatory);
   BindUsage(PotsSignal::Lockout, Mandatory);
   BindUsage(PotsSignal::Release, Mandatory);
}

//------------------------------------------------------------------------------

fixed_string HeaderParmExpl = "header.port: Switch::PortId";

HeaderParm::HeaderParm() : CliIntParm(HeaderParmExpl, 0, Switch::MaxPortId) { }

CliParm* PotsHeaderParameter::CreateCliParm(Usage use) const
{
   return new HeaderParm;
}

//------------------------------------------------------------------------------

void PotsHeaderParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const PotsHeaderInfo* >(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

fn_name PotsHeaderParameter_InjectMsg = "PotsHeaderParameter.InjectMsg";

Parameter::TestRc PotsHeaderParameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft(PotsHeaderParameter_InjectMsg);

   word           port;
   PotsHeaderInfo info;
   auto&          pmsg = static_cast< PotsMessage& >(msg);

   if(!cli.Command()->GetIntParm(port, cli)) return StreamMissingMandatoryParm;

   info.signal = pmsg.GetSignal();
   info.port = port;

   if(pmsg.AddHeader(info) == nullptr)
   {
      *cli.obuf << ParameterNotAdded << CRLF;
      return MessageFailedToAddParm;
   }

   return Ok;
}

//------------------------------------------------------------------------------

fn_name PotsHeaderParameter_VerifyMsg = "PotsHeaderParameter.VerifyMsg";

Parameter::TestRc PotsHeaderParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(PotsHeaderParameter_VerifyMsg);

   TestRc          rc;
   auto&           pmsg = static_cast< const PotsMessage& >(msg);
   word            port;
   PotsHeaderInfo* info;

   rc = pmsg.VerifyParm(PotsParameter::Header, use, info);
   if(rc != Ok) return rc;
   if(use == Illegal) return Ok;
   if(!cli.Command()->GetIntParm(port, cli)) return StreamMissingMandatoryParm;
   if(info->signal != pmsg.GetSignal()) return ParmValueMismatch;
   if(info->port != port) return ParmValueMismatch;
   return Ok;
}

//------------------------------------------------------------------------------

fn_name PotsHeaderInfo_ctor = "PotsHeaderInfo.ctor";

PotsHeaderInfo::PotsHeaderInfo() :
   signal(NIL_ID),
   port(NIL_ID)
{
   Debug::ft(PotsHeaderInfo_ctor);
}

//------------------------------------------------------------------------------

void PotsHeaderInfo::Display(ostream& stream, const string& prefix) const
{
   auto sig = Singleton< PotsProtocol >::Instance()->GetSignal(signal);
   auto cct = Singleton< Switch >::Instance()->CircuitName(port);

   stream << prefix << "signal : " << signal;
   stream << " (" << strClass(sig, false) << ')' << CRLF;
   stream << prefix << "port   : " << port;
   stream << " (" << cct << ')' << CRLF;
}

//==============================================================================

PotsMediaParameter::PotsMediaParameter() :
   MediaParameter(PotsProtocolId, PotsParameter::Media)
{
   BindUsage(PotsSignal::Offhook, Optional);
   BindUsage(PotsSignal::Facility, Optional);
   BindUsage(PotsSignal::Progress, Optional);
   BindUsage(PotsSignal::Supervise, Optional);
}

//==============================================================================

PotsProgressParameter::PotsProgressParameter() :
   ProgressParameter(PotsProtocolId, PotsParameter::Progress)
{
   BindUsage(PotsSignal::Facility, Optional);
   BindUsage(PotsSignal::Progress, Mandatory);
}

//==============================================================================

PotsRingParameter::PotsRingParameter() : PotsParameter(Ring)
{
   BindUsage(PotsSignal::Supervise, Optional);
}

//------------------------------------------------------------------------------

fixed_string RingParmExpl = "ring on?";
fixed_string RingTag = "r";

RingParm::RingParm() : CliBoolParm(RingParmExpl, true, RingTag) { }

CliParm* PotsRingParameter::CreateCliParm(Usage use) const
{
   return new RingParm;
}

//------------------------------------------------------------------------------

void PotsRingParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const PotsRingInfo* >(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

fn_name PotsRingParameter_VerifyMsg = "PotsRingParameter.VerifyMsg";

Parameter::TestRc PotsRingParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(PotsRingParameter_VerifyMsg);

   TestRc        rc;
   auto&         pmsg = static_cast< const PotsMessage& >(msg);
   PotsRingInfo* info;
   bool          ring = false;
   auto          exists = false;

   rc = pmsg.VerifyParm(PotsParameter::Ring, use, info);
   if(rc != Ok) return rc;
   if(use == Illegal) return Ok;

   switch(cli.Command()->GetBoolParmRc(ring, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      break;
   case CliParm::Ok:
      exists = true;
      break;
   default:
      if(use == Mandatory) return IllegalValueInStream;
      return Ok;
   }

   if(exists)
   {
      if(info == nullptr) return OptionalParmMissing;
      if(info->on != ring) return ParmValueMismatch;
   }
   else
   {
      if(info != nullptr) return OptionalParmPresent;
   }

   return Ok;
}

//------------------------------------------------------------------------------

fn_name PotsRingInfo_ctor = "PotsRingInfo.ctor";

PotsRingInfo::PotsRingInfo() : on(false)
{
   Debug::ft(PotsRingInfo_ctor);
}

//------------------------------------------------------------------------------

void PotsRingInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "on : " << on << CRLF;
}

//==============================================================================

PotsScanParameter::PotsScanParameter() : PotsParameter(Scan)
{
   BindUsage(PotsSignal::Supervise, Optional);
}

//------------------------------------------------------------------------------

fixed_string ScanParmExpl = "scan: (x|d|f|df)";
fixed_string ScanTag = "s";

ScanParm::ScanParm() : CliTextParm(ScanParmExpl, true, 0, ScanTag) { }

CliParm* PotsScanParameter::CreateCliParm(Usage use) const
{
   return new ScanParm;
}

//------------------------------------------------------------------------------

void PotsScanParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const PotsScanInfo* >(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

fn_name PotsScanParameter_VerifyMsg = "PotsScanParameter.VerifyMsg";

Parameter::TestRc PotsScanParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(PotsScanParameter_VerifyMsg);

   TestRc        rc;
   auto&         pmsg = static_cast< const PotsMessage& >(msg);
   PotsScanInfo* info;
   string        scan;
   auto          digits = false;
   auto          flash = false;
   auto          exists = false;

   rc = pmsg.VerifyParm(PotsParameter::Scan, use, info);
   if(rc != Ok) return rc;
   if(use == Illegal) return Ok;

   switch(cli.Command()->GetStringRc(scan, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      break;

   case CliParm::Ok:
      exists = true;

      for(size_t i = 0; i < scan.size(); ++i)
      {
         switch(scan[i])
         {
         case 'x':
            break;
         case 'd':
            digits = true;
            break;
         case 'f':
            flash = true;
            break;
         default:
            *cli.obuf << spaces(2) << IllegalScanChar << scan[i] << CRLF;
            return IllegalValueInStream;
         }
      }
      break;

   default:
      if(use == Mandatory) return IllegalValueInStream;
      return Ok;
   }

   if(exists)
   {
      if(info == nullptr) return OptionalParmMissing;
      if(info->digits != digits) return ParmValueMismatch;
      if(info->flash != flash) return ParmValueMismatch;
   }
   else
   {
      if(info != nullptr) return OptionalParmPresent;
   }

   return Ok;
}

//------------------------------------------------------------------------------

fn_name PotsScanInfo_ctor = "PotsScanInfo.ctor";

PotsScanInfo::PotsScanInfo() :
   digits(false),
   flash(false)
{
   Debug::ft(PotsScanInfo_ctor);
}

//------------------------------------------------------------------------------

void PotsScanInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "digits : " << digits << CRLF;
   stream << prefix << "flash  : " << flash << CRLF;
}
}
