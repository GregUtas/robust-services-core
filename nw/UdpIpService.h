//==============================================================================
//
//  UdpIpService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef UDPIPSERVICE_H_INCLUDED
#define UDPIPSERVICE_H_INCLUDED

#include "IpService.h"
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class UdpIpService : public IpService
{
public:
   //  Overridden to indicate that this service runs over UDP.
   //
   virtual IpProtocol Protocol() const override { return IpUdp; }

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a service that runs over UDP.  Protected because
   //  this class is virtual.
   //
   UdpIpService();

   //  Protected because subclasses should be singletons.
   //
   virtual ~UdpIpService();

   //  Overridden to create a PORT for PID.
   //
   virtual IpPort* CreatePort(ipport_t pid) override;
};
}
#endif