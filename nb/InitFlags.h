//==============================================================================
//
//  InitFlags.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef INITFLAGS_H_INCLUDED
#define INITFLAGS_H_INCLUDED

//------------------------------------------------------------------------------

namespace NodeBase
{
//  This supports debugging during initialization.  To change the value
//  of a flag, change the implementation of the static member function
//  that returns its value.
//
namespace InitFlags
{
   //  Return true to allow breakpoints during initialization.  If this
   //  returns false and a breakpoint is hit, RootThread will time out
   //  and try to restart initialization.
   //
   bool AllowBreak();  // default=true

   //  Return true to test the initialization timeout by delaying
   //  indefinitely during initialization.
   //
   bool CauseTimeout();  // default=false

   //  Return true to immediately write trace records to a file, as they
   //  created, if TraceInit or TraceWork is also set.
   //
   bool ImmediateTrace();  // default=false

   //  Return true to trace initialization.
   //
   bool TraceInit();  // default=true

   //  Return true to trace payload work immediately after initialization.
   //
   bool TraceWork();  // default=true
}
}
#endif
