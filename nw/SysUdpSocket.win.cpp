//==============================================================================
//
//  SysUdpSocket.win.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifdef OS_WIN
#include "SysUdpSocket.h"
#include <winsock2.h>
#include "Debug.h"
#include "SysIpL3Addr.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysUdpSocket_ctor = "SysUdpSocket.ctor";

SysUdpSocket::SysUdpSocket(ipport_t port, size_t rxSize, size_t txSize,
   AllocRc& rc) : SysSocket(port, IpUdp, rxSize, txSize, rc)
{
   Debug::ft(SysUdpSocket_ctor);

   //  If the maximum UDP message size has not been set, set it now.
   //
   if((MaxUdpSize_ == 0) && (rc == AllocOk))
   {
      size_t max;
      int maxsize = sizeof(max);

      if(getsockopt(Socket(), SOL_SOCKET, SO_MAX_MSG_SIZE,
         (char*) &max, &maxsize) == SOCKET_ERROR)
      {
         SetError();
         rc = GetOptionError;
         return;
      }

      MaxUdpSize_ = (max < MaxMsgSize ? max : MaxMsgSize);
   }
}

//------------------------------------------------------------------------------

fn_name SysUdpSocket_RecvFrom = "SysUdpSocket.RecvFrom";

word SysUdpSocket::RecvFrom(byte_t* buff, size_t max, SysIpL3Addr& remAddr)
{
   Debug::ft(SysUdpSocket_RecvFrom);

   sockaddr_in peer;
   int peersize = sizeof(peer);

   if((buff == nullptr) || (max == 0))
   {
      Debug::SwErr(SysUdpSocket_RecvFrom, max, 0);
      return 0;
   }

   auto rcvd = recvfrom(Socket(), reinterpret_cast< char* >(buff),
      max, 0, (sockaddr*) &peer, &peersize);

   if(rcvd == SOCKET_ERROR)
   {
      SetError();
      if(GetError() == WSAEWOULDBLOCK) return -2;
      return -1;
   }

   remAddr = SysIpL3Addr(ntohl(peer.sin_addr.s_addr), ntohs(peer.sin_port));
   return rcvd;
}

//------------------------------------------------------------------------------

fn_name SysUdpSocket_SendTo = "SysUdpSocket.SendTo";

word SysUdpSocket::SendTo
   (const byte_t* data, size_t len, const SysIpL3Addr& remAddr)
{
   Debug::ft(SysUdpSocket_SendTo);

   sockaddr_in peer;
   int peersize = sizeof(peer);

   if((data == nullptr) || (len == 0))
   {
      Debug::SwErr(SysUdpSocket_SendTo, len, 0);
      return 0;
   }

   if(!SetBlocking(false)) return GetError();

   peer.sin_family = AF_INET;
   peer.sin_addr.s_addr = htonl(remAddr.GetIpV4Addr());
   peer.sin_port = htons(remAddr.GetPort());

   auto sent = sendto(Socket(), reinterpret_cast< const char* >(data),
      len, 0, (sockaddr*) &peer, peersize);

   if(sent == SOCKET_ERROR) return SetError();
   return sent;
}
}
#endif