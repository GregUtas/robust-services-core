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
#include "Alarm.h"
#include "AlarmRegistry.h"
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

void SysSocket::OutputLog
   (LogId id, fixed_string expl, const IpBuffer* buff) const
{
   Debug::ft(SysSocket_OutputLog);

   auto log = Log::Create(NetworkLogGroup, id);
   if(log == nullptr) return;

   *log << Log::Tab << expl << ": errval=" << GetError();

   if(buff != nullptr)
   {
      *log << " txPort=" << buff->TxAddr().GetPort();
      *log << " rxPort=" << buff->RxAddr().GetPort();
   }

   Log::Submit(log);
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

fn_name SysSocket_SetError1 = "SysSocket.SetError(errval)";

word SysSocket::SetError(word errval)
{
   Debug::ft(SysSocket_SetError1);

   error_ = errval;
   return -1;
}

//------------------------------------------------------------------------------

fn_name SysSocket_SetStatus = "SysSocket.SetStatus";

void SysSocket::SetStatus(bool ok, const string& err)
{
   auto reg = Singleton< AlarmRegistry >::Instance();
   auto alarm = reg->Find(NetworkAlarmName);
   auto status = (ok ? NoAlarm : CriticalAlarm);
   auto id = (ok ? NetworkAvailable : NetworkUnavailable);

   if(alarm == nullptr)
   {
      Debug::SwLog(SysSocket_SetStatus, err, status);
      return;
   }

   auto log = alarm->Create(NetworkLogGroup, id, status);

   if(log != nullptr)
   {
      if(!ok) *log << Log::Tab << "errval=" << err;
      Log::Submit(log);
   }
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
