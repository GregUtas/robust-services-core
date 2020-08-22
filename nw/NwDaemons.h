//==============================================================================
//
//  NwDaemons.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef NWDAEMONS_H_INCLUDED
#define NWDAEMONS_H_INCLUDED

#include "Daemon.h"
#include <string>
#include "NwTypes.h"
#include "SysTypes.h"

namespace NetworkBase
{
   class TcpIpService;
   class UdpIpService;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Daemons for managing I/O threads.
//
extern NodeBase::fixed_string TcpIoDaemonName;

class TcpIoDaemon : public NodeBase::Daemon
{
public:
   //  Finds/creates the daemon that manages the TCP I/O thread that receives
   //  messages on PORT on behalf of SERVICE.
   //
   static TcpIoDaemon* GetDaemon(const TcpIpService* service, ipport_t port);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates a daemon that manages the TCP I/O thread that receives messages
   //  on PORT on behalf of SERVICE.
   //
   TcpIoDaemon(const TcpIpService* service, ipport_t port);

   //  Not subclassed.
   //
   ~TcpIoDaemon();

   //  Returns the name for the daemon that manages the TCP I/O thread on PORT.
   //
   static std::string MakeName(ipport_t port);

   //  Overridden to create a TCP I/O thread.
   //
   NodeBase::Thread* CreateThread() override;

   //  The service for the TCP I/O thread.
   //
   const TcpIpService* const service_;

   //  The port for the TCP I/O thread.
   //
   const ipport_t port_;
};

//------------------------------------------------------------------------------

extern NodeBase::fixed_string UdpIoDaemonName;

class UdpIoDaemon : public NodeBase::Daemon
{
public:
   //  Finds/creates the daemon that manages the UDP I/O thread that receives
   //  messages on PORT on behalf of SERVICE.
   //
   static UdpIoDaemon* GetDaemon(const UdpIpService* service, ipport_t port);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates a daemon that manages the UDP I/O thread that receives messages
   //  on PORT on behalf of SERVICE.
   //
   UdpIoDaemon(const UdpIpService* service, ipport_t port);

   //  Not subclassed.
   //
   ~UdpIoDaemon();

   //  Returns the name for the daemon that manages the UDP I/O thread on PORT.
   //
   static std::string MakeName(ipport_t port);

   //  Overridden to create a UDP I/O thread.
   //
   NodeBase::Thread* CreateThread() override;

   //  The service for the UDP I/O thread.
   //
   const UdpIpService* const service_;

   //  The port for the UDP I/O thread.
   //
   const ipport_t port_;
};
}
#endif
