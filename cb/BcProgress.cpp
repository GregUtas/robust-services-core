//==============================================================================
//
//  BcProgress.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "BcProgress.h"
#include <ostream>
#include "CliIntParm.h"
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
fixed_string ProgressIndStrings[Progress::MaxInd + 2] =
{
   "Nil",
   "End Of Selection",
   "Alerting",
   "Suspend",
   "Resume",
   "Media Update",
   ERROR_STR
};

const char* Progress::strInd(Ind ind)
{
   if((ind >= 0) && (ind <= MaxInd)) return ProgressIndStrings[ind];
   return ProgressIndStrings[MaxInd + 1];
}

//==============================================================================

fn_name ProgressInfo_ctor = "ProgressInfo.ctor";

ProgressInfo::ProgressInfo() : progress(Progress::NilInd)
{
   Debug::ft(ProgressInfo_ctor);
}

//------------------------------------------------------------------------------

void ProgressInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "progress : " << int(progress);
   stream << " (" << Progress::strInd(progress) << ')' << CRLF;
}

//==============================================================================

fn_name ProgressParameter_ctor = "ProgressParameter.ctor";

ProgressParameter::ProgressParameter(ProtocolId prid, Id pid) :
   TlvIntParameter< Progress::Ind >(prid, pid)
{
   Debug::ft(ProgressParameter_ctor);
}

//------------------------------------------------------------------------------

fn_name ProgressParameter_dtor = "ProgressParameter.dtor";

ProgressParameter::~ProgressParameter()
{
   Debug::ft(ProgressParameter_dtor);
}

//------------------------------------------------------------------------------

class ProgressMandParm : public CliIntParm
{
public: ProgressMandParm();
};

class ProgressOptParm : public CliIntParm
{
public: ProgressOptParm();
};

fixed_string ProgressParmExpl = "progress: Progress::Ind";
fixed_string ProgressTag = "p";

ProgressMandParm::ProgressMandParm() :
   CliIntParm(ProgressParmExpl, 0, Progress::MaxInd) { }

ProgressOptParm::ProgressOptParm() :
   CliIntParm(ProgressParmExpl, 0, Progress::MaxInd, true, ProgressTag) { }

CliParm* ProgressParameter::CreateCliParm(Usage use) const
{
   if(use == Mandatory) return new ProgressMandParm;
   return new ProgressOptParm;
}

//------------------------------------------------------------------------------

void ProgressParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const ProgressInfo* >(bytes)->Display(stream, prefix);
}
}
