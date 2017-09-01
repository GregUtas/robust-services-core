//==============================================================================
//
//  SysSocket.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SysSocket.h"
#include <sstream>
#include <string>
#include "Debug.h"
#include "IpBuffer.h"
#include "Log.h"
#include "NwTrace.h"
#include "NwTracer.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "Tool.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysSocket_ctor1 = "SysSocket.ctor(wrap)";

SysSocket::SysSocket(SysSocket_t socket) :
   socket_(socket),
   blocking_(true),
   disconnecting_(false),
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

   Close();
   TraceEvent(NwTrace::Delete, 0);
}

//------------------------------------------------------------------------------

void SysSocket::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "socket        : " << socket_ << CRLF;
   stream << prefix << "blocking      : " << blocking_ << CRLF;
   stream << prefix << "disconnecting : " << disconnecting_ << CRLF;
   stream << prefix << "tracing       : " << tracing_ << CRLF;
   stream << prefix << "error         : " << error_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name SysSocket_IsOpen = "SysSocket.IsOpen";

bool SysSocket::IsOpen() const
{
   Debug::ft(SysSocket_IsOpen);

   return (IsValid() && !disconnecting_);
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
   Debug::SwErr(SysSocket_SendBuff, 0, 0, ErrorLog);
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

NwTrace* SysSocket::TraceEvent(TraceRecord::Id rid, word data)
{
   Debug::ft(SysSocket_TraceEvent);

   if(!TraceEnabled()) return nullptr;
   if(tracing_) return new NwTrace(rid, this, data);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SysSocket_TracePeer = "SysSocket.TracePeer";

NwTrace* SysSocket::TracePeer
   (TraceRecord::Id rid, ipport_t port, const SysIpL3Addr& peer, word data)
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

NwTrace* SysSocket::TracePort(TraceRecord::Id rid, ipport_t port, word data)
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
