//==============================================================================
//
//  SysTcpSocket.h
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
#ifndef SYSTCPSOCKET_H_INCLUDED
#define SYSTCPSOCKET_H_INCLUDED

#include "SysSocket.h"
#include <bitset>
#include <cstddef>
#include "Duration.h"
#include "NwTypes.h"
#include "Q1Way.h"
#include "SysDecls.h"
#include "SysTypes.h"

namespace NetworkBase
{
   class TcpIpService;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Flags that request and report a socket's status during Poll().
//
enum PollFlag
{
   PollInvalid,   // socket is no longer valid
   PollError,     // we already disconnected or peer aborted
   PollHungUp,    // peer disconnected or aborted
   PollWrite,     // can send without blocking
   PollWriteOob,  // can send out-of-band data without blocking
   PollRead,      // can read without blocking
   PollReadOob,   // can read out-of-band data without blocking
   PollFlag_N     // number of flags
};

typedef std::bitset<PollFlag_N> PollFlags;

//------------------------------------------------------------------------------
//
//  Operating system abstraction layer: TCP socket.  The implementation ensures
//  that a pointer to a socket remains valid until both the application and I/O
//  thread have released it.
//
class SysTcpSocket : public SysSocket
{
   friend SysTcpSocketPtr::deleter_type;
public:
   //  Allocates a socket that will send and receive on PORT, on behalf of
   //  SERVICE.  The socket is made non-blocking.  RC is updated to indicate
   //  success or failure, and a log is generated on failure.
   //
   SysTcpSocket(ipport_t port, const TcpIpService* service, AllocRc& rc);

   //  Invoked by an I/O thread when it adds the socket to its poll array.
   //
   void Register();

   //  Invoked by an I/O thread when it removes the socket from its poll
   //  array.  Returns true if the application was not using the socket,
   //  in which case it has been deleted.
   //
   bool Deregister();

   //  Initiates connection setup to remAddr.  Returns 0 on success.  If
   //  the socket is non-blocking, reports success immediately; the socket
   //  then queues outgoing messages until the connection is accepted.
   //  Returns -1 on failure; the invoker should then generate a log.
   //
   NodeBase::word Connect(const SysIpL3Addr& remAddr);

   //  Listens for Connect requests.  BACKLOG is the maximum number of
   //  requests that can be queued, waiting to be processed by Accept.
   //  Returns 0 on success.  Returns -1 on failure; the invoker should
   //  then generate a log.
   //
   NodeBase::word Listen(size_t backlog);

   //  Accesses the flags that request the socket's status when invoking
   //  Poll.  Only the read and write flags should be set.
   //
   PollFlags* InFlags() { return &inFlags_; }

   //  Waits for events on SOCKETS, which is SIZE in length.  TIMEOUT
   //  specifies how long to wait.  Returns the number of sockets on
   //  which events have occurred.  On failure, returns -1 after
   //  generating a log.
   //
   static NodeBase::word Poll(SysTcpSocket* sockets[],
      size_t size, const NodeBase::msecs_t& timeout);

   //  Returns the flags that reported the socket's status after invoking
   //  Poll.  Any of the flags could have been set.
   //
   PollFlags* OutFlags() { return &outFlags_; }

   //  Invoked on a socket that had called Listen to create a socket for
   //  accepting a new connection.  Sets remAddr to the peer address that
   //  is communicating with the new socket.  Clears PollRead and returns
   //  nullptr if no connection requests were pending.  On failure, leaves
   //  PollRead set and returns nullptr; the invoker should then generate
   //  a log.
   //
   SysTcpSocketPtr Accept(SysIpL3Addr& remAddr);

   //  Reads up to SIZE bytes into BUFF.  Returns the number of bytes read.
   //  Returns 0 if the socket was gracefully closed.  Returns -1 when the
   //  invoker should generate a log to report a failure, and -2 if a log
   //  has already been generated.
   //
   NodeBase::word Recv(NodeBase::byte_t* buff, size_t size);

   //  Sends SIZE bytes, starting at DATA, to the address to which the socket
   //  is bound.  Returns the number of bytes sent.  Returns 0 if the socket
   //  would block.  Returns -1 when the invoker should generate a log to
   //  report a failure, and -2 if a log has already been generated.
   //
   NodeBase::word Send(const NodeBase::byte_t* data, size_t size);

