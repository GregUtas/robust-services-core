//==============================================================================
//
//  IpPort.h
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
#ifndef IPPORT_H_INCLUDED
#define IPPORT_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <iosfwd>
#include <memory>
#include "InputHandler.h"
#include "NwTypes.h"
#include "Q1Link.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Alarm;
}

namespace NetworkBase
{
   class IpPortStats;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  An IP port that supports a service (an application protocol).
//
class IpPort : public NodeBase::Protected
{
   friend class InputHandler;
public:
   //  Deregisters the port.  Shuts down any I/O thread and deletes any input
   //  handler.  The I/O thread must delete its socket(s) when it shuts down.
   //  Virtual to allow subclassing.
   //
   virtual ~IpPort();

   //  Deleted to prohibit copying.
   //
   IpPort(const IpPort& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   IpPort& operator=(const IpPort& that) = delete;

   //  Returns the IP port.
   //
   ipport_t GetPort() const { return port_; }

   //  Returns the port's service.
   //
   const IpService* GetService() const { return service_; }

   //  Returns the port's input handler.
   //
   InputHandler* GetHandler() const { return handler_.get(); }

   //  Returns the port's I/O thread.
   //
   IoThread* GetThread() const { return thread_; }

   //  Sets (or clears, if nullptr) the port's I/O thread.
   //
   void SetThread(IoThread* thread);

   //  Returns the port's socket.
   //
   SysSocket* GetSocket() const { return socket_; }

   //  Sets (or clears, if nullptr) the port's socket.  If the socket is valid,
   //  the port must already have an input handler and I/O thread.
   //
   bool SetSocket(SysSocket* socket);

   //  Creates a socket for an application instance.  The socket is registered
   //  with the port's I/O thread, which uses Poll() to receive its messages.
   //  The default version returns nullptr and is overridden by a port that
   //  supports a socket for each application instance.
   //
   virtual SysTcpSocket* CreateAppSocket();

   //  Raises an alarm if the port's I/O thread cannot configure its socket.
   //  ERRVAL is a platform-specific error code.  Returns false.
   //
   bool RaiseAlarm(nwerr_t errval) const;

   //  Clears any alarm after the port's I/O thread successfully configures
   //  its socket.
   //
   void ClearAlarm() const;

   //  Invoked after COUNT bytes were received.
   //
   void BytesRcvd(size_t count) const;

   //  Invoked after COUNT bytes were sent.
   //
   void BytesSent(size_t count) const;

   //  Invoked after COUNT receive operations were performed before yielding.
   //
   void RecvsInSequence(size_t count) const;

   //  Invoked when an incoming message is discarded because it is invalid.
   //
   void InvalidDiscarded() const;

   //  Invoked when ingress work is discarded because of overload controls.
   //
   void IngressDiscarded() const;

   //  Invoked when the array of sockets used for polling is full, preventing
   //  another socket from being added.
   //
   void PollArrayOverflow() const;

   //  Returns the number of messages discarded by overload controls during
   //  the current statistics interval.
   //
   size_t Discards() const;

   //  Displays statistics.  May be overridden to include pool-specific
   //  statistics, but the base class version must be invoked.
   //
   virtual void DisplayStats
      (std::ostream& stream, const NodeBase::Flags& options) const;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

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
protected:
   //  Assigns SERVICE to PORT, registering it with IpPortRegistry.  If PORT
   //  is available, this eventually results in the creation of an I/O thread
   //  that runs the protocol (e.g. UDP) specified by SERVICE, in the faction
   //  (e.g. PayloadFaction) also specified by SERVICE.  When the I/O thread
   //  is entered, it allocates a socket, receives messages on the port, and
   //  passes them to an input handler created by SERVICE.  Protected because
   //  this class is virtual.
   //
   IpPort(ipport_t port, const IpService* service);

   //  Creates an I/O thread for the port.  The default version
   //  generates a fatal log and must be overridden by a subclass
   //  that has an input handler.
   //
   virtual IoThread* CreateIoThread();
private:
   //  Sets the port's input handler.  If the port does not have
   //  an I/O thread, it is created.
   //
   bool BindHandler(InputHandler& handler);

   //  Clears the port's input handler.  If the port has an I/O
   //  thread, it is shut down.
   //
   void UnbindHandler(const InputHandler& handler);

   //  Ensures that the low availability alarm exists.
   //
   void EnsureAlarm();

   //  The next entry in IpPortRegistry.
   //
   NodeBase::Q1Link link_;

   //  The port number associated with this entry.
   //
   const ipport_t port_;

   //  The port's service.
   //
   const IpService* const service_;

   //  The port's input handler.
   //
   std::unique_ptr< InputHandler > handler_;

   //  The port's I/O thread.
   //
   IoThread* thread_;

   //  The port's socket.
   //
   SysSocket* socket_;

   //  The port's alarm.
   //
   NodeBase::Alarm* alarm_;

   //  The port's statistics.
   //
   std::unique_ptr< IpPortStats > stats_;
};
}
#endif
