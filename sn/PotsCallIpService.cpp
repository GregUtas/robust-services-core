//==============================================================================
//
//  PotsCallIpService.cpp
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
#include "PotsSessions.h"
#include "CliText.h"
#include <string>
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "IpPortCfgParm.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCallServiceText : public CliText
{
public: PotsCallServiceText();
};

fixed_string PotsCallServiceStr = "POTS Call/UDP";
fixed_string PotsCallServiceExpl = "POTS Call Protocol";

PotsCallServiceText::PotsCallServiceText() :
   CliText(PotsCallServiceStr, PotsCallServiceExpl) { }

//------------------------------------------------------------------------------

fixed_string PotsCallIpPortKey = "PotsCallIpPort";
fixed_string PotsCallIpPortExpl = "POTS Call Protocol: UDP port";

fn_name PotsCallIpService_ctor = "PotsCallIpService.ctor";

PotsCallIpService::PotsCallIpService() : port_(NilIpPort)
{
   Debug::ft(PotsCallIpService_ctor);

   auto port = std::to_string(PotsCallIpPort);
   cfgPort_.reset(new IpPortCfgParm
      (PotsCallIpPortKey, port.c_str(), &port_, PotsCallIpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*cfgPort_);
}

//------------------------------------------------------------------------------

fn_name PotsCallIpService_dtor = "PotsCallIpService.dtor";

PotsCallIpService::~PotsCallIpService()
{
   Debug::ft(PotsCallIpService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCallIpService_CreateHandler = "PotsCallIpService.CreateHandler";

InputHandler* PotsCallIpService::CreateHandler(IpPort* port) const
{
   Debug::ft(PotsCallIpService_CreateHandler);

   return new PotsCallHandler(port);
}

//------------------------------------------------------------------------------

fn_name PotsCallIpService_CreateText = "PotsCallIpService.CreateText";

CliText* PotsCallIpService::CreateText() const
{
   Debug::ft(PotsCallIpService_CreateText);

   return new PotsCallServiceText;
}
}
