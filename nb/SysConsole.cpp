//==============================================================================
//
//  SysConsole.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SysConsole.h"
#include <ios>
#include <iostream>

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
std::istream& SysConsole::In()
{
   return std::cin;
}

//------------------------------------------------------------------------------

ostream& SysConsole::Out()
{
   std::cout << std::boolalpha << std::nouppercase;
   return std::cout;
}
}
