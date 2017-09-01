//==============================================================================
//
//  SysThreadStack.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

   //  Writes the current thread's call stack into STREAM.  OMIT specifies
   //  the number of functions to omit (other than this one).
   //
   void Display(std::ostream& stream, fn_depth omit);

   //  Returns true if a destructor is found on the stack.
   //
   bool TrapIsOk();
}
}
#endif
