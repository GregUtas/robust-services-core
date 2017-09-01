//==============================================================================
//
//  CfgBitParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgBitParm.h"
#include <cstddef>
#include <string>
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgBitParm_ctor = "CfgBitParm.ctor";

CfgBitParm::CfgBitParm(const char* key, const char* def, const char* expl) :
   CfgParm(key, def, expl)
{
   Debug::ft(CfgBitParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CfgBitParm_dtor = "CfgBitParm.dtor";

CfgBitParm::~CfgBitParm()
{
   Debug::ft(CfgBitParm_dtor);
}

//------------------------------------------------------------------------------

void CfgBitParm::Explain(string& expl) const
{
   CfgParm::Explain(expl);

   expl = "BOOL (T|F): " + expl;
}

//------------------------------------------------------------------------------

fn_name CfgBitParm_GetCurr = "CfgBitParm.GetCurr";

string CfgBitParm::GetCurr() const
{
   Debug::ft(CfgBitParm_GetCurr);

   string s = (GetCurrValue() ? ValidTrueChars() : ValidFalseChars());
   s.erase(1);
   return s;
}

//------------------------------------------------------------------------------

fn_name CfgBitParm_GetCurrValue = "CfgBitParm.GetCurrValue";

bool CfgBitParm::GetCurrValue() const
{
   Debug::ft(CfgBitParm_GetCurrValue);

   //  This is a pure virtual function.
   //
   Debug::SwErr(CfgBitParm_GetCurrValue, 0, 0);
   return false;
}

//------------------------------------------------------------------------------

void CfgBitParm::Patch(sel_t selector, void* arguments)
{
   CfgParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CfgBitParm_SetNext = "CfgBitParm.SetNext";

bool CfgBitParm::SetNext(const string& input)
{
   Debug::ft(CfgBitParm_SetNext);

   if(input.size() != 1) return false;

   string tchars(ValidTrueChars());
   auto n = tchars.size();

   for(size_t i = 0; i < n; ++i)
   {
      if(input.front() == tchars[i]) return SetNextValue(true);
   }

   string fchars(ValidFalseChars());
   n = fchars.size();

   for(size_t i = 0; i < n; ++i)
   {
      if(input.front() == fchars[i]) return SetNextValue(false);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CfgBitParm_SetNextValue = "CfgBitParm.SetNextValue";

bool CfgBitParm::SetNextValue(bool value)
{
   Debug::ft(CfgBitParm_SetNextValue);

   //  This is a pure virtual function.
   //
   Debug::SwErr(CfgBitParm_SetNextValue, 0, 0);
   return false;
}

//------------------------------------------------------------------------------

fixed_string CfgBitParm::ValidFalseChars()
{
   //> Characters that set a configuration parameter to false.
   //
   static fixed_string FalseChars = "FfNn";

   return FalseChars;
}

//------------------------------------------------------------------------------

fixed_string CfgBitParm::ValidTrueChars()
{
   //> Characters that set a configuration parameter to true.
   //
   static fixed_string TrueChars = "TtYy";

   return TrueChars;
}
}
