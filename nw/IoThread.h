//==============================================================================
//
//  IoThread.h
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
#ifndef IOTHREAD_H_INCLUDED
#define IOTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include "Clock.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  For efficiency, the preferred I/O design is one in which messages destined
//  for applications are "pushed" directly into an input handler, either from
//  an interrupt service routine or the task that implements the IP stack.
//  However, the standard practice in many platforms is to "pull" messages
//  using recvfrom (or something similar).  The recvfrom is performed by an
//  I/O thread which then pushes messages into the appropriate input handler.
//
class IoThread : public Thread
{
   friend class IpPort;
public:
   //> The maximum receive buffer size for a socket (in bytes).
   //
   static const size_t MaxRxBuffSize;

   //> The maximum transmit buffer size for a socket (in bytes).
   //
   static const size_t MaxTxBuffSize;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates an I/O thread that receives messages on PORT, on behalf of
   //  SERVICE.  Protected because this class is virtual.
   //
   IoThread(const IpService* service, ipport_t port);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~IoThread();

   //  Once a subclass has received a message and set txAddr_, rxAddr_, and
   //  ticks0_ accordingly, it invokes this to wrap the message and pass it to
   //  PORT's input handler.  SOURCE and SIZE identify the message's location.
   //
   void InvokeHandler(const IpPort& port,
      const byte_t* source, MsgSize size) const;

   //  Returns true after pausing when the thread has run locked for more
   //  than PERCENT of the maximum time allowed.
   //
   virtual bool ConditionalPause(word percent);

   //  Overridden to survive warm restarts.
   //
   virtual bool ExitOnRestart(RestartLevel level) const override;

   //  The port on which the thread receives messages.
   //
   const ipport_t port_;

   //  The IpPort registered against port_.  Must be set by a subclass
   //  constructor.
   //
   IpPort* ipPort_;

   //  The size of the receive buffer for the socket bound against port_.
   //
   size_t rxSize_;

   //  The size of the transmit buffer for the socket bound against port_.
   //
   size_t txSize_;

   //  The host address.
   //
   SysIpL2Addr host_;

   //  The number of messages received during the current work interval.
   //
   size_t recvs_;

   //  The (peer) address that sent the current incoming message.
   //
   SysIpL3Addr txAddr_;

   //  The (host) address on which the current message arrived.
   //
   SysIpL3Addr rxAddr_;

   //  The time when the current message arrived.
   //
   ticks_t ticks0_;

   //  The buffer for receiving messages.
   //
   byte_t* buffer_;
};
}
#endif
