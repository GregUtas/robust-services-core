//==============================================================================
//
//  SysSocket.cpp
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
#include "SysSocket.h"
#include <sstream>
#include <string>
#include "Debug.h"
#include "IpBuffer.h"
#include "Log.h"
#include "Memory.h"
#include "NwTrace.h"
#include "NwTracer.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "Tool.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name NetworkBase_HostToNetwork = "NetworkBase.HostToNetwork";

void HostToNetwork(byte_t* start, size_t size, uint8_t word)
{
   Debug::ft(NetworkBase_HostToNetwork);

   size_t odd = 0;

   switch(word)
   {
   case 0:
   case 1:
      //
      //  No conversion is required.
      //
      return;

   case 2:
   {
      odd = (size & 0x01);
      uint16_t* data = reinterpret_cast< uint16_t* >(start);

      for(size_t i = 0; i < (size >> 1); ++i)
      {
         data[i] = htons(data[i]);
      }

      break;
   }

   case 4:
   {
      odd = (size & 0x03);
      uint32_t* data = reinterpret_cast< uint32_t* >(start);

      for(size_t i = 0; i < (size >> 2); ++i)
      {
         data[i] = htonl(data[i]);
      }

      break;
   }

   case 8:
   {
      odd = (size & 0x07);
      uint64_t* data = reinterpret_cast< uint64_t* >(start);

      for(size_t i = 0; i < (size >> 3); ++i)
      {
         data[i] = htonll(data[i]);
      }

      break;
   }

   default:
      Debug::SwLog(NetworkBase_HostToNetwork, word, 0);
   }

   if(odd != 0) Debug::SwLog(NetworkBase_HostToNetwork, word, odd);
}

//------------------------------------------------------------------------------

fn_name NetworkBase_NetworkToHost = "NetworkBase.NetworkToHost";

void NetworkToHost(byte_t* dest, const byte_t* src, size_t size, uint8_t word)
{
   Debug::ft(NetworkBase_NetworkToHost);

   size_t odd = 0;

   switch(word)
   {
   case 0:
   case 1:
      Memory::Copy(dest, src, size);
      break;

   case 2:
   {
      odd = (size & 0x01);
      const uint16_t* from = reinterpret_cast< const uint16_t* >(src);
      uint16_t* to = reinterpret_cast< uint16_t* >(dest);

      for(size_t i = 0; i < (size >> 1); ++i)
      {
         to[i] = ntohs(from[i]);
      }

      break;
   }

   case 4:
   {
      odd = (size & 0x03);
      const uint32_t* from = reinterpret_cast< const uint32_t* >(src);
      uint32_t* to = reinterpret_cast< uint32_t* >(dest);

      for(size_t i = 0; i < (size >> 2); ++i)
      {
         to[i] = ntohl(from[i]);
      }

      break;
   }

   case 8:
   {
      odd = (size & 0x07);
      const uint64_t* from = reinterpret_cast< const uint64_t* >(src);
      uint64_t* to = reinterpret_cast< uint64_t* >(dest);

      for(size_t i = 0; i < (size >> 3); ++i)
      {
         to[i] = ntohll(from[i]);
      }

      break;
   }

   default:
      Debug::SwLog(NetworkBase_NetworkToHost, word, 0);
   }

   if(odd != 0) Debug::SwLog(NetworkBase_NetworkToHost, word, odd);
}

//==============================================================================

fn_name SysSocket_ctor1 = "SysSocket.ctor(wrap)";

SysSocket::SysSocket(SysSocket_t socket) :
   socket_(socket),
   blocking_(true),
   tracing_(false),
   error_(0)
{
   Debug::ft(SysSocket_ctor1);
}

//------------------------------------------------------------------------------

fn_name SysSocket_dtor = "SysSocket.dtor";

SysSocket::~SysSocket()
{
   Debug::ft(SysSocket_dtor);

   TraceEvent(NwTrace::Delete, 0);
}

//------------------------------------------------------------------------------

void SysSocket::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "socket        : " << socket_ << CRLF;
   stream << prefix << "blocking      : " << blocking_ << CRLF;
   stream << prefix << "tracing       : " << tracing_ << CRLF;
   stream << prefix << "error         : " << error_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name SysSocket_OutputLog = "SysSocket.OutputLog";

void SysSocket::OutputLog(fixed_string expl, const IpBuffer* buff) const
{
   Debug::ft(SysSocket_OutputLog);

   auto log = Log::Create(expl);
   if(log == nullptr) return;

   *log << " errval=" << GetError();

   if(buff != nullptr)
   {
      *log << " txPort=" << buff->TxAddr().GetPort();
      *log << " rxPort=" << buff->RxAddr().GetPort();
   }

   *log << CRLF;
   Log::Spool(log);
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

   //  This is a pure virtual function.
   //
   Debug::SwLog(SysSocket_SendBuff, socket_, 0);
   return SendFailed;
}

//------------------------------------------------------------------------------

fn_name SysSocket_SetError1 = "SysSocket.SetError(errval)";

word SysSocket::SetError(word errval)
{
   Debug::ft(SysSocket_SetError1);

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

fn_name SysSocket_TraceEvent = "SysSocket.TraceEvent";

NwTrace* SysSocket::TraceEvent(TraceRecordId rid, word data)
{
   Debug::ft(SysSocket_TraceEvent);

   if(!TraceEnabled()) return nullptr;
   if(tracing_) return new NwTrace(rid, this, data);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SysSocket_TracePeer = "SysSocket.TracePeer";

NwTrace* SysSocket::TracePeer
   (TraceRecordId rid, ipport_t port, const SysIpL3Addr& peer, word data)
{
   Debug::ft(SysSocket_TracePeer);

   if(!TraceEnabled()) return nullptr;

   auto nwt = Singleton< NwTracer >::Instance();

   if(tracing_ || Trace(nwt->PortStatus(port)) || Trace(nwt->PeerStatus(peer)))
   {
      return new NwTrace(rid, this, data, port, peer);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SysSocket_TracePort = "SysSocket.TracePort";

NwTrace* SysSocket::TracePort(TraceRecordId rid, ipport_t port, word data)
{
   Debug::ft(SysSocket_TracePort);

   if(!TraceEnabled()) return nullptr;

   if(tracing_ || Trace(Singleton< NwTracer >::Instance()->PortStatus(port)))
   {
      return new NwTrace(rid, this, data, port);
   }

   return nullptr;
}
}
