//==============================================================================
//
//  ElementException.cpp
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

ElementException::ElementException
   (RestartLevel level, RestartReason reason, debug64_t errval) :
   Exception(true, 1),
   level_(level),
   reason_(reason),
   errval_(errval)
{
   Debug::ft(ElementException_ctor);
}

//------------------------------------------------------------------------------

fn_name ElementException_dtor = "ElementException.dtor";

ElementException::~ElementException()
{
   Debug::ftnt(ElementException_dtor);
}

//------------------------------------------------------------------------------

void ElementException::Display(ostream& stream, const string& prefix) const
{
   Exception::Display(stream, prefix);

   stream << prefix << "level  : " << level_ << CRLF;
   stream << prefix << "reason : " << reason_ << CRLF;
   stream << prefix << "errval : " << strHex(errval_) << CRLF;
}

//------------------------------------------------------------------------------

fixed_string ElementExceptionExpl = "Fatal Exception";

const char* ElementException::what() const noexcept
{
   return ElementExceptionExpl;
}
}
