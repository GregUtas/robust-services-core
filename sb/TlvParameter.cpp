//==============================================================================
//
//  TlvParameter.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "TlvParameter.h"
#include "Debug.h"
#include "TlvMessage.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name TlvParameter_ctor = "TlvParameter.ctor";

TlvParameter::TlvParameter(ProtocolId prid, Id pid) : Parameter(prid, pid)
{
   Debug::ft(TlvParameter_ctor);
}

//------------------------------------------------------------------------------

fn_name TlvParameter_dtor = "TlvParameter.dtor";

TlvParameter::~TlvParameter()
{
   Debug::ft(TlvParameter_dtor);
}

//------------------------------------------------------------------------------

fn_name TlvParameter_ExtractPid = "TlvParameter.ExtractPid";

Parameter::Id TlvParameter::ExtractPid(const TlvParmLayout& parm)
{
   Debug::ft(TlvParameter_ExtractPid);

   return parm.header.pid;
}

//------------------------------------------------------------------------------

void TlvParameter::Patch(sel_t selector, void* arguments)
{
   Parameter::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name TlvParameter_VerifyMsg = "TlvParameter.VerifyMsg";

Parameter::TestRc TlvParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(TlvParameter_VerifyMsg);

   auto& tmsg = static_cast< const TlvMessage& >(msg);
   auto pid = Pid();
   auto pptr = tmsg.FindParm(pid);

   if((pptr == nullptr) && (use == Mandatory))
      return MessageMissingMandatoryParm;
   if((pptr != nullptr) && (use == Illegal))
      return MessageContainsIllegalParm;
   return Ok;
}
}