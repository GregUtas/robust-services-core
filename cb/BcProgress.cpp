//==============================================================================
//
//  BcProgress.cpp
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
#include "BcProgress.h"
#include "CliIntParm.h"
#include <ostream>
#include "Debug.h"

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

c_string Progress::strInd(Ind ind)
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
