//==============================================================================
//
//  CfgIntParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgIntParm.h"
#include <iosfwd>
#include <sstream>
#include <string>
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgIntParm_ctor = "CfgIntParm.ctor";

CfgIntParm::CfgIntParm(const char* key, const char* def, word* field,
   word min, word max, const char* expl) :
   CfgParm(key, def, expl),
   curr_(field),
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
   Debug::ft(CfgIntParm_dtor);
}

//------------------------------------------------------------------------------

void CfgIntParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgParm::Display(stream, prefix, options);

   stream << prefix << "curr : " << *curr_ << CRLF;
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

   std::ostringstream stream;

   stream << *curr_;
   return stream.str();
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

   *curr_ = next_;
   CfgParm::SetCurr();
}

//------------------------------------------------------------------------------

fn_name CfgIntParm_SetNext = "CfgIntParm.SetNext";

bool CfgIntParm::SetNext(const string& input)
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
      string input;
      next_ = value;
      return true;
   }

   return false;
}
}
