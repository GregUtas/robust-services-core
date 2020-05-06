//==============================================================================
//
//  SignalException.cpp
//
//  Copyright (C) 2017  Greg Utas
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
#include "SignalException.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "PosixSignalRegistry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SignalException_ctor = "SignalException.ctor";

SignalException::SignalException(signal_t sig, debug64_t errval) :
   Exception(true, 1),
   signal_(sig),
   errval_(errval)
{
   Debug::ft(SignalException_ctor);
}

//------------------------------------------------------------------------------

fn_name SignalException_dtor = "SignalException.dtor";

SignalException::~SignalException()
{
   Debug::ft(SignalException_dtor);
}

//------------------------------------------------------------------------------

void SignalException::Display(ostream& stream, const string& prefix) const
{
   Exception::Display(stream, prefix);

   auto reg = Singleton< PosixSignalRegistry >::Instance();

   stream << prefix << "signal : " << reg->strSignal(signal_) << CRLF;
   stream << prefix << "errval : " << strHex(errval_) << CRLF;
}

//------------------------------------------------------------------------------

fixed_string SignalExceptionExpl = "Signal";

const char* SignalException::what() const noexcept
{
   return SignalExceptionExpl;
}
}
