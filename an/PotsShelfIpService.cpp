//==============================================================================
//
//  PotsShelfIpService.cpp
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
#include "PotsShelf.h"
#include "CliText.h"
#include <string>
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "IpPortCfgParm.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsShelfServiceText : public CliText
{
public: PotsShelfServiceText();
};

fixed_string PotsShelfServiceStr = "POTS Shelf/UDP";
fixed_string PotsShelfServiceExpl = "POTS Shelf Protocol";

PotsShelfServiceText::PotsShelfServiceText() :
   CliText(PotsShelfServiceStr, PotsShelfServiceExpl) { }

//------------------------------------------------------------------------------

fixed_string PotsShelfIpPortKey = "PotsShelfIpPort";
fixed_string PotsShelfIpPortExpl = "POTS Shelf Protocol: UDP port";

fn_name PotsShelfIpService_ctor = "PotsShelfIpService.ctor";

PotsShelfIpService::PotsShelfIpService() : port_(NilIpPort)
{
   Debug::ft(PotsShelfIpService_ctor);

   auto port = std::to_string(PotsShelfIpPort);
   portCfg_.reset(new IpPortCfgParm
      (PotsShelfIpPortKey, port.c_str(), &port_, PotsShelfIpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*portCfg_);
}

//------------------------------------------------------------------------------

fn_name PotsShelfIpService_dtor = "PotsShelfIpService.dtor";

PotsShelfIpService::~PotsShelfIpService()
{
   Debug::ft(PotsShelfIpService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsShelfIpService_CreateHandler = "PotsShelfIpService.CreateHandler";

InputHandler* PotsShelfIpService::CreateHandler(IpPort* port) const
{
   Debug::ft(PotsShelfIpService_CreateHandler);

   return new PotsShelfHandler(port);
}

//------------------------------------------------------------------------------

fn_name PotsShelfIpService_CreateText = "PotsShelfIpService.CreateText";

CliText* PotsShelfIpService::CreateText() const
{
   Debug::ft(PotsShelfIpService_CreateText);

   return new PotsShelfServiceText;
}
}
