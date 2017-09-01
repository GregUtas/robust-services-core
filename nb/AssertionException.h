//==============================================================================
//
//  AssertionException.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ASSERTIONEXCEPTION_H_INCLUDED
#define ASSERTIONEXCEPTION_H_INCLUDED

#include "Exception.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Debug::Assert throws this when an assertion fails.
//
class AssertionException : public Exception
{
public:
   //  Invokes Exception's constructor to capture the stack.
   //  ERRVAL is for debugging.
   //
   explicit AssertionException(debug32_t errval);

   //  Not subclassed.
   //
   ~AssertionException() noexcept;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   virtual const char* what() const noexcept override;

   //  An error value for debugging.
   //
   const debug32_t errval_;
};
}
#endif
