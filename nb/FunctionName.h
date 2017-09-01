//==============================================================================
//
//  FunctionName.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
