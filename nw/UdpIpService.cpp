//==============================================================================
//
//  UdpIpService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "UdpIpService.h"
#include "Debug.h"
#include "SysTypes.h"
#include "UdpIpPort.h"

//------------------------------------------------------------------------------

namespace NodeBase
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
   Debug::ft(UdpIpService_dtor);
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