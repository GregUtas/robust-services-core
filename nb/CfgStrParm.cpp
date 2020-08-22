//==============================================================================
//
//  CfgStrParm.cpp
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
#include "CfgStrParm.h"
#include <ostream>
#include "Debug.h"
#include "FunctionGuard.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgStrParm_ctor = "CfgStrParm.ctor";

CfgStrParm::CfgStrParm(c_string key, c_string def, c_string expl) :
   CfgParm(key, def, expl)
{
   Debug::ft(CfgStrParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CfgStrParm_dtor = "CfgStrParm.dtor";

CfgStrParm::~CfgStrParm()
{
   Debug::ftnt(CfgStrParm_dtor);
}

//------------------------------------------------------------------------------

void CfgStrParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgParm::Display(stream, prefix, options);

   stream << prefix << "curr : " << curr_ << CRLF;
   stream << prefix << "next : " << next_ << CRLF;
}

//------------------------------------------------------------------------------

void CfgStrParm::Explain(string& expl) const
{
   CfgParm::Explain(expl);

   expl = "STRING: " + expl;
}

//------------------------------------------------------------------------------

void CfgStrParm::Patch(sel_t selector, void* arguments)
{
   CfgParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CfgStrParm_SetCurr = "CfgStrParm.SetCurr";

void CfgStrParm::SetCurr()
{
   Debug::ft(CfgStrParm_SetCurr);

   FunctionGuard guard(Guard_MemUnprotect);
   curr_ = next_;
   CfgParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgStrParm_SetNext = "CfgStrParm.SetNext";

bool CfgStrParm::SetNext(c_string input)
{
   Debug::ft(CfgStrParm_SetNext);

   FunctionGuard guard(Guard_MemUnprotect);
   next_ = input;
   return true;
}
}
