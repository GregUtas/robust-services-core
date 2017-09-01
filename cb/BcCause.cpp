//==============================================================================
//
//  BcCause.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "BcCause.h"
#include <ostream>
#include "CliIntParm.h"
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
fixed_string CauseIndStrings[Cause::MaxInd + 2] =
{
   "Invalid Cause",
   "Unallocated Number",
   "Confirmation",
   "Address Timeout",
   "Normal Call Clearing",
   "User Busy",
   "Alerting Timeout",
   "Answer Timeout",
   "Exchange Routing Error",
   "Destination Out Of Order",
   "Invalid Address",
   "Facility Rejected",
   "Temporary Failure",
   "Outgoing Calls Barred",
   "Incoming Calls Barred",
   "Call Redirected",
   "Excessive Redirection",
   "Message Invalid For State",
   "Parameter Absent",
   "Protocol Timer Expired",
   "Reset Circuit",
   ERROR_STR
};

const char* Cause::strInd(Ind ind)
{
   if((ind >= 0) && (ind <= MaxInd)) return CauseIndStrings[ind];
   return CauseIndStrings[MaxInd + 1];
}

//==============================================================================

fn_name CauseInfo_ctor = "CauseInfo.ctor";

CauseInfo::CauseInfo() : cause(Cause::NilInd)
{
   Debug::ft(CauseInfo_ctor);
}

//------------------------------------------------------------------------------

void CauseInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "cause : " << int(cause);
   stream << " (" << Cause::strInd(cause) << ')' << CRLF;
}

//==============================================================================

fn_name CauseParameter_ctor = "CauseParameter.ctor";

CauseParameter::CauseParameter(ProtocolId prid, Id pid) :
   TlvIntParameter< Cause::Ind >(prid, pid)
{
   Debug::ft(CauseParameter_ctor);
}

//------------------------------------------------------------------------------

fn_name CauseParameter_dtor = "CauseParameter.dtor";

CauseParameter::~CauseParameter()
{
   Debug::ft(CauseParameter_dtor);
}

//------------------------------------------------------------------------------

class CauseMandParm : public CliIntParm
{
public: CauseMandParm();
};

class CauseOptParm : public CliIntParm
{
public: CauseOptParm();
};

fixed_string CauseParmExpl = "cause: Cause::Ind";
fixed_string CauseTag = "c";

CauseMandParm::CauseMandParm() : CliIntParm(CauseParmExpl, 0, Cause::MaxInd) { }

CauseOptParm::CauseOptParm() :
   CliIntParm(CauseParmExpl, 0, Cause::MaxInd, true, CauseTag) { }

CliParm* CauseParameter::CreateCliParm(Usage use) const
{
   if(use == Mandatory) return new CauseMandParm;
   return new CauseOptParm;
}

//------------------------------------------------------------------------------

void CauseParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const CauseInfo* >(bytes)->Display(stream, prefix);
}
}
