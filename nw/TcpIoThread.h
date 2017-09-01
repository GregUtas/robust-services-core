//==============================================================================
//
//  TcpIoThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TCPIOTHREAD_H_INCLUDED
#define TCPIOTHREAD_H_INCLUDED

#include "IoThread.h"
#include <cstddef>
#include "Array.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  I/O thread for TCP-based protocols.
//
class TcpIoThread : public IoThread
{
public:
   //> The maximum number of connections allowed on a socket.
   //
   static const size_t MaxConns;

   //  Creates an I/O thread that will receive TCP messages.  fdSize is the
   //  number of simultaneous connections to support.  The other arguments
   //  are described in the base class.
   //
   TcpIoThread(Faction faction, ipport_t port,
      size_t rxSize, size_t txSize, size_t fdSize);

   //  Overridden to claim IpBuffers queued for output.
   //
   virtual void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected to restrict deletion.
   //
   virtual ~TcpIoThread();

   //  Overridden to release resources in order to unblock.
   //
   virtual void Unblock() override;

   //  Overridden to release resources during error recovery.
   //
   virtual void Cleanup() override;
private:
   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to receive TCP messages on PORT.
   //
   virtual void Enter() override;

   //  Returns the listener socket.
   //
   SysTcpSocket* Listener() const;

   //  Ensures that the listener socket exists upon entering the thread
   //  or after the listener socket encounters an error.  Returns nullptr
   //  on failure.
   //
   SysTcpSocket* EnsureListener();

   //  Allocates or replaces the listener socket.
   //
   SysTcpSocket* AllocateListener();

   //  Generates a log, deregisters LISTENER from our port, and returns
   //  false if an error has occurred on LISTENER.
   //
   bool ListenerHasFailed(SysTcpSocket* listener) const;

   //  Invoked when a listener could not be allocated.  ERRVAL is the
   //  reason for the failure.
   //
   SysTcpSocket* ListenerError(word errval) const;

   //  Polls the sockets until at least one of them reports an event or
   //  an error occurs.  Returns the result of SysTcpSocket::Poll.
   //
   word PollSockets();

   //  Services the socket at curr_.
   //
   void ServiceSocket();

   //  Invoked to accept a connection.  Clears the PollRead flag if no
   //  connection request was pending.  Returns true if a connection was
   //  accepted.
   //
   bool AcceptConn();

   //  Adds SOCKET to the list of sockets when accepting a new connection.
   //
   virtual bool InsertSocket(SysSocket* socket) override;

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

   //  Generates a log when an error occurs.  EXPL is a text explanation,
   //  and ERROR is the general type of error.  SOCKET is where the error
   //  occurred, and ERRVAL is used if SOCKET is nullptr.
   //
   void OutputLog(fixed_string expl, Error error,
      SysTcpSocket* socket, debug32_t errval = 0) const;

   //  Releases resources when exiting or cleaning up the thread.
   //
   void ReleaseResources();

   //  The sockets associated with the port served by this thread.  The
   //  first socket listens for new connections, and each of the others
   //  handles an individual connection.
   //
   Array< SysTcpSocket* > sockets_;

   //  The number of sockets with events that still need to be serviced.
   //
   size_t ready_;

   //  The socket currently being serviced (used to index sockets_).
   //
   size_t curr_;
};
}
#endif
