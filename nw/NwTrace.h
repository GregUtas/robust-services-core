//==============================================================================
//
//  NwTrace.h
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
#ifndef NWTRACE_H_INCLUDED
#define NWTRACE_H_INCLUDED

#include "TimedRecord.h"
#include "NwTypes.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Records a TCP socket event.
//
class NwTrace : public TimedRecord
{
public:
   //  Types of socket trace records.
   //
   static const Id Acquire    = 1;
   static const Id Release    = 2;
   static const Id Register   = 3;
   static const Id Deregister = 4;
   static const Id Connect    = 5;
   static const Id Listen     = 6;
   static const Id Poll       = 7;
   static const Id Accept     = 8;
   static const Id Recv       = 9;
   static const Id RecvFrom   = 10;
   static const Id Queue      = 11;
   static const Id Dispatch   = 12;
   static const Id Send       = 13;
   static const Id SendTo     = 14;
   static const Id Disconnect = 15;
   static const Id Close      = 16;
   static const Id Purge      = 17;
   static const Id Delete     = 18;

   //  Creates a trace record of type RID, which is associated with SOCKET.
   //  DATA is event specific.
   //
   NwTrace(Id rid, const SysSocket* socket, word data);

   //  Creates a trace record of type RID, which is associated with SOCKET
   //  and PORT.  DATA is event specific.
   //
   NwTrace(Id rid, const SysSocket* socket, word data, ipport_t port);

   //  Creates a trace record of type RID, which is associated with SOCKET,
   //  PORT, and PEER.  DATA is event specific.
   //
   NwTrace(Id rid, const SysSocket* socket, word data, ipport_t port,
      const SysIpL3Addr& peer);

   //  Overridden to display the trace record.
   //
   bool Display(std::ostream& stream, bool diff) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   const char* EventString() const override;

   //  The socket on which the event occurred.
   //
   const SysSocket* const socket_;

   //  Event-specific data.
   //
   const word data_;

   //  The host IP port associated with the event.
   //
   const ipport_t port_;

   //  The peer IP address associated with the event.
   //
   const SysIpL3Addr peer_;
};
}
#endif
