//==============================================================================
//
//  SysUdpSocket.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
SysUdpSocket::~SysUdpSocket()
{
   Debug::ftnt("SysUdpSocket.dtor");

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

   if(size > MaxUdpSize_)
   {
      Debug::SwLog(SysUdpSocket_SendBuff, "size too large", size);
      return SendFailed;
   }

   auto txport = buff.TxAddr().GetPort();
   auto port = Singleton<IpPortRegistry>::Instance()->GetPort(txport);
   const auto& peer = buff.RxAddr();
   auto data = port->GetHandler()->HostToNetwork(buff, src, size);
   auto sent = SendTo(data, size, peer);
   TracePeer(NwTrace::SendTo, txport, peer, sent);

   if(sent <= 0)
   {
      OutputLog(NetworkSocketError, "sendto", &buff);
      return SendFailed;
   }

   port->BytesSent(size);
   return SendOk;
}

//------------------------------------------------------------------------------

void SysUdpSocket::SendToSelf(ipport_t port)
{
   Debug::ft("SysUdpSocket.SendToSelf");

   SysIpL3Addr self(SysIpL2Addr::LoopbackIpAddr(), port);
   byte_t message[8] = { 'U', 'N', 'B', 'L', 'O', 'C', 'K', '!' };
   SendTo(message, 8, self);
}
}
