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
#include <string>
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Operating system abstraction layer: layer 2 IP address.
//
class SysIpL2Addr : public NodeBase::Object
{
public:
   //  Constructs a nil address (255.255.255.255).
   //
   SysIpL2Addr();

   //  Constructs an IPv4 address.
   //
   explicit SysIpL2Addr(ipv4addr_t v4Addr);

   //  Constructs an IPv4 address from the string "n.n.n.n".
   //  Failure can be checked using IsValid.
   //
   explicit SysIpL2Addr(const std::string& text);

   //  Virtual to allow subclassing.
   //
   virtual ~SysIpL2Addr();

   //  Copy constructor.
   //
   SysIpL2Addr(const SysIpL2Addr& that) = default;

   //  Copy operator.
   //
   SysIpL2Addr& operator=(const SysIpL2Addr& that) = default;

   //  Constructs the loopback address (127.0.0.1) in host order.
   //
   static SysIpL2Addr LoopbackAddr();

   //  Returns true if the address is non-nil.
   //
   bool IsValid() const;

   //  Returns the full IPv4 address.
   //
   ipv4addr_t GetIpV4Addr() const { return v4Addr_; }

   //  Returns the address as a string ("n.n.n.n").
   //
   virtual std::string to_str() const;

   //  Updates NAME to the standard name of this host.
   //
   static bool HostName(std::string& name);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the full IPv4 address;
   //
   void SetIpV4Addr(ipv4addr_t v4Addr) { v4Addr_ = v4Addr; }
private:
   //  IPv4 address.
   //
   ipv4addr_t v4Addr_;
};
}
#endif
