//==============================================================================
//
//  PotsProtocol.cpp
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
#include "PotsProtocol.h"
#include "BcAddress.h"
#include "BcProgress.h"
#include "CliText.h"
#include "MediaParameter.h"
#include <sstream>
#include "CliBoolParm.h"
#include "CliCommand.h"
#include "CliIntParm.h"
#include "CliTextParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "PotsCliParms.h"
#include "Registry.h"
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
   friend class Singleton<PotsAlertingSignal>;

   PotsAlertingSignal();
   ~PotsAlertingSignal() = default;
   CliText* CreateText() const override;
};

class PotsDigitsSignal : public PotsSignal
{
   friend class Singleton<PotsDigitsSignal>;

   PotsDigitsSignal();
   ~PotsDigitsSignal() = default;
   CliText* CreateText() const override;
};

class PotsFacilitySignal : public PotsSignal
{
   friend class Singleton<PotsFacilitySignal>;

   PotsFacilitySignal();
   ~PotsFacilitySignal() = default;
   CliText* CreateText() const override;
};

class PotsFlashSignal : public PotsSignal
{
   friend class Singleton<PotsFlashSignal>;

   PotsFlashSignal();
   ~PotsFlashSignal() = default;
   CliText* CreateText() const override;
};

class PotsReleaseSignal : public PotsSignal
{
   friend class Singleton<PotsReleaseSignal>;

   PotsReleaseSignal();
   ~PotsReleaseSignal() = default;
   CliText* CreateText() const override;
};

class PotsLockoutSignal : public PotsSignal
{
   friend class Singleton<PotsLockoutSignal>;

   PotsLockoutSignal();
   ~PotsLockoutSignal() = default;
   CliText* CreateText() const override;
};

class PotsOffhookSignal : public PotsSignal
{
   friend class Singleton<PotsOffhookSignal>;

   PotsOffhookSignal();
   ~PotsOffhookSignal() = default;
   CliText* CreateText() const override;
};

class PotsOnhookSignal : public PotsSignal
{
   friend class Singleton<PotsOnhookSignal>;

   PotsOnhookSignal();
   ~PotsOnhookSignal() = default;
   CliText* CreateText() const override;
};

class PotsProgressSignal : public PotsSignal
{
   friend class Singleton<PotsProgressSignal>;

   PotsProgressSignal();
   ~PotsProgressSignal() = default;
   CliText* CreateText() const override;
};

class PotsSuperviseSignal : public PotsSignal
{
   friend class Singleton<PotsSuperviseSignal>;

   PotsSuperviseSignal();
   ~PotsSuperviseSignal() = default;
   CliText* CreateText() const override;
};

//  Classes for individual POTS parameters.
//
class PotsCauseParameter : public CauseParameter
{
   friend class Singleton<PotsCauseParameter>;

   PotsCauseParameter();
   ~PotsCauseParameter() = default;
};

class PotsDigitsParameter : public AddressParameter
{
   friend class Singleton<PotsDigitsParameter>;

   PotsDigitsParameter();
   ~PotsDigitsParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
};

class PotsFacilityParameter : public PotsParameter
{
   friend class Singleton<PotsFacilityParameter>;

   PotsFacilityParameter();
   ~PotsFacilityParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
   void DisplayMsg(ostream& stream, const string& prefix,
      const byte_t* bytes, size_t count) const override;
   TestRc InjectMsg(CliThread& cli, Message& msg, Usage use) const override;
   TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};

class FacilityMandParm : public CliText
{
public: FacilityMandParm();
};

class FacilityOptParm : public CliText
{
public: FacilityOptParm();
};

class PotsHeaderParameter : public PotsParameter
{
   friend class Singleton<PotsHeaderParameter>;

   PotsHeaderParameter();
   ~PotsHeaderParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
   void DisplayMsg(ostream& stream, const string& prefix,
      const byte_t* bytes, size_t count) const override;
   TestRc InjectMsg(CliThread& cli, Message& msg, Usage use) const override;
   TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};

class PotsMediaParameter : public MediaParameter
{
   friend class Singleton<PotsMediaParameter>;

   PotsMediaParameter();
   ~PotsMediaParameter() = default;
};

class PotsProgressParameter : public ProgressParameter
{
   friend class Singleton<PotsProgressParameter>;

   PotsProgressParameter();
   ~PotsProgressParameter() = default;
};

class PotsRingParameter : public PotsParameter
{
   friend class Singleton<PotsRingParameter>;

   PotsRingParameter();
   ~PotsRingParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
   void DisplayMsg(ostream& stream, const string& prefix,
      const byte_t* bytes, size_t count) const override;
   TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};

class PotsScanParameter : public PotsParameter
{
   friend class Singleton<PotsScanParameter>;

   PotsScanParameter();
   ~PotsScanParameter() = default;
   CliParm* CreateCliParm(Usage use) const override;
   void DisplayMsg(ostream& stream, const string& prefix,
      const byte_t* bytes, size_t count) const override;
   TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};

//==============================================================================

