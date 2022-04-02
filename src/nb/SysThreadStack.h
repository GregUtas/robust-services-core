//==============================================================================
//
//  SysThreadStack.h
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
#ifndef SYSTHREADSTACK_H_INCLUDED
#define SYSTHREADSTACK_H_INCLUDED

#include <iosfwd>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: thread stack disassembly.
//
namespace SysThreadStack
{
   //  Loads symbol information during startup.
   //
   void Startup(RestartLevel level);

   //  Unloads symbol information during shutdown.
   //
   void Shutdown(RestartLevel level);

   //  Returns the depth (on the stack) of the calling function.
   //
   fn_depth FuncDepth();

   //  Writes the current thread's call stack into STREAM.
   //
   void Display(std::ostream& stream);

   //  Returns false if a destructor is found on the stack.
   //
   bool TrapIsOk();
}
}
#endif
