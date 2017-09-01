//==============================================================================
//
//  SysSignals.win.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifdef OS_WIN
#include "SysSignals.h"
#include "Debug.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysSignals_CreateNativeSignals = "SysSignals.CreateNativeSignals";

void SysSignals::CreateNativeSignals()
{
   Debug::ft(SysSignals_CreateNativeSignals);

   Singleton< SigAbort >::Instance();
   Singleton< SigBreak >::Instance();
   Singleton< SigFpe >::Instance();
   Singleton< SigIll >::Instance();
   Singleton< SigInt >::Instance();
   Singleton< SigSegv >::Instance();
   Singleton< SigTerm >::Instance();
}
}
#endif