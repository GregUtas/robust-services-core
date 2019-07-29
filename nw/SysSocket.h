//==============================================================================
//
//  SysSocket.h
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
uint64_t ntohll(uint32_t netllong);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

//------------------------------------------------------------------------------

class SysSocket : public NodeBase::Dynamic
{
public:
   //> Arbitrary limit on the size of IP messages (in bytes).
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
   //  an operation on the socket is allowed to block.
   //
   bool SetBlocking(bool blocking);

   //  Configures the socket for use by SERVICE.  SHARED is set if the
   //  socket is shared by all instances of SERVICE rather than being
   //  dedicated to a single instance.  The default version sets the
   //  size of the socket's receive and transmit buffers based on
   //  o RxSize and TxSize if SHARED is set, and
   //  o GetAppSocketSizes if SHARED is not set.
   //
   virtual AllocRc SetService(const IpService* service, bool shared);

   //  Sends BUFF from the socket.
   //
   virtual SendRc SendBuff(IpBuffer& buff) = 0;

   //  Generates the network log specified by ID when a socket operation
   //  fails.  EXPL explains the failure, and BUFF is any associated buffer.
   //
   void OutputLog(NodeBase::LogId id,
      NodeBase::fixed_string expl, const IpBuffer* buff) const;

   //  Returns the last error report on the socket.  Its interpretation
   //  is platform specific.
   //
   NodeBase::word GetError() const { return error_; }

   //  Initializes the socket layer of the host O/S during startup.
   //
   static bool StartLayer();

   //  Releases the socket layer of the host O/S during shutdown.
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
   //  to a port.  RC is updated to indicate success or failure.  Protected
   //  because this class is virtual.
   //
   SysSocket(ipport_t port, const IpService* service, AllocRc& rc);

   //  Invoked by SysTcpSocket::Accept to wrap a socket that was created
   //  for a new connection.
   //
   explicit SysSocket(NodeBase::SysSocket_t socket);

   //  Closes the socket.  Protected so that subclasses can control their
   //  deletion policy.  Virtual to allow subclassing.
   //
   virtual ~SysSocket();

   //  Returns the native socket.
   //
   NodeBase::SysSocket_t Socket() const { return socket_; }

   //  Sets the error code for the socket so that it can be obtained for
   //  logging purposes.  If the value is not provided explicitly, it is
   //  obtained from the underlying platform.  Returns -1.
   //
   NodeBase::word SetError();
   NodeBase::word SetError(NodeBase::word errval);
private:
   //  Deleted to prohibit copying.
   //
   SysSocket(const SysSocket& that) = delete;
   SysSocket& operator=(const SysSocket& that) = delete;

   //  Updates the network alarm when the network goes down or comes back up.
   //  ERR is included in the alarm log when OK is false.
   //
   static void SetStatus(bool ok, const std::string& err);

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

   //  Set if operations on the socket can block.  Used by SetBlocking to
   //  avoid unnecessary work.
   //
   bool blocking_;

   //  Set if this socket is being traced.
   //
   bool tracing_;

   //  The last error reported on this socket by the underlying platform.
   //
   NodeBase::word error_;
};
}
#endif
