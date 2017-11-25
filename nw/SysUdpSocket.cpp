//==============================================================================
//
//  SysUdpSocket.cpp
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
#include "SysUdpSocket.h"
#include "Debug.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "NwTrace.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
size_t SysUdpSocket::MaxUdpSize_ = 0;

//------------------------------------------------------------------------------

fn_name SysUdpSocket_dtor = "SysUdpSocket.dtor";

SysUdpSocket::~SysUdpSocket()
{
   Debug::ft(SysUdpSocket_dtor);
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

   byte_t* start = nullptr;

   auto size = buff.OutgoingBytes(start);

   if(size > SysUdpSocket::MaxUdpSize_)
   {
      Debug::SwErr(SysUdpSocket_dtor, buff.TxAddr().GetPort(), size);
      return SendFailed;
   }

   auto port = buff.TxAddr().GetPort();
   auto& peer = buff.RxAddr();
   auto sent = SendTo(start, size, peer);
   TracePeer(NwTrace::SendTo, port, peer, sent);

   if(sent <= 0)
   {
      //s Handle SendTo() error.
      //
      OutputLog("UDP SENDTO ERROR", &buff);
      return SendFailed;
   }

   Singleton< IpPortRegistry >::Instance()->GetPort(port)->BytesSent(size);
   return SendOk;
}
}