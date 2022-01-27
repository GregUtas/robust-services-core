//==============================================================================
//
//  SysIpL2Addr.h
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
#ifndef SYSIPL2ADDR_H_INCLUDED
#define SYSIPL2ADDR_H_INCLUDED

#include "Object.h"
#include <cstdint>
#include <string>
#include <vector>
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Operating system abstraction layer: layer 2 IP address.
//
class SysIpL2Addr : public NodeBase::Object
{
public:
   //  Constructs the null address.
   //
   SysIpL2Addr();

   //  Constructs an address from TEXT.  An IPv4 address must use decimal
   //  digits and be of the form n.n.n.n (n = 0 to 255).  An IPv6 address
   //  must use hex digits and be of the form h:h:h:h:h:h:h:h (h = 0 to
   //  0xffff).  Failure can be checked by invoking IsValid.
   //
   SysIpL2Addr(const std::string& text);

   //  Virtual to allow subclassing.
   //
   virtual ~SysIpL2Addr();

   //  Copy constructor.
   //
   SysIpL2Addr(const SysIpL2Addr& that) = default;

   //  Copy operator.
   //
   SysIpL2Addr& operator=(const SysIpL2Addr& that) = default;

   //  Returns true if this IP address is identical to THAT.
   //
   bool operator==(const SysIpL2Addr& that) const;

   //  Returns true if this IP address is different from THAT.
   //
   bool operator!=(const SysIpL2Addr& that) const;

   //  Returns true if this platform supports IPv6 and dual-mode sockets.
   //  If true can be returned, the only reason to return false is to test
   //  IPv4-only operation.
   //
   static bool SupportsIPv6();

   //  Returns true if the address is not the null address.
   //
   bool IsValid() const;

   //  Returns the address as a string.
   //
   virtual std::string to_str() const;

   //  Returns all local addresses except IPv6 addresses with a non-zero
   //  scope identifier.  Set the comment in the declaration of IPv6Addr.
   //
   static std::vector< SysIpL2Addr > LocalAddrs();

   //  Constructs this element's loopback address.
   //
   static const SysIpL2Addr& LoopbackIpAddr();

   //  Returns true if this is a loopback address.
   //
   bool IsLoopbackIpAddr() const;

   //  Updates NAME to the standard name of this host.
   //
   static bool LocalName(std::string& name);

   //  Returns the type of address.
   //
   IpAddrFamily Family() const;

   //  Sets the address to the null address.
   //
   void Nullify();

   //  Returns the null address (all zeroes).
   //
   static const SysIpL2Addr& NullIpAddr();

   //  Returns the raw IPv6 address.
   //
   const IPv6Addr& Addr() { return addr_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Constructs an IPv4 address from NETADDR, which must be in network
   //  order.
   //
   explicit SysIpL2Addr(IPv4Addr netaddr);

   //  Constructs an IPv6 address from NETADDR, whose quartets must be in
   //  network order.
   //
   explicit SysIpL2Addr(const uint16_t netaddr[8]);

   //  Sets NETADDR from our IPv4 address by converting it from host to
   //  network order.
   //
   void HostToNetwork(IPv4Addr& netaddr) const;

   //  Sets NETADDR from our IPv6 address by converting it from host to
   //  network order.
   //
   void HostToNetwork(uint16_t netaddr[8]) const;

   //  Sets an IPv4 address from NETADDR, which must be in network order.
   //
   void NetworkToHost(IPv4Addr netaddr);

   //  Sets an IPv6 address from NETADDR, whose quartets must be in network
   //  order.
   //
   void NetworkToHost(const uint16_t netaddr[8]);
private:
   //  The address.  An IPv4 address is stored as an IPv4-mapped IPv6 address.
   //
   IPv6Addr addr_;
};
}
#endif