   //  Sets locAddr to the address of this socket.  On failure, generates
   //  a log and returns false.
   //
   bool LocAddr(SysIpL3Addr& locAddr);

   //  Sets remAddr to the peer address that is communicating with this
   //  socket.  On failure, generates a log and returns false.
   //
   bool RemAddr(SysIpL3Addr& remAddr);

   //  Invoked by an I/O thread when the socket becomes writeable, which
   //  prompts it to send any queued messages.
   //
   void Dispatch();

   //  Sets the linger option to determine how the socket will be closed.
   //  If GRACEFUL is not set, a hard close (RST) will occur.  Lingering
   //  for a timeout is not supported because it requires the socket to
   //  block.  On failure, returns false after generating a log.
   //
   bool SetClose(bool graceful);

   //  Invoked by an I/O thread to delete the socket immediately.
   //
   void Purge();

   //  The socket's state with respect to the application.
   //
   enum AppState : unsigned int
   {
      Initial,   // socket allocated
      Acquired,  // application has invoked Acquire()
      Released   // application has invoked Release()
   };

   //  Returns the socket's application state.
   //
   AppState GetAppState() const { return appState_; }

   //  Invoked by an application when it begins to use the socket.
   //
   virtual void Acquire();

   //  Invoked by an application when it no longer requires the socket.
   //
   virtual void Release();

   //  Returns true if the socket is valid and has not initiated a disconnect.
   //
   bool IsOpen() const;

   //  Sets the incoming message buffer.  If a different buffer is already
   //  present, it is deleted.  If BUFF is nullptr, the buffer is deleted.
   //
   void SetIcMsg(IpBuffer* buff);

   //  Returns the incoming message buffer.
   //
   IpBuffer* IcMsg() const { return icMsg_; }

   //  Returns the incoming message buffer and sets it to nullptr.
   //
   IpBuffer* AcquireIcMsg();

   //  Overridden to claim IpBuffers queued for output.
   //
   void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to indicate that this socket is running TCP.
   //
   IpProtocol Protocol() const override { return IpTcp; }

   //  Overridden to send BUFF.
   //
   SendRc SendBuff(IpBuffer& buff) override;

   //  Overridden to configure the socket for a keepalive if required.
   //
   AllocRc SetService(const IpService* service, bool shared) override;
private:
   //  Initiates a disconnect and disables further sends on the socket.
   //
   void Disconnect();

   //  States for TCP sockets.
   //
   enum State : unsigned int
   {
      Idle,        // initial state
      Listening,   // has invoked Listen
      Connecting,  // has invoked Connect
      Connected    // created by Accept, or Connect has succeeded
   };

   //  Invoked to wrap SOCKET, which was created to accept a connection
   //  from a peer that is using PORT.  The socket is made non-blocking.
   //
   SysTcpSocket(NodeBase::SysSocket_t socket, ipport_t port);

   //  Closes the socket.  Private because sockets are closed and deleted
   //  using Release, Deregister, or Purge.
   //
   ~SysTcpSocket();

   //  Queues BUFF when it cannot be sent until the socket is writeable.
   //  If HENQ is set, the buffer is placed at the front of the queue.
   //
   SendRc QueueBuff(IpBuffer* buff, bool henq = false);

   //  The socket's state.
   //
   State state_ : 8;

   //  Set if the socket has initiated a disconnect.
   //
   bool disconnecting_;

   //  Set if the socket is registered with an I/O thread.
   //
   bool iotActive_ : 8;

   //  Set if an application is using the socket.
   //
   AppState appState_ : 8;

   //  Flags that query the socket's status before invoking Poll.
   //
   PollFlags inFlags_;

   //  Flags that report the socket's status after invoking Poll.
   //
   PollFlags outFlags_;

   //  An incoming message that is being assembled because it was segmented.
   //
   IpBuffer* icMsg_;

   //  Pending outgoing messages.  Outgoing messages are queued while the
   //  socket is waiting for a reply to a Connect or is otherwise blocked.
   //  The messages are sent when the socket becomes writeable.
   //
   NodeBase::Q1Way<IpBuffer> ogMsgq_;
};
}
#endif
