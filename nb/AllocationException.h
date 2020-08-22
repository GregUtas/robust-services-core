//==============================================================================
//
//  AllocationException.h
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
   ~AllocationException();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   const char* what() const noexcept override;

   //  The type of memory requested.
   //
   const MemoryType type_;

   //  The amount of memory requested.
   //
   const size_t size_;
};
}
#endif
