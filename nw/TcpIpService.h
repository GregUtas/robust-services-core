//==============================================================================
//
//  TcpIpService.h
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
#ifndef TCPIPSERVICE_H_INCLUDED
#define TCPIPSERVICE_H_INCLUDED

#include "IpService.h"
#include <cstddef>
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
class TcpIpService : public IpService
{
public:
   //  Returns true if the service implements a server capability.
   //  Overridden to return false if the service does not accept
   //  incoming connections.
   //
   virtual bool AcceptsConns() const { return true; }

   //  Returns the maximum number of simultaneous connections for
   //  the service's I/O thread.
   //
   virtual size_t MaxConns() const = 0;

   //  Returns the maximum number of connection requests that can
   //  be queued for the service's I/O thread.
   //
   virtual size_t MaxBacklog() const = 0;

   //  Returns true if keepalive messages should be used.
   //
   virtual bool Keepalive() const { return false; }

   //  Overridden to indicate that this service runs over TCP.
   //
   IpProtocol Protocol() const override { return IpTcp; }

   //  Overridden to indicate that applications do not use the I/O
   //  thread's primary socket (the listener socket, if it exists)
   //  when sending messages.
   //
   bool HasSharedSocket() const override { return false; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
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
   IpPort* CreatePort(ipport_t pid) override;
};
}
#endif
