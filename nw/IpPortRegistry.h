//==============================================================================
//
//  IpPortRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef IPPORTREGISTRY_H_INCLUDED
#define IPPORTREGISTRY_H_INCLUDED

#include "Protected.h"
#include <string>
#include "NbTypes.h"
#include "NwTypes.h"
#include "Q1Way.h"
#include "SysIpL2Addr.h"

namespace NodeBase
{
   class HostAddrCfg;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for IP ports that receive messages for applications.
//
class IpPortRegistry : public Protected
{
   friend class IpPort;
   friend class Singleton< IpPortRegistry >;
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
   const Q1Way< IpPort >& Ports() const { return portq_; }

   //  Returns true if DEST.ADDR is either SRCE.ADDR, the loopback address,
   //  or the host IP address, *and* the destination port is either NilIpPort
   //  or has an IpPort registered against it.
   //
   bool CanBypassStack(const SysIpL3Addr& srce, const SysIpL3Addr& dest) const;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
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
   Q1Way< IpPort > portq_;

   //  The statistics group for IP ports.
   //
   StatisticsGroupPtr statsGroup_;
};
}
#endif