PotsProtocol::PotsProtocol() : TlvProtocol(PotsProtocolId, TimerProtocolId)
{
   Debug::ft("PotsProtocol.ctor");

   //  Create POTS signals and parameters.
   //
   Singleton<PotsOffhookSignal>::Instance();
   Singleton<PotsDigitsSignal>::Instance();
   Singleton<PotsAlertingSignal>::Instance();
   Singleton<PotsFlashSignal>::Instance();
   Singleton<PotsOnhookSignal>::Instance();
   Singleton<PotsFacilitySignal>::Instance();
   Singleton<PotsProgressSignal>::Instance();
   Singleton<PotsSuperviseSignal>::Instance();
   Singleton<PotsLockoutSignal>::Instance();
   Singleton<PotsReleaseSignal>::Instance();

   Singleton<PotsHeaderParameter>::Instance();
   Singleton<PotsDigitsParameter>::Instance();
   Singleton<PotsRingParameter>::Instance();
   Singleton<PotsScanParameter>::Instance();
   Singleton<PotsMediaParameter>::Instance();
   Singleton<PotsCauseParameter>::Instance();
   Singleton<PotsProgressParameter>::Instance();
   Singleton<PotsFacilityParameter>::Instance();
}

//------------------------------------------------------------------------------

PotsProtocol::~PotsProtocol()
{
   Debug::ftnt("PotsProtocol.dtor");
}

//==============================================================================

PotsSignal::PotsSignal(Id sid) : Signal(PotsProtocolId, sid) { }

//------------------------------------------------------------------------------

PotsAlertingSignal::PotsAlertingSignal() : PotsSignal(Alerting) { }

fixed_string AlertingTextStr = "A";
fixed_string AlertingTextExpl = "alerting";

CliText* PotsAlertingSignal::CreateText() const
{
   return new CliText(AlertingTextExpl, AlertingTextStr);
}

//------------------------------------------------------------------------------

PotsDigitsSignal::PotsDigitsSignal() : PotsSignal(Digits) { }

fixed_string DigitsTextStr = "D";
fixed_string DigitsTextExpl = "digits";

CliText* PotsDigitsSignal::CreateText() const
{
   return new CliText(DigitsTextExpl, DigitsTextStr);
}

//------------------------------------------------------------------------------

PotsFacilitySignal::PotsFacilitySignal() : PotsSignal(Facility) { }

fixed_string FacilitySigStr = "F";
fixed_string FacilitySigExpl = "facility";

CliText* PotsFacilitySignal::CreateText() const
{
   return new CliText(FacilitySigExpl, FacilitySigStr);
}

//------------------------------------------------------------------------------

PotsFlashSignal::PotsFlashSignal() : PotsSignal(Flash) { }

fixed_string FlashTextStr = "L";
fixed_string FlashTextExpl = "flash ('link')";

CliText* PotsFlashSignal::CreateText() const
{
   return new CliText(FlashTextExpl, FlashTextStr);
}

//------------------------------------------------------------------------------

PotsLockoutSignal::PotsLockoutSignal() : PotsSignal(Lockout) { }

fixed_string LockoutTextStr = "L";
fixed_string LockoutTextExpl = "lockout";

CliText* PotsLockoutSignal::CreateText() const
{
   return new CliText(LockoutTextExpl, LockoutTextStr);
}

//------------------------------------------------------------------------------

PotsOffhookSignal::PotsOffhookSignal() : PotsSignal(Offhook) { }

fixed_string OffhookTextStr = "B";
fixed_string OffhookTextExpl = "offhook ('begin')";

CliText* PotsOffhookSignal::CreateText() const
{
   return new CliText(OffhookTextExpl, OffhookTextStr);
}

//------------------------------------------------------------------------------

PotsOnhookSignal::PotsOnhookSignal() : PotsSignal(Onhook) { }

fixed_string OnhookTextStr = "E";
fixed_string OnhookTextExpl = "onhook ('end')";

CliText* PotsOnhookSignal::CreateText() const
{
   return new CliText(OnhookTextExpl, OnhookTextStr);
}

//------------------------------------------------------------------------------

PotsProgressSignal::PotsProgressSignal() : PotsSignal(Progress) { }

fixed_string ProgressTextStr = "P";
fixed_string ProgressTextExpl = "progress";

CliText* PotsProgressSignal::CreateText() const
{
   return new CliText(ProgressTextExpl, ProgressTextStr);
}

//------------------------------------------------------------------------------

PotsReleaseSignal::PotsReleaseSignal() : PotsSignal(Release) { }

fixed_string ReleaseTextStr = "R";
fixed_string ReleaseTextExpl = "release";

CliText* PotsReleaseSignal::CreateText() const
{
   return new CliText(ReleaseTextExpl, ReleaseTextStr);
}

//------------------------------------------------------------------------------

PotsSuperviseSignal::PotsSuperviseSignal() : PotsSignal(Supervise) { }

fixed_string SuperviseTextStr = "S";
fixed_string SuperviseTextExpl = "supervise";

CliText* PotsSuperviseSignal::CreateText() const
{
   return new CliText(SuperviseTextExpl, SuperviseTextStr);
}

//==============================================================================

