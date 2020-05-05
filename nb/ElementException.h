//==============================================================================
//
//  ElementException.h
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
   //  LEVEL is the restart severity.  REASON is one of the values defined
   //  in Restart.h. ERRVAL is for debugging.
   //
   ElementException(RestartLevel leve, reinit_t reason, debug64_t errval);

   //  Not subclassed.
   //
   ~ElementException();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const override;

   //  Returns the severity of the restart.
   //
   RestartLevel Level() const { return level_; }

   //  Returns the reason for the restart.
   //
   reinit_t Reason() const { return reason_; }

   //  Returns the error value.
   //
   debug64_t Errval() const { return errval_; }
private:
   //  Overridden to identify the type of exception.
   //
   const char* what() const noexcept override;

   //  The severity of the restart.
   //
   RestartLevel level_;

   //  The reason for the restart.
   //
   const reinit_t reason_;

   //  An error value for debugging.
   //
   const debug64_t errval_;
};
}
#endif
