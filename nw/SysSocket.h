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
#include "NwTypes.h"
#include "SysDecls.h"
#include "SysTypes.h"
#include "ToolTypes.h"
#include "TraceRecord.h"

namespace NodeBase
{
   class NwTrace;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: sockets.
//
//  NOTE: IPv6 and out-of-band data are not currently supported.
//
class SysSocket : public Dynamic
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

   //  Invoked by an application when it begins to use the socket.
   //
   virtual void Acquire() { }

   //  Invoked by an application when it no longer requires the socket.
   //
   virtual void Release() { }

   //  Nullifies the socket if it is no longer valid.
   //
   void Invalidate();

   //  Returns true if the socket is valid.
   //
   bool IsValid() const;

   //  Returns true if the socket is valid and has not initiated a
   //  disconnect sequence.
   //
   bool IsOpen() const;

   //  Returns true if no bytes are waiting to be read from the socket.
   //
   bool Empty();

   //  Invoked before performing socket operations.  If BLOCKING is set,
   //  an operation on the socket is allowed to block.
   //
   bool SetBlocking(bool blocking);

   //  Sets the size of the socket's receive and transmit buffers.
   //
   AllocRc SetBuffSizes(size_t rxSize, size_t txSize);

   //  Sends BUFF from the socket.
   //
   virtual SendRc SendBuff(IpBuffer& buff) = 0;

   //  Generates a log when a socket operation fails.  EXPL explains the
   //  failure, and BUFF is any associated buffer.
   //
   void OutputLog(fixed_string expl, const IpBuffer* buff) const;

   //  Returns the last error report on the socket.  Its interpretation
   //  is platform specific.
   //
   word GetError() const { return error_; }

   //  Initializes the socket layer of the host O/S during startup.
   //
   static bool StartLayer();

   //  Releases the socket layer of the host O/S during shutdown.
   //
   static void StopLayer();

   //  Returns a trace record for RID if tracing is enabled on this
   //  socket.  DATA is event specific.
   //
   NwTrace* TraceEvent(TraceRecord::Id rid, word data);

   //  Returns a trace record for RID if this socket should trace PORT
   //  on this node.  DATA contains event-specific information.
   //
   NwTrace* TracePort(TraceRecord::Id rid, ipport_t port, word data);

   //  Returns a trace record for RID if this socket should trace PORT or PEER.
   //  DATA is event specific.
   //
   NwTrace* TracePeer
      (TraceRecord::Id rid, ipport_t port, const SysIpL3Addr& peer, word data);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Allocates a socket that will send and receive on PORT, using PROTO.
   //  If PORT is NilIpPort, the socket is created but is not bound to a
   //  port.  rxSize and txSize specify the size of the receive and send
   //  buffers.  RC is updated to indicate success or failure.  Protected
   //  because this class is virtual.
   //
   SysSocket(ipport_t port, IpProtocol proto,
      size_t rxSize, size_t txSize, AllocRc& rc);

   //  Invoked by SysTcpSocket::Accept to wrap a socket that was created
   //  for a new connection.
   //
   explicit SysSocket(SysSocket_t socket);

   //  Closes the socket.  Protected so that subclasses can control their
   //  deletion policy.  Virtual to allow subclassing.
   //
   virtual ~SysSocket();

   //  Returns the native socket.
   //
   SysSocket_t Socket() const { return socket_; }

   //  Initiates a disconnect and disables further sends on the socket.
   //  Protected so that subclasses can decide how to expose this function.
   //
   void Disconnect();

   //  Closes the socket.  Protected so that subclasses can decide how to
   //  expose this function.
   //
   void Close();

   //  Sets the error code for the socket so that it can be obtained for
   //  logging purposes.  If the value is not provided explicitly, it is
   //  obtained from the underlying platform.  Returns -1.
   //
   word SetError();
   word SetError(word errval);
private:
   //  Overridden to prohibit copying.
   //
   SysSocket(const SysSocket& that);
   void operator=(const SysSocket& that);

   //  Sets or clears tracing_ and returns the new setting.
   //
   bool SetTracing(bool tracing);

   //  Returns true if tracing is currently enabled.
   //
   bool TraceEnabled();

   //  Returns true if tracing is enabled and STATUS indicates that this
   //  socket should be traced.
   //
   bool Trace(TraceStatus status);

   //  The actual native socket.
   //
   SysSocket_t socket_;

   //  Set if operations on the socket can block.  Used by SetBlocking to
   //  avoid unnecessary work.
   //
   bool blocking_;

   //  Set if the socket has initiated a disconnect.
   //
   bool disconnecting_;

   //  Set if this socket is being traced.
   //
   bool tracing_;

   //  The last error reported on this socket by the underlying platform.
   //
   word error_;
};
}
#endif
