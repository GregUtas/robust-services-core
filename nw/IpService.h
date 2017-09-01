//==============================================================================
//
//  IpService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef IPSERVICE_H_INCLUDED
#define IPSERVICE_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include "NbTypes.h"
#include "NwTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliText;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
class IpService : public Protected
{
   friend class Registry< IpService >;
public:
   //> The maximum number of IP services.
   //
   static const id_t MaxId;

   //  Returns a string that identifies the service for display purposes.
   //
   virtual const char* Name() const = 0;

   //  Returns the IP protocol over which the service runs.
   //
   virtual IpProtocol Protocol() const = 0;

   //  Returns the port on which the service should be started during a
   //  restart.
   //
   virtual ipport_t Port() const = 0;

   //  Returns the scheduler faction for the service's I/O thread.
   //
   virtual Faction GetFaction() const = 0;

   //  Returns the size of the receive buffer for the service's I/O thread.
   //
   virtual size_t RxSize() const = 0;

   //  Returns the size of the transmit buffer for the service's I/O thread.
   //
   virtual size_t TxSize() const = 0;

   //  Creates a subclass of CliText for provisioning the service through
   //  the CLI.
   //
   virtual CliText* CreateText() const = 0;

   //  Allocates an application socket when sending an initial message.
   //  Overridden by services that support a dedicated socket for each
   //  application instance.
   //
   virtual SysSocket* CreateAppSocket() const { return nullptr; }

   //  Provides sizes of the receive and transmit buffers for application
   //  sockets.  Invoked to configure the socket when the service's I/O
   //  thread accepts a connection.  The default version generates a log
   //  and must be overridden by services that use a dedicated socket for
   //  each application instance.
   //
   virtual void GetAppSocketSizes(size_t& rxSize, size_t& txSize) const;

   //  Provisions the service on PORT.
   //
   IpPort* Provision(ipport_t port);

   //  Returns the service's identifier.
   //
   id_t Sid() const { return sid_.GetId(); }

   //  Returns the offset to sid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden for restarts.  Invokes CreatePort and CreateHandler to
   //  start the service on the port, if any, returned by GetPort.  May
   //  be overridden if, for example, the service needs to be started on
   //  multiple ports.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Registers the service with IpServiceRegistry.  Protected because
   //  this class is virtual.
   //
   IpService();

   //  Removes the service from IpServiceRegistry.  Protected because
   //  subclasses should be singletons.
   //
   virtual ~IpService();
private:
   //  Overridden to prohibit copying.
   //
   IpService(const IpService& that);
   void operator=(const IpService& that);

   //  Creates an InputHandler that will host the service on PORT.
   //
   virtual InputHandler* CreateHandler(IpPort* port) const = 0;

   //  Creates an IpPort that will host the service on PID.  Overridden
   //  by each IpProtocol-based subclass.
   //
   virtual IpPort* CreatePort(ipport_t pid) = 0;

   //  The service's identifier.
   //
   RegCell sid_;
};
}
#endif
