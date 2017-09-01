//==============================================================================
//
//  SoftwareException.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SOFTWAREEXCEPTION_H_INCLUDED
#define SOFTWAREEXCEPTION_H_INCLUDED

#include "Exception.h"
#include <string>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Debug::SwErr throws this when an application decides to abort work in
//  progress.
//
class SoftwareException : public Exception
{
public:
   //  ERRVAL/ERRSTR and OFFSET are the arguments to Debug::SwErr.
   //
   SoftwareException
      (debug64_t errval, debug32_t offset, fn_depth depth = 1);
   SoftwareException
      (const std::string& errstr, debug32_t offset, fn_depth depth = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~SoftwareException() noexcept;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   virtual const char* what() const noexcept override;

   //  An error valuefor debugging.
   //
   const debug64_t errval_;

   //  A string for debugging.
   //
   const std::string errstr_;

   //  A location or additional value associated with the exception.
   //
   const debug32_t offset_;
};
}
#endif
