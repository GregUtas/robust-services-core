//==============================================================================
//
//  Restart.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Restart.h"
#include "Debug.h"
#include "ElementException.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
RestartStatus Restart::Status_ = Initial;
RestartLevel Restart::Level_ = RestartNil;

//------------------------------------------------------------------------------

fn_name Restart_Initiate = "Restart.Initiate";

void Restart::Initiate(reinit_t reason, debug32_t errval)
{
   Debug::ft(Restart_Initiate);

   throw ElementException(reason, errval);
}
}