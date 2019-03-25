//==============================================================================
//
//  TlvParameter.cpp
//
//  Copyright (C) 2017  Greg Utas
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
#include "TlvParameter.h"
#include "Debug.h"
#include "TlvMessage.h"

using namespace NodeBase;

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
