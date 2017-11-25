//==============================================================================
//
//  FunctionName.cpp
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
#include "FunctionName.h"
#include <cstring>
#include <string>

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string FunctionName::TypeStr = "fn_name";
fixed_string FunctionName::CtorTag = ".ctor";
fixed_string FunctionName::DtorTag = ".dtor";
fixed_string FunctionName::OpNewTag = ".operator new";
fixed_string FunctionName::OpDelTag = ".operator delete";

//------------------------------------------------------------------------------

int FunctionName::compare(fn_name_arg func, const char* str)
{
   return strcmp(func, str);
}

//------------------------------------------------------------------------------

size_t FunctionName::find(fn_name_arg func, const char* str)
{
   auto begin = strstr(func, str);
   if(begin == nullptr) return string::npos;
   return (begin - func);
}

//------------------------------------------------------------------------------

size_t FunctionName::rfind(fn_name_arg func, const char* str)
{
   string name(func);
   return name.rfind(str);
}
}
