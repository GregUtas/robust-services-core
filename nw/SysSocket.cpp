//==============================================================================
//
//  SysSocket.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "SysSocket.h"
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "IpBuffer.h"
#include "Log.h"
#include "NwLogs.h"
#include "NwTrace.h"
#include "NwTracer.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "Tool.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
SysSocket::SysSocket(SysSocket_t socket, ipport_t port) :
   socket_(socket),
   error_(0),
   port_(port),
   blocking_(true),
   tracing_(false)
{
   Debug::ft("SysSocket.ctor(wrap)");
}

//------------------------------------------------------------------------------

SysSocket::~SysSocket()
{
   Debug::ftnt("SysSocket.dtor");

   TraceEvent(NwTrace::Delete, 0);
}

//------------------------------------------------------------------------------

void SysSocket::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "socket   : " << socket_ << CRLF;
   stream << prefix << "error    : " << error_ << CRLF;
   stream << prefix << "port     : " << port_ << CRLF;
   stream << prefix << "blocking : " << blocking_ << CRLF;
   stream << prefix << "tracing  : " << tracing_ << CRLF;
}

//------------------------------------------------------------------------------

nwerr_t SysSocket::OutputLog(LogId id, c_string func, nwerr_t errval)
{
   Debug::ft("SysSocket.OutputLog(errval)");

   //  Generate a log that appends information about this socket.
   //
   error_ = errval;

   std::ostringstream stream;

   stream << CRLF << Log::Tab << to_str();
   OutputNwLog(id, func, errval, stream.str().c_str());
   return -1;
}

//------------------------------------------------------------------------------

void SysSocket::OutputLog(LogId id, c_string func, const IpBuffer* buff) const
{
   Debug::ft("SysSocket.OutputLog(buff)");

   //  Generate a log that appends BUFF's addresses.
   //
   std::ostringstream stream;

   stream << CRLF;

   if(buff != nullptr)
   {
      stream << Log::Tab << "txAddr=" << buff->TxAddr().to_str(true) << CRLF;
      stream << Log::Tab << "rxAddr=" << buff->RxAddr().to_str(true);
   }
   else
   {
      stream << Log::Tab << to_str();
   }

   OutputNwLog(id, func, GetError(), stream.str().c_str());
}

//------------------------------------------------------------------------------

void SysSocket::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SysSocket_SendBuff = "SysSocket.SendBuff";

SysSocket::SendRc SysSocket::SendBuff(IpBuffer& buff)
{
   Debug::ft(SysSocket_SendBuff);

   Debug::SwLog(SysSocket_SendBuff, strOver(this), 0);
   return SendFailed;
}

//------------------------------------------------------------------------------

nwerr_t SysSocket::SetError(nwerr_t errval)
{
   Debug::ft("SysSocket.SetError(errval)");

   error_ = errval;
   return -1;
}

//------------------------------------------------------------------------------

bool SysSocket::SetTracing(bool tracing)
{
   tracing_ = tracing;
   return tracing;
}

//------------------------------------------------------------------------------

string SysSocket::to_str() const
{
   std::ostringstream stream;
   stream << "socket=" << socket_;

   if(port_ != NilIpPort) stream << " port=" << port_;
   return stream.str();
}

//------------------------------------------------------------------------------

bool SysSocket::Trace(TraceStatus status)
{
   auto tracer = Singleton< TraceBuffer >::Instance();
   if(!tracer->ToolIsOn(NetworkTracer)) return false;
   if(status == TraceIncluded) return SetTracing(true);
   if(status == TraceExcluded) return false;
   return SetTracing(tracer->FilterIsOn(TraceAll));
}

//------------------------------------------------------------------------------

bool SysSocket::TraceEnabled()
{
   return (Debug::TraceOn() ? true : SetTracing(false));
}

//------------------------------------------------------------------------------

NwTrace* SysSocket::TraceEvent(TraceRecordId rid, word data)
{
   Debug::ft("SysSocket.TraceEvent");

   if(!TraceEnabled()) return nullptr;

   if(tracing_)
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      auto rec = new NwTrace(rid, this, data);
      if(buff->Insert(rec)) return rec;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

NwTrace* SysSocket::TracePeer
   (TraceRecordId rid, ipport_t port, const SysIpL3Addr& peer, word data)
{
   Debug::ft("SysSocket.TracePeer");

   if(!TraceEnabled()) return nullptr;

   auto nwt = Singleton< NwTracer >::Instance();

   if(tracing_ || Trace(nwt->PortStatus(port)) || Trace(nwt->PeerStatus(peer)))
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      auto rec = new NwTrace(rid, this, data, port, peer);
      if(buff->Insert(rec)) return rec;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

NwTrace* SysSocket::TracePort(TraceRecordId rid, ipport_t port, word data)
{
   Debug::ft("SysSocket.TracePort");

   if(!TraceEnabled()) return nullptr;

   if(tracing_ || Trace(Singleton< NwTracer >::Instance()->PortStatus(port)))
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      auto rec = new NwTrace(rid, this, data, port);
      if(buff->Insert(rec)) return rec;
   }

   return nullptr;
}
}
