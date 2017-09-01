//==============================================================================
//
//  SoftwareException.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SoftwareException.h"
#include <ostream>
#include "Debug.h"
#include "Formatters.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SoftwareException_ctor1 = "SoftwareException.ctor";

SoftwareException::SoftwareException(debug64_t errval, debug32_t offset,
   fn_depth depth) : Exception(true, depth),
   errval_(errval),
   errstr_(EMPTY_STR),
   offset_(offset)
{
   Debug::ft(SoftwareException_ctor1);
}

//------------------------------------------------------------------------------

fn_name SoftwareException_ctor2 = "SoftwareException.ctor(string)";

SoftwareException::SoftwareException(const string& errstr, debug32_t offset,
   fn_depth depth) : Exception(true, depth),
   errval_(0),
   errstr_(errstr),
   offset_(offset)
{
   Debug::ft(SoftwareException_ctor2);
}

//------------------------------------------------------------------------------

fn_name SoftwareException_dtor = "SoftwareException.dtor";

SoftwareException::~SoftwareException() noexcept
{
   Debug::ft(SoftwareException_dtor);
}

//------------------------------------------------------------------------------

void SoftwareException::Display(ostream& stream, const string& prefix) const
{
   if(errstr_.empty())
      stream << prefix << "errval : " << strHex(errval_) << CRLF;
   else
      stream << prefix << "errstr : " << errstr_ << CRLF;

   stream << prefix << "offset : " << strHex(offset_) << CRLF;
}

//------------------------------------------------------------------------------

fixed_string SoftwareExceptionExpl = "Software Abort";

const char* SoftwareException::what() const noexcept
{
   return SoftwareExceptionExpl;
}
}
