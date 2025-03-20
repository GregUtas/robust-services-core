//==============================================================================
//
//  NwTrace.cpp
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
#include "NwTrace.h"
#include <ostream>
#include <string>
#include "Formatters.h"
#include "ToolTypes.h"
#include "TraceDump.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
NwTrace::NwTrace(Id rid, const SysSocket* socket, word data) :
   TimedRecord(NetworkTracer),
   socket_(socket),
   data_(data),
   port_(NilIpPort)
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

NwTrace::NwTrace(Id rid, const SysSocket* socket, word data, ipport_t port) :
   TimedRecord(NetworkTracer),
   socket_(socket),
   data_(data),
   port_(port)
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

NwTrace::NwTrace(Id rid, const SysSocket* socket, word data, ipport_t port,
   const SysIpL3Addr& peer) : TimedRecord(NetworkTracer),
   socket_(socket),
   data_(data),
   port_(port),
   peer_(peer)
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

bool NwTrace::Display(ostream& stream, const string& opts)
{
   if(!TimedRecord::Display(stream, opts)) return false;

   stream << spaces(TraceDump::EvtToObj);

   if(socket_ != nullptr)
      stream << socket_;
   else
      stream << spaces(NIBBLES_PER_POINTER);

   stream << TraceDump::Tab();

   switch(rid_)
   {
   case Acquire:
   case Release:
   case Register:
   case Deregister:
   case Queue:
   case Dispatch:
   case Purge:
   case Delete:
      stream << "state=" << data_;
      break;
   case Connect:
      stream << "peer=" << peer_.to_str(false) << " rc=" << data_;
      break;
   case Listen:
      stream << "port=" << port_ << " backlog=" << data_;
      break;
   case Poll:
      stream << "ready=" << data_;
      break;
   case Accept:
      stream << "port=" << port_ << " peer=" << peer_.to_str(false);
      break;
   case Recv:
      stream << "rcvd=" << data_;
      break;
   case RecvFrom:
      stream << "rcvd=" << data_;
      stream << " port=" << port_ << " peer=" << peer_.to_str(false);
      break;
   case Send:
      stream << "sent=" << data_;
      break;
   case SendTo:
      stream << "sent=" << data_;
      stream << " port=" << port_ << " peer=" << peer_.to_str(false);
      break;
   case Disconnect:
      break;
   case Close:
      stream << "hard=" << (data_ == 0);
      break;
   }

   return true;
}

//------------------------------------------------------------------------------

fixed_string AcquireEventStr    = " +app";
fixed_string ReleaseEventStr    = " -app";
fixed_string RegisterEventStr   = " +iot";
fixed_string DeregisterEventStr = " -iot";
fixed_string ConnectEventStr    = " conn";
fixed_string ListenEventStr     = " lstn";
fixed_string PollEventStr       = " poll";
fixed_string AcceptEventStr     = " acpt";
fixed_string RecvEventStr       = " recv";
fixed_string RecvFromEventStr   = "urecv";
fixed_string QueueEventStr      = "queue";
fixed_string DispatchEventStr   = " disp";
fixed_string SendEventStr       = " send";
fixed_string SendToEventStr     = "usend";
fixed_string DisconnectEventStr = " disc";
fixed_string CloseEventStr      = "close";
fixed_string PurgeEventStr      = "purge";
fixed_string DeleteEventStr     = "-sock";

c_string NwTrace::EventString() const
{
   switch(rid_)
   {
   case Acquire: return AcquireEventStr;
   case Release: return ReleaseEventStr;
   case Register: return RegisterEventStr;
   case Deregister: return DeregisterEventStr;
   case Connect: return ConnectEventStr;
   case Listen: return ListenEventStr;
   case Poll: return PollEventStr;
   case Accept: return AcceptEventStr;
   case Recv: return RecvEventStr;
   case RecvFrom: return RecvFromEventStr;
   case Queue: return QueueEventStr;
   case Dispatch: return DispatchEventStr;
   case Send: return SendEventStr;
   case SendTo: return SendToEventStr;
   case Disconnect: return DisconnectEventStr;
   case Close: return CloseEventStr;
   case Purge: return PurgeEventStr;
   case Delete: return DeleteEventStr;
   }

   return ERROR_STR;
}
}
