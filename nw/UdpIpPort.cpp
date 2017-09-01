//==============================================================================
//
//  UdpIpPort.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "UdpIpPort.h"
#include "Debug.h"
#include "IpService.h"
#include "SysTypes.h"
#include "UdpIoThread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name UdpIpPort_ctor = "UdpIpPort.ctor";

UdpIpPort::UdpIpPort(ipport_t port, IpService* service) : IpPort(port, service)
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

   auto svc = GetService();

   return new UdpIoThread
      (svc->GetFaction(), GetPort(), svc->RxSize(), svc->TxSize());
}

//------------------------------------------------------------------------------

void UdpIpPort::Patch(sel_t selector, void* arguments)
{
   IpPort::Patch(selector, arguments);
}
}