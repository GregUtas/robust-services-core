//==============================================================================
//
//  CfgBoolParm.cpp
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
CfgBoolParm::CfgBoolParm(c_string key, c_string def, c_string expl) :
   CfgBitParm(key, def, expl),
   curr_(false),
   next_(false)
{
   Debug::ft("CfgBoolParm.ctor");
}

//------------------------------------------------------------------------------

CfgBoolParm::~CfgBoolParm()
{
   Debug::ftnt("CfgBoolParm.dtor");
}

//------------------------------------------------------------------------------

void CfgBoolParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgBitParm::Display(stream, prefix, options);

   stream << prefix << "curr : " << curr_ << CRLF;
   stream << prefix << "next : " << next_ << CRLF;
}

//------------------------------------------------------------------------------

void CfgBoolParm::Patch(sel_t selector, void* arguments)
{
   CfgBitParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void CfgBoolParm::SetCurr()
{
   Debug::ft("CfgBoolParm.SetCurr");

   FunctionGuard guard(Guard_MemUnprotect);
   curr_ = next_;
   CfgBitParm::SetCurr();
}

//------------------------------------------------------------------------------

bool CfgBoolParm::SetNextValue(bool value)
{
   Debug::ft("CfgBoolParm.SetNextValue");

   FunctionGuard guard(Guard_MemUnprotect);
   next_ = value;
   return true;
}
}
