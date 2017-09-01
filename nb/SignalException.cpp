//==============================================================================
//
//  SignalException.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

SignalException::SignalException(signal_t sig, debug32_t errval) :
   Exception(true, 1),
   signal_(sig),
   errval_(errval)
{
   Debug::ft(SignalException_ctor);
}

//------------------------------------------------------------------------------

fn_name SignalException_dtor = "SignalException.dtor";

SignalException::~SignalException() noexcept
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
