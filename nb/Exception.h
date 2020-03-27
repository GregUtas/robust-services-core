//==============================================================================
//
//  Exception.h
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
#ifndef EXCEPTION_H_INCLUDED
#define EXCEPTION_H_INCLUDED

#include <exception>
#include <iosfwd>
#include <sstream>
#include <string>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  All exceptions should be subclassed from this.  It is virtual and cannot
//  be thrown itself, but it ensures that all subclasses capture the function
//  call stack for debugging purposes.
//
class Exception : public std::exception
{
public:
   //  Deleted to prohibit assignment.
   //
   Exception& operator=(const Exception& that) = delete;
   Exception& operator=(const Exception&& that) = delete;

   //  Outputs information about the exception in STREAM.  INDENT specifies
   //  how far to indent the output.  The implementation provided here does
   //  nothing because the call stack is output separately.
   //
   virtual void Display(std::ostream& stream, const std::string& prefix) const;

   //  Returns the stream that contains the call stack.
   //
   std::ostringstream* Stack() const { return stack_.get(); }
protected:
   //  Captures the call stack in stack_ if STACK is true.  DEPTH is the level
   //  of subclassing (1 for a direct subclass from Exception), which is used
   //  to omit exception constructors from the call stack.  Protected because
   //  this class is virtual.
   //
   Exception(bool stack, fn_depth depth);

   //  Exceptions need to be copyable, so transfer ownership of stack_.
   //
   Exception(const Exception& that);
   Exception(Exception&& that);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~Exception();

   //  Overridden so that Thread::Start can catch this exception.  Subclasses
   //  should override this implementation.
   //
   const char* what() const noexcept override;
private:
   //  The function call stack at the time that the exception occurred.
   //  Mutable so that the copy constructor can transfer ownership if
   //  an exception is caught and rethrown.
   //
   mutable ostringstreamPtr stack_;
};
}
#endif
