//==============================================================================
//
//  CfgStrParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgStrParm.h"
#include <ostream>
#include "Clock.h"
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgStrParm_ctor = "CfgStrParm.ctor";

CfgStrParm::CfgStrParm
   (const char* key, const char* def, string* field, const char* expl) :
   CfgParm(key, def, expl),
   curr_(field),
   next_(EMPTY_STR)
{
   Debug::ft(CfgStrParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CfgStrParm_dtor = "CfgStrParm.dtor";

CfgStrParm::~CfgStrParm()
{
   Debug::ft(CfgStrParm_dtor);
}

//------------------------------------------------------------------------------

void CfgStrParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgParm::Display(stream, prefix, options);

   stream << prefix << "curr : " << *curr_ << CRLF;
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

   *curr_ = next_;
   CfgParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgStrParm_SetNext = "CfgStrParm.SetNext";

bool CfgStrParm::SetNext(const string& input)
{
   Debug::ft(CfgStrParm_SetNext);

   next_ = input;
   return true;
}

//==============================================================================

fn_name CfgFileTimeParm_ctor = "CfgFileTimeParm.ctor";

CfgFileTimeParm::CfgFileTimeParm
   (const char* key, const char* def, string* field, const char* expl) :
   CfgStrParm(key, def, field, expl),
   input_(def)
{
   Debug::ft(CfgFileTimeParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CfgFileTimeParm_dtor = "CfgFileTimeParm.dtor";

CfgFileTimeParm::~CfgFileTimeParm()
{
   Debug::ft(CfgFileTimeParm_dtor);
}

//------------------------------------------------------------------------------

fn_name CfgFileTimeParm_SetNext = "CfgFileTimeParm.SetNext";

bool CfgFileTimeParm::SetNext(const string& input)
{
   Debug::ft(CfgFileTimeParm_SetNext);

   input_ = input;
   string full = input + Clock::TimeZeroStr();
   return CfgStrParm::SetNext(full);
}
}
