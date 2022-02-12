//==============================================================================
//
//  NwDaemons.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "Duration.h"
#include "NwTypes.h"
#include "SysTypes.h"
#include "TimePoint.h"

namespace NetworkBase
{
   class IoThreadRecreator;
   class TcpIpService;
   class UdpIpService;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Daemons for managing I/O threads.
//
class IoDaemon : public NodeBase::Daemon
{
   friend class IoThreadRecreator;
public:
   //  Virtual to allow subclassing.
   //
   virtual ~IoDaemon() = default;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a daemon identified by NAME that will recreate the I/O thread
   //  for SERVICE and PORT if it exits.
   //
   IoDaemon(NodeBase::c_string name, const IpService* service, ipport_t port);
private:
   //  Overridden by subclasses to create a protocol-specific thread for
   //  SERVICE and PORT.
   //
   virtual NodeBase::Thread* CreateIoThread
      (const IpService* service, ipport_t port) = 0;

   //  Invoked when the deferred work item is deleted.
   //
   void RecreatorDeleted();

   //  Overridden to create the I/O thread.
   //
   NodeBase::Thread* CreateThread() override;

   //  The service for the I/O thread.
   //
   const IpService* const service_;

   //  The port for the I/O thread.
   //
   const ipport_t port_;

   //  The time when the last thread was created.
   //
   NodeBase::TimePoint lastCreation_;

   //  The backoff time for recreating the thread if it exits quickly.
   //
   NodeBase::secs_t backoffSecs_;

   //  The work item queued to recreate the thread after a backoff time.
   //
   IoThreadRecreator* recreator_;
};

//------------------------------------------------------------------------------

extern NodeBase::fixed_string TcpIoDaemonName;

class TcpIoDaemon : public IoDaemon
{
public:
   //  Finds/creates the daemon that manages the TCP I/O thread that receives
   //  messages on PORT on behalf of SERVICE.
   //
   static TcpIoDaemon* GetDaemon(const TcpIpService* service, ipport_t port);

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates a daemon that manages the TCP I/O thread that receives messages
   //  on PORT on behalf of SERVICE.
   //
   TcpIoDaemon(const TcpIpService* service, ipport_t port);

   //  Overridden to create a TCP I/O thread.
   //
   NodeBase::Thread* CreateIoThread
      (const IpService* service, ipport_t port) override;
};

//------------------------------------------------------------------------------

extern NodeBase::fixed_string UdpIoDaemonName;

class UdpIoDaemon : public IoDaemon
{
public:
   //  Finds/creates the daemon that manages the UDP I/O thread that receives
   //  messages on PORT on behalf of SERVICE.
   //
   static UdpIoDaemon* GetDaemon(const UdpIpService* service, ipport_t port);

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates a daemon that manages the UDP I/O thread that receives messages
   //  on PORT on behalf of SERVICE.
   //
   UdpIoDaemon(const UdpIpService* service, ipport_t port);

   //  Overridden to create a UDP I/O thread.
   //
   NodeBase::Thread* CreateIoThread
      (const IpService* service, ipport_t port) override;
};
}
#endif
