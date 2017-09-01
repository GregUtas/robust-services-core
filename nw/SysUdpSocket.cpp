//==============================================================================
//
//  SysUdpSocket.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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