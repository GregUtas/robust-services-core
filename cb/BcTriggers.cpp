//==============================================================================
//
//  BcTriggers.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "BcSessions.h"
#include "Debug.h"

//------------------------------------------------------------------------------

namespace CallBase
{
fn_name BcTrigger_ctor = "BcTrigger.ctor";

BcTrigger::BcTrigger(Id tid) : Trigger(tid)
{
   Debug::ft(BcTrigger_ctor);
}

fn_name BcTrigger_dtor = "BcTrigger.dtor";

BcTrigger::~BcTrigger()
{
   Debug::ft(BcTrigger_dtor);
}
}