//==============================================================================
//
//  UdpIpService.cpp
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
#include "UdpIpService.h"
#include "Debug.h"
#include "SysTypes.h"
#include "UdpIpPort.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name UdpIpService_ctor = "UdpIpService.ctor";

UdpIpService::UdpIpService()
{
   Debug::ft(UdpIpService_ctor);
}

//------------------------------------------------------------------------------

fn_name UdpIpService_dtor = "UdpIpService.dtor";

UdpIpService::~UdpIpService()
{
   Debug::ftnt(UdpIpService_dtor);
}

//------------------------------------------------------------------------------

fn_name UdpIpService_CreatePort = "UdpIpService.CreatePort";

IpPort* UdpIpService::CreatePort(ipport_t pid)
{
   Debug::ft(UdpIpService_CreatePort);

   return new UdpIpPort(pid, this);
}

//------------------------------------------------------------------------------

void UdpIpService::Patch(sel_t selector, void* arguments)
{
   IpService::Patch(selector, arguments);
}
}
