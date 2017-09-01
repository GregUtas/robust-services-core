//==============================================================================
//
//  ElementException.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ELEMENTEXCEPTION_H_INCLUDED
#define ELEMENTEXCEPTION_H_INCLUDED

#include "Exception.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Restart::Initiate throws this to reinitialize the element.
//
class ElementException : public Exception
{
public:
   //  REASON is one of the values defined in Restart.h.
   //  ERRVAL is for debugging.
   //
   ElementException(reinit_t reason, debug32_t errval);

   //  Not subclassed.
   //
   ~ElementException() noexcept;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix) const override;

   //  Returns the reason for the restart.
   //
   reinit_t Reason() const { return reason_; }

   //  Returns the error value.
   //
   debug32_t Errval() const { return errval_; }
private:
   //  Overridden to identify the type of exception.
   //
   virtual const char* what() const noexcept override;

   //  The reason for the restart.
   //
   const reinit_t reason_;

   //  An error value for debugging.
   //
   const debug32_t errval_;
};
}
#endif
