//==============================================================================
//
//  CfgBitParm.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "CfgBitParm.h"
#include <cstddef>
#include <string>
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
CfgBitParm::CfgBitParm(c_string key, c_string def, c_string expl) :
   CfgParm(key, def, expl)
{
   Debug::ft("CfgBitParm.ctor");
}

//------------------------------------------------------------------------------

CfgBitParm::~CfgBitParm()
{
   Debug::ftnt("CfgBitParm.dtor");
}

//------------------------------------------------------------------------------

void CfgBitParm::Explain(string& expl) const
{
   CfgParm::Explain(expl);

   expl = "BOOL (T|F): " + expl;
}

//------------------------------------------------------------------------------

string CfgBitParm::GetCurr() const
{
   Debug::ft("CfgBitParm.GetCurr");

   string s = (GetValue() ? ValidTrueChars() : ValidFalseChars());
   s.erase(1);
   return s;
}

//------------------------------------------------------------------------------

fn_name CfgBitParm_GetValue = "CfgBitParm.GetValue";

bool CfgBitParm::GetValue() const
{
   Debug::ft(CfgBitParm_GetValue);

   Debug::SwLog(CfgBitParm_GetValue, strOver(this), 0);
   return false;
}

//------------------------------------------------------------------------------

void CfgBitParm::Patch(sel_t selector, void* arguments)
{
   CfgParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool CfgBitParm::SetNext(c_string input)
{
   Debug::ft("CfgBitParm.SetNext");

   string next(input);

   if(next.size() != 1) return false;

   string tchars(ValidTrueChars());
   auto n = tchars.size();

   for(size_t i = 0; i < n; ++i)
   {
      if(next.front() == tchars[i]) return SetNextValue(true);
   }

   string fchars(ValidFalseChars());
   n = fchars.size();

   for(size_t i = 0; i < n; ++i)
   {
      if(next.front() == fchars[i]) return SetNextValue(false);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CfgBitParm_SetNextValue = "CfgBitParm.SetNextValue";

bool CfgBitParm::SetNextValue(bool value)
{
   Debug::ft(CfgBitParm_SetNextValue);

   Debug::SwLog(CfgBitParm_SetNextValue, strOver(this), 0);
   return false;
}

//------------------------------------------------------------------------------

fixed_string CfgBitParm::ValidFalseChars()
{
   //  Characters that set a configuration parameter to false.
   //
   static fixed_string FalseChars = "FfNn";

   return FalseChars;
}

//------------------------------------------------------------------------------

fixed_string CfgBitParm::ValidTrueChars()
{
   //  Characters that set a configuration parameter to true.
   //
   static fixed_string TrueChars = "TtYy";

   return TrueChars;
}
}
