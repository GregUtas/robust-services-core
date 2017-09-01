//==============================================================================
//
//  CfgFlagParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgFlagParm.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgFlagParm_ctor = "CfgFlagParm.ctor";

CfgFlagParm::CfgFlagParm(const char* key, const char* def,
   Flags* field, FlagId fid, const char* expl) :
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
   Debug::ft(CfgFlagParm_dtor);
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

bool CfgFlagParm::GetCurrValue() const
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

   curr_->set(fid_, next_);
   CfgBitParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgFlagParm_SetNextValue = "CfgFlagParm.SetNextValue";

bool CfgFlagParm::SetNextValue(bool value)
{
   Debug::ft(CfgFlagParm_SetNextValue);

   next_ = value;
   return true;
}
}
