//==============================================================================
//
//  IpPortRegistry.h
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
#ifndef IPPORTREGISTRY_H_INCLUDED
#define IPPORTREGISTRY_H_INCLUDED

#include "Persistent.h"
#include <string>
#include "NbTypes.h"
#include "NwTypes.h"
#include "Q1Way.h"
#include "SysIpL2Addr.h"

namespace NetworkBase
{
   class HostAddrCfg;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Global registry for IP ports that receive messages for applications.
//
class IpPortRegistry : public NodeBase::Persistent
{
   friend class IpPort;
   friend class NodeBase::Singleton< IpPortRegistry >;
public:
   //  Returns the element's IP address (hostname).
   //
   static SysIpL2Addr HostAddress();

   //  Returns the IpPort registered against PORT and PROTOCOL.  If PROTOCOL
   //  is IpAny, the first IpPort registered against PORT is returned.
   //
   IpPort* GetPort(ipport_t port, IpProtocol protocol = IpAny) const;

   //  Returns the registry of ports.  Used for iteration.
   //
   const NodeBase::Q1Way< IpPort >& Ports() const { return portq_; }

   //  Returns true if DEST.ADDR is either SRCE.ADDR, the loopback address,
   //  or the host IP address, *and* the destination port is either NilIpPort
   //  or has an IpPort registered against it.
   //
   bool CanBypassStack(const SysIpL3Addr& srce, const SysIpL3Addr& dest) const;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   IpPortRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~IpPortRegistry();

   //  Adds PORT to the registry.
   //
   bool BindPort(IpPort& port);

   //  Removes PORT from the registry.
   //
   void UnbindPort(IpPort& port);

   //  The element's IP address.
   //
   static SysIpL2Addr HostAddr_;

   //  The element's IP address.
   //
   static std::string HostAddrStr_;

   //  Configuration parameter for the element's IP address.
   //
   std::unique_ptr< HostAddrCfg > hostAddrCfg_;

   //  Information about each IP port that receives messages.
   //
   NodeBase::Q1Way< IpPort > portq_;

   //  The statistics group for IP ports.
   //
   NodeBase::StatisticsGroupPtr statsGroup_;
};
}
#endif
