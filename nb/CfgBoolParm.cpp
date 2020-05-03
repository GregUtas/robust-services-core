//==============================================================================
//
//  CfgBoolParm.cpp
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
#include "CfgBoolParm.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "FunctionGuard.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgBoolParm_ctor = "CfgBoolParm.ctor";

CfgBoolParm::CfgBoolParm
(c_string key, c_string def, bool* field, c_string expl) :
   CfgBitParm(key, def, expl),
   curr_(field),
   next_(false)
{
   Debug::ft(CfgBoolParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CfgBoolParm_dtor = "CfgBoolParm.dtor";

CfgBoolParm::~CfgBoolParm()
{
   Debug::ft(CfgBoolParm_dtor);
}

//------------------------------------------------------------------------------

void CfgBoolParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgBitParm::Display(stream, prefix, options);

   stream << prefix << "curr : " << *curr_ << CRLF;
   stream << prefix << "next : " << next_ << CRLF;
}

//------------------------------------------------------------------------------

void CfgBoolParm::Patch(sel_t selector, void* arguments)
{
   CfgBitParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CfgBoolParm_SetCurr = "CfgBoolParm.SetCurr";

void CfgBoolParm::SetCurr()
{
   Debug::ft(CfgBoolParm_SetCurr);

   FunctionGuard guard(Guard_MemUnprotect);
   *curr_ = next_;
   CfgBitParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgBoolParm_SetNextValue = "CfgBoolParm.SetNextValue";

bool CfgBoolParm::SetNextValue(bool value)
{
   Debug::ft(CfgBoolParm_SetNextValue);

   FunctionGuard guard(Guard_MemUnprotect);
   next_ = value;
   return true;
}
}
