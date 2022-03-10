//==============================================================================
//
//  SoftwareException.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef SOFTWAREEXCEPTION_H_INCLUDED
#define SOFTWAREEXCEPTION_H_INCLUDED

#include "Exception.h"
#include <string>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  An application throws this when it decides to abort work in progress.
//
class SoftwareException : public Exception
{
public:
   //  ERRVAL/ERRSTR and OFFSET provide debugging information, the same as
   //  for Debug::SwLog.
   //
   SoftwareException(debug64_t errval, debug64_t offset);
   SoftwareException(const std::string& errstr, debug64_t offset);

   //  Virtual to allow subclassing.
   //
   virtual ~SoftwareException();

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

   //  A string for debugging.
   //
   const std::string errstr_;

   //  A location or additional value associated with the exception.
   //
   const debug64_t offset_;
};
}
#endif
