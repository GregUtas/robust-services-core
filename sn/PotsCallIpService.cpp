//==============================================================================
//
//  PotsCallIpService.cpp
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
#include "PotsSessions.h"
#include "CfgParmRegistry.h"
#include "CliText.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "IpService.h"
#include "Restart.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
fixed_string PotsCallUdpKey = "PotsCallUdp";
fixed_string PotsCallUdpExpl = "Create UDP I/O thread for POTS Call";

PotsCallIpService::PotsCallIpService()
{
   Debug::ft("PotsCallIpService.ctor");

   enabled_.reset
      (new CfgServiceParm(PotsCallUdpKey, "F", PotsCallUdpExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*enabled_);
}

//------------------------------------------------------------------------------

PotsCallIpService::~PotsCallIpService()
{
   Debug::ftnt("PotsCallIpService.dtor");
}

//------------------------------------------------------------------------------

InputHandler* PotsCallIpService::CreateHandler(IpPort* port) const
{
   Debug::ft("PotsCallIpService.CreateHandler");

   return new PotsCallHandler(port);
}

//------------------------------------------------------------------------------

fixed_string PotsCallServiceStr = "POTS Call/UDP";
fixed_string PotsCallServiceExpl = "POTS Call Protocol";

CliText* PotsCallIpService::CreateText() const
{
   Debug::ft("PotsCallIpService.CreateText");

   return new CliText(PotsCallServiceStr, PotsCallServiceExpl);
}

//------------------------------------------------------------------------------

bool PotsCallIpService::Enabled() const
{
   return enabled_->CurrValue();
}

//------------------------------------------------------------------------------

void PotsCallIpService::Shutdown(RestartLevel level)
{
   Debug::ft("PotsCallIpService.Shutdown");

   FunctionGuard guard(Guard_ImmUnprotect);
   Restart::Release(enabled_);

   IpService::Shutdown(level);
}

//------------------------------------------------------------------------------

void PotsCallIpService::Startup(RestartLevel level)
{
   Debug::ft("PotsCallIpService.Startup");

   if(enabled_ == nullptr)
   {
      FunctionGuard guard(Guard_ImmUnprotect);
      enabled_.reset
         (new CfgServiceParm(PotsCallUdpKey, "F", PotsCallUdpExpl, this));
      Singleton< CfgParmRegistry >::Instance()->BindParm(*enabled_);
   }

   IpService::Startup(level);
}
}
