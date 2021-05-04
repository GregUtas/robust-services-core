//==============================================================================
//
//  SignalException.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef SIGNALEXCEPTION_H_INCLUDED
#define SIGNALEXCEPTION_H_INCLUDED

#include "Exception.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Thread::SignalHandler throws this to handle a signal such as SIGSEGV.
//
class SignalException : public Exception
{
public:
   //  SIG is the signal that occurred.  ERRVAL is for debugging.
   //
   SignalException(signal_t sig, debug64_t errval);

   //  Not subclassed.
   //
   ~SignalException();

   //  Returns the signal that occurred.
   //
   signal_t GetSignal() const { return signal_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   const char* what() const noexcept override;

   //  The actual signal (e.g. SIGSEGV).
   //
   const signal_t signal_;

   //  An error value for debugging.
   //
   const debug64_t errval_;
};
}
#endif
