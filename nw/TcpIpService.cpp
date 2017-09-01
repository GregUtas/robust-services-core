//==============================================================================
//
//  TcpIpService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "TcpIpService.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"
#include "TcpIpPort.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name TcpIpService_ctor = "TcpIpService.ctor";

TcpIpService::TcpIpService()
{
   Debug::ft(TcpIpService_ctor);
}

//------------------------------------------------------------------------------

fn_name TcpIpService_dtor = "TcpIpService.dtor";

TcpIpService::~TcpIpService()
{
   Debug::ft(TcpIpService_dtor);
}

//------------------------------------------------------------------------------

fn_name TcpIpService_CreatePort = "TcpIpService.CreatePort";

IpPort* TcpIpService::CreatePort(ipport_t pid)
{
   Debug::ft(TcpIpService_CreatePort);

   return new TcpIpPort(pid, this);
}

//------------------------------------------------------------------------------

void TcpIpService::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   IpService::Display(stream, prefix, options);

   stream << prefix << "MaxConns   : " << MaxConns() << CRLF;
   stream << prefix << "MaxBacklog : " << MaxBacklog() << CRLF;
}

//------------------------------------------------------------------------------

fn_name TcpIpService_MaxBacklog = "TcpIpService.MaxBacklog";

size_t TcpIpService::MaxBacklog() const
{
   Debug::ft(TcpIpService_MaxBacklog);

   Debug::SwErr(TcpIpService_MaxBacklog, Name(), Sid());
   return 0;
}

//------------------------------------------------------------------------------

fn_name TcpIpService_MaxConns = "TcpIpService.MaxConns";

size_t TcpIpService::MaxConns() const
{
   Debug::ft(TcpIpService_MaxConns);

   Debug::SwErr(TcpIpService_MaxConns, Name(), Sid());
   return 0;
}

//------------------------------------------------------------------------------

void TcpIpService::Patch(sel_t selector, void* arguments)
{
   IpService::Patch(selector, arguments);
}
}
