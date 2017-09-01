//==============================================================================
//
//  ElementException.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ElementException.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name ElementException_ctor = "ElementException.ctor";

ElementException::ElementException(reinit_t reason, debug32_t errval) :
   Exception(true, 1),
   reason_(reason),
   errval_(errval)
{
   Debug::ft(ElementException_ctor);
}

//------------------------------------------------------------------------------

fn_name ElementException_dtor = "ElementException.dtor";

ElementException::~ElementException() noexcept
{
   Debug::ft(ElementException_dtor);
}

//------------------------------------------------------------------------------

void ElementException::Display(ostream& stream, const string& prefix) const
{
   Exception::Display(stream, prefix);

   stream << prefix << "reason : " << strHex(reason_) << CRLF;
   stream << prefix << "errval : " << strHex(errval_) << CRLF;
}

//------------------------------------------------------------------------------

fixed_string ElementExceptionExpl = "Fatal Exception";

const char* ElementException::what() const noexcept
{
   return ElementExceptionExpl;
}
}
