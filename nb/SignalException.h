//==============================================================================
//
//  SignalException.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   SignalException(signal_t sig, debug32_t errval);

   //  Not subclassed.
   //
   ~SignalException() noexcept;

   //  Returns the signal that occurred.
   //
   signal_t GetSignal() const { return signal_; }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   virtual const char* what() const noexcept override;

   //  The actual signal (e.g. SIGSEGV).
   //
   const signal_t signal_;

   //  An error value for debugging.
   //
   const debug32_t errval_;
};
}
#endif
