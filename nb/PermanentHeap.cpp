//==============================================================================
//
//  PermanentHeap.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PermanentHeap.h"
#include "Debug.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name PermanentHeap_ctor = "PermanentHeap.ctor";

PermanentHeap::PermanentHeap() : SysHeap(MemPerm, 0)
{
   Debug::ft(PermanentHeap_ctor);
}

//------------------------------------------------------------------------------

fn_name PermanentHeap_dtor = "PermanentHeap.dtor";

PermanentHeap::~PermanentHeap()
{
   Debug::ft(PermanentHeap_dtor);
}

//------------------------------------------------------------------------------

PermanentHeap* PermanentHeap::Instance()
{
   static PermanentHeap heap;

   return &heap;
}
}