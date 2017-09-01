//==============================================================================
//
//  UdpIpPort.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef UDPIPPORT_H_INCLUDED
#define UDPIPPORT_H_INCLUDED

#include "IpPort.h"
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  An IP port that supports a UDP-based protocol.
//
class UdpIpPort : public IpPort
{
public:
   //  See IpPort's constructor.
   //
   UdpIpPort(ipport_t port, IpService* service);

   //  Not subclassed.
   //
   ~UdpIpPort();

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to create a UdpIoThread for the port.
   //
   virtual IoThread* CreateIoThread() override;
};
}
#endif
