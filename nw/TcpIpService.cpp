//==============================================================================
//
//  TcpIpService.cpp
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
#include "TcpIpService.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"
#include "TcpIpPort.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
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
   stream << prefix << "Keepalive  : " << Keepalive() << CRLF;
}

//------------------------------------------------------------------------------

fn_name TcpIpService_MaxBacklog = "TcpIpService.MaxBacklog";

size_t TcpIpService::MaxBacklog() const
{
   Debug::ft(TcpIpService_MaxBacklog);

   Debug::SwLog(TcpIpService_MaxBacklog, Name(), Sid());
   return 0;
}

//------------------------------------------------------------------------------

fn_name TcpIpService_MaxConns = "TcpIpService.MaxConns";

size_t TcpIpService::MaxConns() const
{
   Debug::ft(TcpIpService_MaxConns);

   Debug::SwLog(TcpIpService_MaxConns, Name(), Sid());
   return 0;
}

//------------------------------------------------------------------------------

void TcpIpService::Patch(sel_t selector, void* arguments)
{
   IpService::Patch(selector, arguments);
}
}
