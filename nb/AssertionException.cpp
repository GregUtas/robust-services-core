//==============================================================================
//
//  AssertionException.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "AssertionException.h"
#include <ostream>
#include <string>
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name AssertionException_ctor = "AssertionException.ctor";

AssertionException::AssertionException(debug32_t errval) : Exception(true, 1),
   errval_(errval)
{
   Debug::ft(AssertionException_ctor);
}

//------------------------------------------------------------------------------

fn_name AssertionException_dtor = "AssertionException.dtor";

AssertionException::~AssertionException() noexcept
{
   Debug::ft(AssertionException_dtor);
}

//------------------------------------------------------------------------------

void AssertionException::Display(ostream& stream, const string& prefix) const
{
   Exception::Display(stream, prefix);

   stream << prefix << "errval : " << errval_ << CRLF;
}

//------------------------------------------------------------------------------

fixed_string AssertionExceptionExpl = "Assertion Failed";

const char* AssertionException::what() const noexcept
{
   return AssertionExceptionExpl;
}
}
