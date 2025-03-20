//==============================================================================
//
//  AssertionException.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
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
   explicit AssertionException(debug64_t errval);

   //  Not subclassed.
   //
   ~AssertionException();

   //  Copy constructor.
   //
   AssertionException(const AssertionException& that) = default;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const override;
private:
   //  Overridden to identify the type of exception.
   //
   const char* what() const noexcept override;

   //  An error value for debugging.
   //
   const debug64_t errval_;
};
}
#endif
