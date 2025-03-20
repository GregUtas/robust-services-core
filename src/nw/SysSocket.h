//==============================================================================
//
//  SysSocket.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef SYSSOCKET_H_INCLUDED
#define SYSSOCKET_H_INCLUDED

#include "Dynamic.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include "NbTypes.h"
#include "NwTypes.h"
#include "SysDecls.h"
#include "SysTypes.h"
#include "ToolTypes.h"

namespace NetworkBase
{
   class NwTrace;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Operating system abstraction layer: sockets.
//
//  NOTE: IPv6 and out-of-band data are not currently supported.
//
//  The standard functions for converting to/from network order.
//
uint64_t htonll(uint64_t hostllong);
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint64_t ntohll(uint64_t netllong);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

//------------------------------------------------------------------------------

class SysSocket : public NodeBase::Dynamic
{
public:
   //  Deleted to prohibit copying.
   //
   SysSocket(const SysSocket& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   SysSocket& operator=(const SysSocket& that) = delete;

   //> Arbitrary limit on the size of IP messages (in bytes).  Note
   //  that a protocol running over UDP is typically restricted to
   //  a smaller size.
   //
   static const size_t MaxMsgSize = 2048;

   //  The outcome when allocating a socket.
   //
   enum AllocRc
   {
      AllocOk,         // success
      AllocFailed,     // failed to allocate socket
      GetOptionError,  // failed to read socket attribute
      SetOptionError,  // failed to set socket attribute
      BindError        // failed to bind socket to port
   };

   //  The outcome when sending a message buffer.
   //
   enum SendRc
   {
      SendOk,       // success
      SendQueued,   // buffer queued until connection is accepted
      SendBlocked,  // socket cannot send buffer without blocking
      SendFailed    // failed to send buffer
   };

   //  Returns the protocol that the socket is running.
   //
   virtual IpProtocol Protocol() const { return IpAny; }

   //  Nullifies the socket if it is no longer valid.
   //
   void Invalidate();

   //  Returns true if the socket is valid.
   //
   bool IsValid() const;

   //  Returns true if no bytes are waiting to be read from the socket.
   //
   bool Empty();

   //  Invoked before performing socket operations.  If BLOCKING is set,
   //  an operation on the socket is allowed to block.  Returns false and
   //  generates a log if the socket could not be configured as desired.
   //
   bool SetBlocking(bool blocking);

   //  Configures the socket for use by SERVICE.  SHARED is set if the
   //  socket is shared by all instances of SERVICE rather than being
   //  dedicated to a single instance.  The default version sets the
   //  size of the socket's receive and transmit buffers based on
   //  o RxSize and TxSize if SHARED is set, and
   //  o GetAppSocketSizes if SHARED is not set.
   //  Returns AllocOk on success.  On failure, generates a log and
   //  returns the reason for the failure.
   //
   virtual AllocRc SetService(const IpService* service, bool shared);

   //  Sends BUFF from the socket.  Returns SendOk or SendQueued on
   //  success.  On failure, generates a log and returns the reason for
   //  the failure.
   //
   virtual SendRc SendBuff(IpBuffer& buff) = 0;

   //  Generates the network log specifies by ID when the platform-specific
   //  ERRVAL was returned by FUNC.  Returns -1.
   //
   nwerr_t OutputLog(NodeBase::LogId id,
      NodeBase::c_string func, nwerr_t errval);

   //  Generates the network log specified by ID when BUFF could not be
   //  sent.  FUNC identifies the function that failed.
   //
   void OutputLog(NodeBase::LogId id,
      NodeBase::c_string func, const IpBuffer* buff) const;

   //  Returns the last error report on the socket.  Its interpretation
   //  is platform specific.
   //
   nwerr_t GetError() const { return error_; }

   //  Returns the alarm name associated with the platform-specific ERRVAL.
   //  Returns an empty string if no alarm is associated with the error.
   //
   static NodeBase::c_string AlarmName(nwerr_t errval);

   //  Initializes this executable's use of our element's socket layer
   //  during startup.
   //
   static bool StartLayer();

   //  Releases this executable's use of our element's socket layer
   //  during shutdown.
   //
   static void StopLayer();

   //  Returns a trace record for RID if tracing is enabled on this
   //  socket.  DATA is event specific.
   //
   NwTrace* TraceEvent(NodeBase::TraceRecordId rid, NodeBase::word data);

   //  Returns a trace record for RID if this socket should trace PORT
   //  on this node.  DATA contains event-specific information.
   //
   NwTrace* TracePort
      (NodeBase::TraceRecordId rid, ipport_t port, NodeBase::word data);

   //  Returns a trace record for RID if this socket should trace PORT or PEER.
   //  DATA is event specific.
   //
   NwTrace* TracePeer(NodeBase::TraceRecordId rid, ipport_t port,
      const SysIpL3Addr& peer, NodeBase::word data);

   //  Displays socket information for logging purposes.
   //
   std::string to_str() const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Allocates a socket that will send and receive on PORT, on behalf of
   //  SERVICE.  If PORT is NilIpPort, the socket is created but is not bound
   //  to a port.  RC is updated to indicate success or failure, and a log is
   //  generated on failure.  Protected because this class is virtual.
   //
   SysSocket(ipport_t port, const IpService* service, AllocRc& rc);

   //  Invoked by SysTcpSocket::Accept to wrap a socket that was created
   //  for a new connection.
   //
   SysSocket(NodeBase::SysSocket_t socket, ipport_t port);

   //  Records the deletion.  Subclasses must invoke Close().  Protected so
   //  that subclasses can control their deletion policy.  Virtual to allow
   //  subclassing.
   //
   virtual ~SysSocket();

   //  Returns the native socket.
   //
   NodeBase::SysSocket_t Socket() const { return socket_; }

   //  Closes the socket.  Protected so that subclasses can decide how
   //  to expose this function.  DISCONNECTING is set if the socket has
   //  initiated a disconnect.
   //
   void Close(bool disconnecting);

   //  Sets the platform-specific ERRVAL for the socket so that it can
   //  later be retrieved and included in a log.  Returns -1.
   //
   nwerr_t SetError(nwerr_t errval);
private:
   //  Sets or clears tracing_ and returns the new setting.
   //
   bool SetTracing(bool tracing);

   //  Returns true if tracing is currently enabled.
   //
   bool TraceEnabled();

   //  Returns true if tracing is enabled and STATUS indicates that this
   //  socket should be traced.
   //
   bool Trace(NodeBase::TraceStatus status);

   //  The actual native socket.
   //
   NodeBase::SysSocket_t socket_;

   //  The last error reported on this socket by the underlying platform.
   //
   nwerr_t error_;

   //  The port to which the socket is bound.
   //
   ipport_t port_;

   //  Set if operations on the socket can block.  Used by SetBlocking to
   //  avoid unnecessary work.
   //
   bool blocking_;

   //  Set if this socket is being traced.
   //
   bool tracing_;
};
}
#endif
