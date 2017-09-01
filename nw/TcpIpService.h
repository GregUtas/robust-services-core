//==============================================================================
//
//  TcpIpService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TCPIPSERVICE_H_INCLUDED
#define TCPIPSERVICE_H_INCLUDED

#include "IpService.h"
#include <cstddef>
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class TcpIpService : public IpService
{
public:
   //  Returns the maximum number of simultaneous connections for
   //  the service's I/O thread.
   //
   virtual size_t MaxConns() const = 0;

   //  Returns the maximum number of connection requests that can
   //  be queued for the service's I/O thread.
   //
   virtual size_t MaxBacklog() const = 0;

   //  Overridden to indicate that this service runs over TCP.
   //
   virtual IpProtocol Protocol() const override { return IpTcp; }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a service that runs over TCP.  Protected because
   //  this class is virtual.
   //
   TcpIpService();

   //  Protected because subclasses should be singletons.
   //
   virtual ~TcpIpService();

   //  Overridden to create a PORT for PID.
   //
   virtual IpPort* CreatePort(ipport_t pid) override;
};
}
#endif
