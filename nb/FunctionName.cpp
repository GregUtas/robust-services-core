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

//------------------------------------------------------------------------------

int FunctionName::compare(fn_name_arg func, c_string str)
{
   return strcmp(func, str);
}

//------------------------------------------------------------------------------

size_t FunctionName::find(fn_name_arg func, c_string str)
{
   string name(func);
   return name.find(str);
}

//------------------------------------------------------------------------------

bool FunctionName::is_dtor(fn_name_arg func)
{
   //  This is invoked on every function capture, so it needs to be fast.
   //  It checks if the tail end of FUNC matches ".dtor".
   //
   static int DtorLen = strlen(DtorTag);

   int j = 0;

   for(int i = strlen(func) - DtorLen; (i >= 0) && (j < DtorLen); ++i, ++j)
   {
      if(func[i] != DtorTag[j]) return false;
   }

   return (j == DtorLen);
}

//------------------------------------------------------------------------------

size_t FunctionName::rfind(fn_name_arg func, c_string str)
{
   string name(func);
   return name.rfind(str);
}
}
