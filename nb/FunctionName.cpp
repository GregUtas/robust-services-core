//==============================================================================
//
//  FunctionName.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
