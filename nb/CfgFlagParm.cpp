//==============================================================================
//
//  CfgFlagParm.cpp
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
fn_name CfgFlagParm_ctor = "CfgFlagParm.ctor";

CfgFlagParm::CfgFlagParm(c_string key, c_string def,
   Flags* field, FlagId fid, c_string expl) :
   CfgBitParm(key, def, expl),
   curr_(field),
   next_(false),
   fid_(fid)
{
   Debug::ft(CfgFlagParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CfgFlagParm_dtor = "CfgFlagParm.dtor";

CfgFlagParm::~CfgFlagParm()
{
   Debug::ftnt(CfgFlagParm_dtor);
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

bool CfgFlagParm::GetValue() const
{
   return curr_->test(fid_);
}

//------------------------------------------------------------------------------

void CfgFlagParm::Patch(sel_t selector, void* arguments)
{
   CfgBitParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CfgFlagParm_SetCurr = "CfgFlagParm.SetCurr";

void CfgFlagParm::SetCurr()
{
   Debug::ft(CfgFlagParm_SetCurr);

   FunctionGuard guard(Guard_MemUnprotect);
   curr_->set(fid_, next_);
   CfgBitParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgFlagParm_SetNextValue = "CfgFlagParm.SetNextValue";

bool CfgFlagParm::SetNextValue(bool value)
{
   Debug::ft(CfgFlagParm_SetNextValue);

   FunctionGuard guard(Guard_MemUnprotect);
   next_ = value;
   return true;
}
}
