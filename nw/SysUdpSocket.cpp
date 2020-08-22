//==============================================================================
//
//  SysUdpSocket.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "SysUdpSocket.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "InputHandler.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "NwLogs.h"
#include "NwTrace.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
size_t SysUdpSocket::MaxUdpSize_ = 0;

//------------------------------------------------------------------------------

fn_name SysUdpSocket_dtor = "SysUdpSocket.dtor";

SysUdpSocket::~SysUdpSocket()
{
   Debug::ftnt(SysUdpSocket_dtor);

   Close(false);
}

//------------------------------------------------------------------------------

void SysUdpSocket::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   SysSocket::Display(stream, prefix, options);

   stream << prefix << "MaxUdpSize : " << MaxUdpSize_ << CRLF;
}

//------------------------------------------------------------------------------

void SysUdpSocket::Patch(sel_t selector, void* arguments)
{
   SysSocket::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SysUdpSocket_SendBuff = "SysUdpSocket.SendBuff";

SysSocket::SendRc SysUdpSocket::SendBuff(IpBuffer& buff)
{
   Debug::ft(SysUdpSocket_SendBuff);

   byte_t* src = nullptr;

   auto size = buff.OutgoingBytes(src);

   if(size > SysUdpSocket::MaxUdpSize_)
   {
      Debug::SwLog(SysUdpSocket_SendBuff, "size too large", size);
      return SendFailed;
   }

   auto txport = buff.TxAddr().GetPort();
   auto port = Singleton< IpPortRegistry >::Instance()->GetPort(txport);
   auto& peer = buff.RxAddr();
   auto dest = port->GetHandler()->HostToNetwork(buff, src, size);
   auto sent = SendTo(dest, size, peer);
   TracePeer(NwTrace::SendTo, txport, peer, sent);

   if(sent <= 0)
   {
      //s Handle SendTo() error.
      //
      OutputLog(NetworkSocketError, "SendTo", &buff);
      return SendFailed;
   }

   port->BytesSent(size);
   return SendOk;
}
}
