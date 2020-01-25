//==============================================================================
//
//  FunctionName.h
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
#ifndef FUNCTIONNAME_H_INCLUDED
#define FUNCTIONNAME_H_INCLUDED

#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Provides analogs for the std::string functions used on function names
//  (fn_name).
//
namespace FunctionName
{
   //  Constants for tools.  The "tags" appear as substrings in the
   //  types of functions indicated.
   //
   extern fixed_string TypeStr;   // type for a function name
   extern fixed_string CtorTag;   // in a constructor
   extern fixed_string DtorTag;   // in a destructor
   extern fixed_string OpNewTag;  // in a new operator
   extern fixed_string OpDelTag;  // in a delete operator
}
}
#endif