PotsParameter::PotsParameter(Id pid) : TlvParameter(PotsProtocolId, pid) { }

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

CliParm* PotsDigitsParameter::CreateCliParm(Usage use) const
{
   return new CliTextParm(DigitsExpl, false, 0);
}

//==============================================================================

PotsFacilityParameter::PotsFacilityParameter() : PotsParameter(Facility)
{
   BindUsage(PotsSignal::Facility, Mandatory);
   BindUsage(PotsSignal::Supervise, Optional);
}

//------------------------------------------------------------------------------

CliParm* PotsFacilityParameter::CreateCliParm(Usage use) const
{
   if(use == Mandatory) return new FacilityMandParm;
   return new FacilityOptParm;
}

//------------------------------------------------------------------------------

void PotsFacilityParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast<const PotsFacilityInfo*>(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

Parameter::TestRc PotsFacilityParameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft("PotsFacilityParameter.InjectMsg");

   id_t             index;
   word             sid, ind;
   PotsFacilityInfo info;
   auto&            pmsg = static_cast<PotsMessage&>(msg);

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

Parameter::TestRc PotsFacilityParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft("PotsFacilityParameter.VerifyMsg");

   TestRc            rc;
   auto&             pmsg = static_cast<const PotsMessage&>(msg);
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

fixed_string ServiceIdExpl = "sid: ServiceId";

fixed_string FacilityIndExpl = "ind: Facility::Ind";

fixed_string FacilityParmStr = "f";
fixed_string FacilityParmExpl = "facility info";

FacilityMandParm::FacilityMandParm() :
   CliText(FacilityParmExpl, FacilityParmStr)
{
   BindParm(*new CliIntParm(ServiceIdExpl, 0, Service::MaxId));
   BindParm(*new CliIntParm(FacilityIndExpl, 0, UINT8_MAX));
}

FacilityOptParm::FacilityOptParm() :
   CliText(FacilityParmExpl, FacilityParmStr, true)
{
   BindParm(*new CliIntParm(ServiceIdExpl, 0, Service::MaxId));
   BindParm(*new CliIntParm(FacilityIndExpl, 0, UINT8_MAX));
}

//------------------------------------------------------------------------------

PotsFacilityInfo::PotsFacilityInfo() :
   sid(NIL_ID),
   ind(Facility::NilInd)
{
   Debug::ft("PotsFacilityInfo.ctor");
}

//------------------------------------------------------------------------------

void PotsFacilityInfo::Display(ostream& stream, const string& prefix) const
{
   auto svc = Singleton<ServiceRegistry>::Instance()->Services().At(sid);

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

CliParm* PotsHeaderParameter::CreateCliParm(Usage use) const
{
   return new CliIntParm(HeaderParmExpl, 0, Switch::MaxPortId);
}

//------------------------------------------------------------------------------

void PotsHeaderParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast<const PotsHeaderInfo*>(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

Parameter::TestRc PotsHeaderParameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft("PotsHeaderParameter.InjectMsg");

   word           port;
   PotsHeaderInfo info;
   auto&          pmsg = static_cast<PotsMessage&>(msg);

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

Parameter::TestRc PotsHeaderParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft("PotsHeaderParameter.VerifyMsg");

   TestRc          rc;
   auto&           pmsg = static_cast<const PotsMessage&>(msg);
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

PotsHeaderInfo::PotsHeaderInfo() :
   signal(NIL_ID),
   port(NIL_ID)
{
   Debug::ft("PotsHeaderInfo.ctor");
}

//------------------------------------------------------------------------------

void PotsHeaderInfo::Display(ostream& stream, const string& prefix) const
{
   auto sig = Singleton<PotsProtocol>::Instance()->GetSignal(signal);
   auto cct = Singleton<Switch>::Instance()->CircuitName(port);

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

CliParm* PotsRingParameter::CreateCliParm(Usage use) const
{
   return new CliBoolParm(RingParmExpl, true, RingTag);
}

//------------------------------------------------------------------------------

void PotsRingParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast<const PotsRingInfo*>(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

Parameter::TestRc PotsRingParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft("PotsRingParameter.VerifyMsg");

   TestRc        rc;
   auto&         pmsg = static_cast<const PotsMessage&>(msg);
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

PotsRingInfo::PotsRingInfo() : on(false)
{
   Debug::ft("PotsRingInfo.ctor");
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

CliParm* PotsScanParameter::CreateCliParm(Usage use) const
{
   return new CliTextParm(ScanParmExpl, true, 0, ScanTag);
}

//------------------------------------------------------------------------------

void PotsScanParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast<const PotsScanInfo*>(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

Parameter::TestRc PotsScanParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft("PotsScanParameter.VerifyMsg");

   TestRc        rc;
   auto&         pmsg = static_cast<const PotsMessage&>(msg);
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

PotsScanInfo::PotsScanInfo() :
   digits(false),
   flash(false)
{
   Debug::ft("PotsScanInfo.ctor");
}

//------------------------------------------------------------------------------

void PotsScanInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "digits : " << digits << CRLF;
   stream << prefix << "flash  : " << flash << CRLF;
}
}
