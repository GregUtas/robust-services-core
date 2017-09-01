//==============================================================================
//
//  AllocationException.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ALLOCATIONEXCEPTION_H_INCLUDED
#define ALLOCATIONEXCEPTION_H_INCLUDED

#include "Exception.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  This is thrown when memory allocation fails.  It replaces bad_alloc,
//  which does not capture the stack.
//
class AllocationException : public Exception
{
public:
   //  Invokes Exception's constructor to capture the stack.  TYPE and
   //  SIZE were the arguments to the failed Memory::Alloc.
   //
   AllocationException(MemoryType type, size_t size);

   //  Not subclassed.
   //
   ~AllocationException() noexcept;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   virtual const char* what() const noexcept override;

   //  The type of memory requested.
   //
   const MemoryType type_;

   //  The amount of memory requested.
   //
   const size_t size_;
};
}
#endif
