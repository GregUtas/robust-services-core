//==============================================================================
//
//  CfgBoolParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgBoolParm.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgBoolParm_ctor = "CfgBoolParm.ctor";

CfgBoolParm::CfgBoolParm
(const char* key, const char* def, bool* field, const char* expl) :
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

   *curr_ = next_;
   CfgBitParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgBoolParm_SetNextValue = "CfgBoolParm.SetNextValue";

bool CfgBoolParm::SetNextValue(bool value)
{
   Debug::ft(CfgBoolParm_SetNextValue);

   next_ = value;
   return true;
}
}
