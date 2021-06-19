//==============================================================================
//
//  UdpIpService.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef UDPIPSERVICE_H_INCLUDED
#define UDPIPSERVICE_H_INCLUDED

#include "IpService.h"
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
class UdpIpService : public IpService
{
public:
   //  Overridden to indicate that applications share the I/O
   //  thread's primary socket when sending messages.
   //
   bool HasSharedSocket() const override { return true; }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to indicate that this service runs over UDP.
   //
   IpProtocol Protocol() const override { return IpUdp; }
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
   IpPort* CreatePort(ipport_t pid) override;
};
}
#endif
