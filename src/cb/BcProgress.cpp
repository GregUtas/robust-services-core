//==============================================================================
//
//  BcProgress.cpp
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
#include "BcProgress.h"
#include <ostream>
#include "CliIntParm.h"
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
   return (ind <= MaxInd ?
      ProgressIndStrings[ind] : ProgressIndStrings[MaxInd + 1]);
}

//==============================================================================

ProgressInfo::ProgressInfo() : progress(Progress::NilInd)
{
   Debug::ft("ProgressInfo.ctor");
}

//------------------------------------------------------------------------------

void ProgressInfo::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "progress : " << int(progress);
   stream << " (" << Progress::strInd(progress) << ')' << CRLF;
}

//==============================================================================

ProgressParameter::ProgressParameter(ProtocolId prid, Id pid) :
   TlvIntParameter<Progress::Ind>(prid, pid)
{
   Debug::ft("ProgressParameter.ctor");
}

//------------------------------------------------------------------------------

ProgressParameter::~ProgressParameter()
{
   Debug::ftnt("ProgressParameter.dtor");
}

//------------------------------------------------------------------------------

fixed_string ProgressParmExpl = "progress: Progress::Ind";
fixed_string ProgressTag = "p";

CliParm* ProgressParameter::CreateCliParm(Usage use) const
{
   return (use == Mandatory ?
      new CliIntParm(ProgressParmExpl, 0, Progress::MaxInd) :
      new CliIntParm(ProgressParmExpl, 0, Progress::MaxInd, true, ProgressTag));
}

//------------------------------------------------------------------------------

void ProgressParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast<const ProgressInfo*>(bytes)->Display(stream, prefix);
}
}
