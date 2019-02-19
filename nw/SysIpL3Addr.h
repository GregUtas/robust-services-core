//==============================================================================
//
//  SysIpL3Addr.h
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
#ifndef SYSIPL3ADDR_H_INCLUDED
#define SYSIPL3ADDR_H_INCLUDED

#include "SysIpL2Addr.h"
#include <string>
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Operating system abstraction layer: layer 3 IP address.
//
class SysIpL3Addr : public SysIpL2Addr
{
public:
   //  Constructs a nil address.
   //
   SysIpL3Addr();

   //  Constructs an address from v4Addr, PORT, PROTO, and SOCKET.  If SOCKET
   //  is valid, it determines PROTO.
   //
   SysIpL3Addr(ipv4addr_t v4Addr, ipport_t port,
      IpProtocol proto = IpAny, SysTcpSocket* socket = nullptr);

   //  Constructs an address from L2ADDR, PORT, PROTO, and SOCKET.  If SOCKET
   //  is valid, it determines PROTO.
   //
   SysIpL3Addr(const SysIpL2Addr& l2Addr, ipport_t port,
      IpProtocol proto = IpAny, SysTcpSocket* socket = nullptr);

   //  Constructs an address for the host identified by NAME.  SERVICE may be
   //  a port number or the name of a service associated with a well-known
   //  port.  PROTO is updated to the service's protocol.  Failure can be
   //  detected using SysIpL2Addr::IsValid.
   //
   SysIpL3Addr
      (const std::string& name, const std::string& service, IpProtocol& proto);

   //  Copy constructor.
   //
   SysIpL3Addr(const SysIpL3Addr& that);

   //  Copy operator.
   //
   SysIpL3Addr& operator=(const SysIpL3Addr& that);

   //  Virtual to allow subclassing.
   //
   virtual ~SysIpL3Addr();

   //  Sets the socket for the address.
   //
   void SetSocket(SysTcpSocket* socket);

   //  Returns the port.
   //
   ipport_t GetPort() const { return port_; }

   //  Returns the protocol.
   //
   IpProtocol GetProtocol() const { return proto_; }

   //  Returns the dedicated socket assigned to the address.
   //
   SysTcpSocket* GetSocket() const { return socket_; }

   //  If the address has a dedicated socket_, releases it and sets
   //  it to nullptr.
   //
   void ReleaseSocket();

   //  Updates NAME and SERVICE to the standard host name and port
   //  service name of the host identified by this address.
   //
   bool AddrToName(std::string& name, std::string& service) const;

   //  Returns the address as a string ("a.b.c.d:port").
   //
   virtual std::string to_str() const override;

   //  The same as to_str(), but also displays proto_ and socket_ unless
   //  both still have their default values (IpAny and nullptr).
   //
   std::string to_string() const;

   //  Returns true if the IP addresses and ports match.
   //
   bool operator==(const SysIpL3Addr& that) const;

   //  Returns the inverse of the == operator.
   //
   bool operator!=(const SysIpL3Addr& that) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  The port number associated with the address.
   //
   ipport_t port_ : 16;

   //  The protocol requested for the port.
   //
   IpProtocol proto_ : 16;

   //  The port's dedicated socket, if any.
   //
   SysTcpSocket* socket_;
};
}
#endif
