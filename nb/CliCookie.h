//==============================================================================
//
//  CliCookie.h
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
#ifndef CLICOOKIE_H_INCLUDED
#define CLICOOKIE_H_INCLUDED

#include "Object.h"
#include <cstdint>
#include <vector>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Accesses the current location in a CLI command's parameter tree when
//  parsing the input stream.
//
class CliCookie : public Object
{
public:
   //  Public so that an instance can be declared as a member.
   //
   CliCookie();

   //  Not subclassed.
   //
   ~CliCookie();

   //  Initializes the cookie prior to parsing the next line in
   //  the input stream.
   //
   void Initialize();

   //  Returns the index associated with the parameter at DEPTH.
   //
   uint32_t Index(uint32_t depth) const;

   //  Proceeds to the next parameter at the same depth.
   //
   void Advance();

   //  Increases the parse depth when the current parameter has
   //  its own parameters.
   //
   void Descend();

   //  Increases the parse depth when the current parameter (at
   //  offset INDEX) has its own parameters.
   //
   void Descend(uint32_t index);

   //  Proceeds to the next parameter when all of the parameters
   //  at the current depth have been found.
   //
   void Ascend();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  The current location at each level in the parameter tree.
   //
   std::vector< uint32_t > index_;
};
}
#endif
