//==============================================================================
//
//  TcpIoThread.h
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
#ifndef TCPIOTHREAD_H_INCLUDED
#define TCPIOTHREAD_H_INCLUDED

#include "IoThread.h"
#include <cstddef>
#include "Array.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "SysTypes.h"

namespace NetworkBase
{
   class TcpIpService;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  I/O thread for TCP-based protocols.
//
class TcpIoThread : public IoThread
{
public:
   //> The maximum number of connections allowed on a socket.
   //
   static const size_t MaxConns;

   //  Creates an I/O thread that will receive TCP messages on
   //  behalf of SERVICE.
   //
   TcpIoThread(const TcpIpService* service, ipport_t port);

   //  Adds SOCKET to the list of sockets when accepting a new connection.
   //
   bool InsertSocket(SysSocket* socket);

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
protected:
   //  Protected to restrict deletion.
   //
   virtual ~TcpIoThread();

   //  Overridden to release resources in order to unblock.
   //
   void Unblock() override;

   //  Overridden to release resources during error recovery.
   //
   void Cleanup() override;
private:
   //  Overridden to return a name for the thread.
   //
   const char* AbbrName() const override;

   //  Overridden to receive TCP messages on PORT.
   //
   void Enter() override;

   //  Returns the listener socket.
   //
   SysTcpSocket* Listener() const;

   //  Ensures that the listener socket (if required) exists upon entering
   //  the thread or after the listener socket encounters an error.  Returns
   //  false on failure.
   //
   bool EnsureListener();

   //  Allocates or replaces the listener socket.
   //
   SysTcpSocket* AllocateListener();

   //  Generates a log, deregisters LISTENER from our port, and returns
   //  false if an error has occurred on LISTENER.
   //
   bool ListenerHasFailed(SysTcpSocket* listener) const;

   //  Raises an alarm when the thread will exit because a listener socket
   //  could not be configured.  ERRVAL is the reason for the failure.
   //  Returns nullptr.
   //
   SysTcpSocket* RaiseAlarm(NodeBase::word errval) const;

   //  Clears any alarm associated with the thread's service.
   //
   void ClearAlarm() const;

   //  Polls the sockets until at least one of them reports an event or
   //  an error occurs.  Returns the result of SysTcpSocket::Poll.
   //
   NodeBase::word PollSockets();

   //  Services the socket at curr_.
   //
   void ServiceSocket();

   //  Invoked to accept a connection.  Clears the PollRead flag if no
   //  connection request was pending.  Returns true if a connection was
   //  accepted.
   //
   bool AcceptConn();

   //  Removes sockets_[index] from the list of sockets.  If it contains a
   //  valid socket, that socket is released.  Because the last socket moves
   //  into the vacated slot, INDEX (used for iteration) is decremented.
   //
   void EraseSocket(size_t& index);

   //  Specifies the error value to be reported by OutputLog (see below).
   //
   enum Error
   {
      SocketNull,   // include ERRVAL in log
      SocketError,  // include socket->GetError() in log
      SocketFlags   // include socket->OutFlags() in log
   };

   //  Generates the log specified by ID when an error occurs.  EXPL explains
   //  the failure, and ERROR is the general type of error.  SOCKET is where
   //  the error occurred, and ERRVAL is used if SOCKET is nullptr.
   //
   void OutputLog(NodeBase::LogId id, NodeBase::fixed_string expl, Error error,
      SysTcpSocket* socket, NodeBase::debug32_t errval = 0) const;

   //  Releases resources when exiting or cleaning up the thread.
   //
   void ReleaseResources();

   //  The sockets associated with the port served by this thread.  The
   //  first socket listens for new connections, and each of the others
   //  handles an individual connection.
   //
   NodeBase::Array< SysTcpSocket* > sockets_;

   //  Set if the underlying service accepts connections.  If not set,
   //  a listener socket is not allocated, and sockets_[0] is not used.
   //
   bool listen_;

   //  The number of sockets with events that still need to be serviced.
   //
   size_t ready_;

   //  The socket currently being serviced (used to index sockets_).
   //
   size_t curr_;
};
}
#endif
