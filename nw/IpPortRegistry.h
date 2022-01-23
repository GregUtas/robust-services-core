//==============================================================================
//
//  IpPortRegistry.h
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
#ifndef IPPORTREGISTRY_H_INCLUDED
#define IPPORTREGISTRY_H_INCLUDED

#include "Protected.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "Q1Way.h"
#include "SysIpL2Addr.h"

namespace NetworkBase
{
   class LocalAddrCfg;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Global registry for IP ports that receive messages for services.
//
class IpPortRegistry : public NodeBase::Protected
{
   friend class NodeBase::Singleton< IpPortRegistry >;
   friend class IpPort;
public:
   //  Deleted to prohibit copying.
   //
   IpPortRegistry(const IpPortRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   IpPortRegistry& operator=(const IpPortRegistry& that) = delete;

   //  Returns this element's IP address.
   //
   static const SysIpL2Addr& LocalAddr();

   //  Returns true if IPv6 should be used.
   //
   static bool UseIPv6();

   //  Returns the IpPort registered against PORT and PROTOCOL.  If PROTOCOL
   //  is IpAny, the first IpPort registered against PORT is returned.
   //
   IpPort* GetPort(ipport_t port, IpProtocol protocol = IpAny) const;

   //  Returns the registry of ports.  Used for iteration.
   //
   const NodeBase::Q1Way< IpPort >& Ports() const { return portq_; }

   //  Returns true if DEST's IP address is the same as SRCE's, a loopback
   //  address, or this element's IP address, *and* the destination port is
   //  either NilIpPort or has an IpPort registered against it.
   //
   bool CanBypassStack(const SysIpL3Addr& srce, const SysIpL3Addr& dest) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   IpPortRegistry();

   //  Private because this is a singleton.
   //
   ~IpPortRegistry();

   //  Adds PORT to the registry.
   //
   bool BindPort(IpPort& port);

   //  Removes PORT from the registry.
   //
   void UnbindPort(IpPort& port);

   //  Determines whether IPv6 should be used.
   //
   void SetIPv6();

   //  Determines this element's address.
   //
   void SetLocalAddr();

   //  Set if IPv6 should be used.
   //
   bool ipv6Enabled_;

   //  The element's IP address.
   //
   SysIpL2Addr localAddr_;

   //  Configuration parameter for the element's IP address.
   //
   std::unique_ptr< LocalAddrCfg > localAddrCfg_;

   //  Information about each IP port that receives messages.
   //
   NodeBase::Q1Way< IpPort > portq_;

   //  The statistics group for IP ports.
   //
   NodeBase::StatisticsGroupPtr statsGroup_;
};
}
#endif
