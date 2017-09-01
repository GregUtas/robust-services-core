//==============================================================================
//
//  InitFlags.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "InitFlags.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
bool InitFlags::AllowBreak()
{
#ifdef FIELD_LOAD
   return false;
#else
   return true;
#endif
}

//------------------------------------------------------------------------------

bool InitFlags::CauseTimeout()
{
   //  If this returns true, AllowBreak must return false.
   //
   return false;
}

//------------------------------------------------------------------------------

bool InitFlags::ImmediateTrace()
{
   //  This has no effect unless TraceInit or TraceWork returns true.
   //
   return false;
}

//------------------------------------------------------------------------------

bool InitFlags::TraceInit()
{
#ifdef FIELD_LOAD
   return false;
#else
   return true;
#endif
}

//------------------------------------------------------------------------------

bool InitFlags::TraceWork()
{
   return false;
}
}