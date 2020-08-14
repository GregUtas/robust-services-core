//==============================================================================
//
//  CfgIntParm.cpp
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
#include "CfgIntParm.h"
#include <iosfwd>
#include <sstream>
#include <string>
#include "Debug.h"
#include "FunctionGuard.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgIntParm_ctor = "CfgIntParm.ctor";

CfgIntParm::CfgIntParm(c_string key, c_string def, word min, word max,
   c_string expl) : CfgParm(key, def, expl),
   curr_(0),
   next_(0),
   min_(min),
   max_(max)
{
   Debug::ft(CfgIntParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CfgIntParm_dtor = "CfgIntParm.dtor";

CfgIntParm::~CfgIntParm()
{
   Debug::ftnt(CfgIntParm_dtor);
}

//------------------------------------------------------------------------------

void CfgIntParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgParm::Display(stream, prefix, options);

   stream << prefix << "curr : " << curr_ << CRLF;
   stream << prefix << "next : " << next_ << CRLF;
   stream << prefix << "min  : " << min_ << CRLF;
   stream << prefix << "max  : " << max_ << CRLF;
}

//------------------------------------------------------------------------------

void CfgIntParm::Explain(string& expl) const
{
   std::ostringstream stream;

   CfgParm::Explain(expl);

   stream << "INT (" << min_ << ':' << max_ << "): " << expl;
   expl = stream.str();
}

//------------------------------------------------------------------------------

fn_name CfgIntParm_GetCurr = "CfgIntParm.GetCurr";

string CfgIntParm::GetCurr() const
{
   Debug::ft(CfgIntParm_GetCurr);

   return std::to_string(curr_);
}

//------------------------------------------------------------------------------

void CfgIntParm::Patch(sel_t selector, void* arguments)
{
   CfgParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CfgIntParm_SetCurr = "CfgIntParm.SetCurr";

void CfgIntParm::SetCurr()
{
   Debug::ft(CfgIntParm_SetCurr);

   FunctionGuard guard(Guard_MemUnprotect);
   curr_ = next_;
   CfgParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgIntParm_SetNext = "CfgIntParm.SetNext";

bool CfgIntParm::SetNext(c_string input)
{
   Debug::ft(CfgIntParm_SetNext);

   word n = 0;
   std::istringstream stream(input);

   stream >> n;
   if(stream.eof()) return SetNextValue(n);
   return false;
}

//------------------------------------------------------------------------------

fn_name CfgIntParm_SetNextValue = "CfgIntParm.SetNextValue";

bool CfgIntParm::SetNextValue(word value)
{
   Debug::ft(CfgIntParm_SetNextValue);

   if((value >= min_) && (value <= max_))
   {
      FunctionGuard guard(Guard_MemUnprotect);
      next_ = value;
      return true;
   }

   return false;
}
}
