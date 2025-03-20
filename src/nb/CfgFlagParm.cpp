//==============================================================================
//
//  CfgFlagParm.cpp
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
#include "CfgFlagParm.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Debug.h"
#include "FunctionGuard.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
CfgFlagParm::CfgFlagParm(c_string key, c_string def,
   Flags* field, FlagId fid, c_string expl) :
   CfgBitParm(key, def, expl),
   curr_(field),
   next_(false),
   fid_(fid)
{
   Debug::ft("CfgFlagParm.ctor");
}

//------------------------------------------------------------------------------

CfgFlagParm::~CfgFlagParm()
{
   Debug::ftnt("CfgFlagParm.dtor");
}

//------------------------------------------------------------------------------

bool CfgFlagParm::CurrValue() const
{
   return curr_->test(fid_);
}

//------------------------------------------------------------------------------

void CfgFlagParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgBitParm::Display(stream, prefix, options);

   stream << prefix << "curr : " << curr_->test(fid_) << CRLF;
   stream << prefix << "next : " << next_ << CRLF;
   stream << prefix << "fid  : " << int(fid_) << CRLF;
}

//------------------------------------------------------------------------------

bool CfgFlagParm::NextValue() const
{
   return next_;
}

//------------------------------------------------------------------------------

void CfgFlagParm::Patch(sel_t selector, void* arguments)
{
   CfgBitParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void CfgFlagParm::SetCurr()
{
   Debug::ft("CfgFlagParm.SetCurr");

   FunctionGuard guard(Guard_MemUnprotect);
   curr_->set(fid_, next_);
   CfgBitParm::SetCurr();
}

//------------------------------------------------------------------------------

bool CfgFlagParm::SetNextValue(bool value)
{
   Debug::ft("CfgFlagParm.SetNextValue");

   FunctionGuard guard(Guard_MemUnprotect);
   next_ = value;
   return true;
}
}
