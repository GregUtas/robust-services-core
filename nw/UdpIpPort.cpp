//==============================================================================
//
//  UdpIpPort.cpp
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
#include "UdpIpPort.h"
#include "Debug.h"
#include "NwDaemons.h"
#include "SysTypes.h"
#include "UdpIoThread.h"
#include "UdpIpService.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name UdpIpPort_ctor = "UdpIpPort.ctor";

UdpIpPort::UdpIpPort(ipport_t port, const IpService* service) :
   IpPort(port, service)
{
   Debug::ft(UdpIpPort_ctor);
}

//------------------------------------------------------------------------------

fn_name UdpIpPort_dtor = "UdpIpPort.dtor";

UdpIpPort::~UdpIpPort()
{
   Debug::ft(UdpIpPort_dtor);
}

//------------------------------------------------------------------------------

fn_name UdpIpPort_CreateIoThread = "UdpIpPort.CreateIoThread";

IoThread* UdpIpPort::CreateIoThread()
{
   Debug::ft(UdpIpPort_CreateIoThread);

   auto svc = static_cast< const UdpIpService* >(GetService());
   auto daemon = UdpIoDaemon::GetDaemon(svc, GetPort());
   return new UdpIoThread(daemon, svc, GetPort());
}

//------------------------------------------------------------------------------

void UdpIpPort::Patch(sel_t selector, void* arguments)
{
   IpPort::Patch(selector, arguments);
}
}
