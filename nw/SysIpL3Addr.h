//==============================================================================
//
//  SysIpL3Addr.h
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
#ifndef SYSIPL3ADDR_H_INCLUDED
#define SYSIPL3ADDR_H_INCLUDED

#include "SysIpL2Addr.h"
#include <cstdint>
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

   //  Constructs an address from L2ADDR, PORT, PROTO, and SOCKET.  If SOCKET
   //  is valid, it determines PROTO.
   //
   SysIpL3Addr(const SysIpL2Addr& l2Addr, ipport_t port,
      IpProtocol proto = IpAny, SysTcpSocket* socket = nullptr);

   //  Constructs an IPv4 address from NETADDR, NETPORT, PROTO, and SOCKET.
   //  NETADDR and NETPORT must be in network order.  If SOCKET is valid,
   //  it determines PROTO.
   //
   SysIpL3Addr(IPv4Addr netaddr, ipport_t netport,
      IpProtocol proto = IpAny, SysTcpSocket* socket = nullptr);

   //  Constructs an IPv6 address from NETADDR, NETPORT, PROTO, and SOCKET.
   //  NETADDR's quartets and NETPORT must be in network order.  If SOCKET
   //  is valid, it determines PROTO.
   //
   SysIpL3Addr(const uint16_t netaddr[8], ipport_t netport,
      IpProtocol proto = IpAny, SysTcpSocket* socket = nullptr);

   //  Constructs an address from TEXT.  See the SysIpL2Addr(string) constructor
   //  for the format required for the layer 2 address.  A port is optional; if
   //  present, it appears as :p (in decimal) after the address.  If the address
   //  is IPv6, it must be enclosed in square brackets if :p follows.  Failure
   //  can be checked by invoking SysIpL2Addr::IsValid.
   //
   explicit SysIpL3Addr(const std::string& text);

   //  Constructs an address for the host identified by NAME.  SERVICE may be
   //  a port number or the name of a service associated with a well-known
   //  port.  PROTO is updated to the service's protocol.  Failure can be
   //  detected using SysIpL2Addr::IsValid.
   //
   //  NOTE: Obtaining the result may involve a remote query, so the invoking
   //  ====  thread is temporarily made preemptable.
   //
   SysIpL3Addr
      (const std::string& name, const std::string& service, IpProtocol& proto);

   //  Virtual to allow subclassing.
   //
   virtual ~SysIpL3Addr();

   //  Copy constructor.
   //
   SysIpL3Addr(const SysIpL3Addr& that) = default;

   //  Copy operator.
   //
   SysIpL3Addr& operator=(const SysIpL3Addr& that) = default;

   //  Returns true if the IP addresses and ports match.
   //
   bool operator==(const SysIpL3Addr& that) const;

   //  Returns the inverse of the == operator.
   //
   bool operator!=(const SysIpL3Addr& that) const;

   //  Sets NETADDR and NETPORT from our IPv4 address by converting it from host
   //  to network order.
   //
   void HostToNetwork(IPv4Addr& netaddr, ipport_t& netport) const;

   //  Sets NETADDR and NETPORT from our IPv6 address by converting it from host
   //  to network order.
   //
   void HostToNetwork(uint16_t netaddr[8], ipport_t& netport) const;

   //  Sets an IPv4 address from NETADDR and NETPORT, which must be in network
   //  order.
   //
   void NetworkToHost(IPv4Addr netaddr, ipport_t netport);

   //  Sets an IPv6 address from NETADDR and NETPORT, whose quartets must be in
   //  network order.
   //
   void NetworkToHost(const uint16_t netaddr[8], ipport_t netport);

   //  Returns true if THAT's IP address matches ours.
   //
   bool L2AddrMatches(const SysIpL2Addr& that) const;

   //  Returns the port.
   //
   ipport_t GetPort() const { return port_; }

   //  Returns the protocol.
   //
   IpProtocol GetProtocol() const { return proto_; }

   //  Sets the socket for the address.
   //
   void SetSocket(SysTcpSocket* socket);

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
   //  NOTE: Obtaining the result may involve a remote query, so the
   //  ====  invoking thread is temporarily made preemptable.
   //
   bool AddrToName(std::string& name, std::string& service) const;

   //  The same as to_str(), but also displays proto_ and socket_ unless
   //  both still have their default values (IpAny and nullptr).
   //
   std::string to_string() const;

   //  Sets the address to the null address after releasing socket_.
   //
   void Nullify();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Returns the address as a string ("n.n.n.n:p").
   //
   std::string to_str() const override;
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
