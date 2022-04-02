//==============================================================================
//
//  TcpIoThread.h
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
#ifndef TCPIOTHREAD_H_INCLUDED
#define TCPIOTHREAD_H_INCLUDED

#include "IoThread.h"
#include <cstddef>
#include "Allocators.h"
#include "Array.h"
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

   //  Creates a TCP I/O thread, managed by DAEMON, that receives messages
   //  on PORT on behalf of SERVICE.
   //
   TcpIoThread
      (NodeBase::Daemon* daemon, const TcpIpService* service, ipport_t port);

   //  Adds SOCKET to the list of sockets when accepting a new connection.
   //
   bool InsertSocket(SysSocket* socket);

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
private:
   //  Returns the listener socket.
   //
   SysTcpSocket* Listener() const;

   //  Ensures that the listener socket (if required) exists upon entering
   //  the thread or after the listener socket encounters an error.  Returns
   //  false on failure.
   //
   bool EnsureListener();

   //  Allocates or replaces the listener socket.  Returns true on success.
   //
   bool AllocateListener();

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

   //  Releases resources when exiting or cleaning up the thread.
   //
   void ReleaseResources();

   //  Overridden to return a name for the thread.
   //
   NodeBase::c_string AbbrName() const override;

   //  Overridden to claim IpBuffers queued for output.
   //
   void ClaimBlocks() override;

   //  Overridden to receive TCP messages on PORT.
   //
   void Enter() override;

   //  The sockets associated with the port served by this thread.  The
   //  first socket listens for new connections, and each of the others
   //  handles an individual connection.
   //
   NodeBase::Array< SysTcpSocket*,
      NodeBase::DynamicAllocator< SysTcpSocket* >> sockets_;

   //  Set if the underlying service accepts connections.  If not set,
   //  a listener socket is not allocated, and sockets_[0] is not used.
   //
   bool listen_;

   //  The number of sockets with events that still need to be serviced.
   //
   NodeBase::word ready_;

   //  The socket currently being serviced (used to index sockets_).
   //
   size_t curr_;
};
}
#endif
