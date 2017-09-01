//==============================================================================
//
//  NwTrace.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NWTRACE_H_INCLUDED
#define NWTRACE_H_INCLUDED

#include "TimedRecord.h"
#include "NwTypes.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
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
   virtual bool Display(std::ostream& stream) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

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
