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

#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Provides analogs for the std::string functions used on function
//  names (fn_name).
//
namespace FunctionName
{
   //  Returns the first location of STR in FUNC, else string::npos.
   //
   size_t find(fn_name_arg func, const char* str);

   //  Returns the last location of STR in FUNC, else string::npos.
   //
   size_t rfind(fn_name_arg func, const char* str);

   //  Returns -1, 0, or 1 if FUNC is less than, equal to, or greater
   //  than STR.
   //
   int compare(fn_name_arg func, const char* str);

   //  Constants for tools.  The "tags" appear as substrings in the
   //  types of functions indicated.
   //
   extern fixed_string TypeStr;   // type for a function name
   extern fixed_string CtorTag;   // in a constructor
   extern fixed_string DtorTag;   // in a destructor
   extern fixed_string OpNewTag;  // in operator new
   extern fixed_string OpDelTag;  // in operator delete
}
}
#endif
