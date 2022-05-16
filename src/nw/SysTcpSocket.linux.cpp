//==============================================================================
//
//  SysTcpSocket.linux.cpp
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
#ifdef OS_LINUX

#include "SysTcpSocket.h"
#include <chrono>
#include <errno.h>
#include <memory>
#include <netinet/in.h>
#include <ratio>
#include <sys/poll.h>
#include <sys/socket.h>
#include "Debug.h"
#include "IpPortRegistry.h"
#include "NwLogs.h"
#include "NwTrace.h"
#include "SysIpL3Addr.h"
#include "TcpIpService.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
SysTcpSocketPtr SysTcpSocket::Accept(SysIpL3Addr& remAddr)
{
   Debug::ft("SysTcpSocket.Accept");

   sockaddr_in ipv4peer;
   sockaddr_in6 ipv6peer;
   sockaddr* peer = nullptr;
   socklen_t peersize = 0;

   auto ipv6 = IpPortRegistry::UseIPv6();

   if(ipv6)
   {
      peer = (sockaddr*) &ipv6peer;
      peersize = sizeof(ipv6peer);
   }
   else
   {
      peer = (sockaddr*) &ipv4peer;
      peersize = sizeof(ipv4peer);
   }

   auto socket = accept(Socket(), peer, &peersize);

   if(socket == INVALID_SOCKET)
   {
      SetError(errno);
      if(errno == EWOULDBLOCK) outFlags_.reset(PollRead);
      return nullptr;
   }

   auto port = NilIpPort;

   if(ipv6)
   {
      remAddr = SysIpL3Addr(ipv6peer.sin6_addr.s6_addr16,
         ipv6peer.sin6_port, IpTcp, this);
      port = ipv6peer.sin6_port;
   }
   else
   {
      remAddr = SysIpL3Addr(ipv4peer.sin_addr.s_addr,
         ipv4peer.sin_port, IpTcp, this);
      port = ipv4peer.sin_port;
   }

   return SysTcpSocketPtr(new SysTcpSocket(socket, port));
}

//------------------------------------------------------------------------------

word SysTcpSocket::Connect(const SysIpL3Addr& remAddr)
{
   Debug::ft("SysTcpSocket.Connect");

   sockaddr_in ipv4peer;
   sockaddr_in6 ipv6peer;
   sockaddr* peer = nullptr;
   socklen_t peersize = 0;

   auto ipv6 = IpPortRegistry::UseIPv6();

   if(ipv6)
   {
      ipv6peer.sin6_family = AF_INET6;
      remAddr.HostToNetwork(ipv6peer.sin6_addr.s6_addr16, ipv6peer.sin6_port);
      ipv6peer.sin6_flowinfo = 0;
      ipv6peer.sin6_scope_id = 0;
      peer = (sockaddr*) &ipv6peer;
      peersize = sizeof(ipv6peer);
   }
   else
   {
      ipv4peer.sin_family = AF_INET;
      remAddr.HostToNetwork(ipv4peer.sin_addr.s_addr, ipv4peer.sin_port);
      peer = (sockaddr*) &ipv4peer;
      peersize = sizeof(ipv4peer);
   }

   if(connect(Socket(), peer, peersize) != 0)
   {
      SetError(errno);
      if(errno != EWOULDBLOCK) return errno;
   }

   return 0;
}

//------------------------------------------------------------------------------

void SysTcpSocket::Disconnect()
{
   Debug::ft("SysTcpSocket.Disconnect");

   if(!disconnecting_ && (state_ != Idle) && IsValid())
   {
      TraceEvent(NwTrace::Disconnect, 0);

      if(shutdown(Socket(), SHUT_WR) != 0)
      {
         switch(errno)
         {
         case ECONNRESET:  // peer has disconnected
         case ENOTCONN:    // connect() still pending
            break;
         default:
            OutputLog(NetworkSocketError, "shutdown", errno);
         }
      }

      disconnecting_ = true;
   }
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Listen = "SysTcpSocket.Listen";

word SysTcpSocket::Listen(size_t backlog)
{
   Debug::ft(SysTcpSocket_Listen);

   if(backlog > SOMAXCONN)
   {
      Debug::SwLog(SysTcpSocket_Listen, "backlog too large", backlog);
      backlog = SOMAXCONN;
   }

   if(listen(Socket(), backlog) != 0)
   {
      SetError(errno);
      return -1;
   }

   state_ = Listening;
   return 0;
}

//------------------------------------------------------------------------------

bool SysTcpSocket::LocAddr(SysIpL3Addr& locAddr)
{
   Debug::ft("SysTcpSocket.LocAddr");

   sockaddr_in ipv4self;
   sockaddr_in6 ipv6self;
   sockaddr* self = nullptr;
   socklen_t selfsize = 0;

   auto ipv6 = IpPortRegistry::UseIPv6();

   if(ipv6)
   {
      self = (sockaddr*) &ipv6self;
      selfsize = sizeof(ipv6self);
   }
   else
   {
      self = (sockaddr*) &ipv4self;
      selfsize = sizeof(ipv4self);
   }

   if(getsockname(Socket(), self, &selfsize) != 0)
   {
      OutputLog(NetworkSocketError, "getsockname", errno);
      return false;
   }

   if(ipv6)
      locAddr = SysIpL3Addr(ipv6self.sin6_addr.s6_addr16,
         ipv6self.sin6_port, IpTcp, nullptr);
   else
      locAddr = SysIpL3Addr(ipv4self.sin_addr.s_addr,
         ipv4self.sin_port, IpTcp, nullptr);
   return true;
}

//------------------------------------------------------------------------------

void SysTcpSocket::Patch(sel_t selector, void* arguments)
{
   SysSocket::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

word SysTcpSocket::Poll
   (SysTcpSocket* sockets[], size_t size, const msecs_t& timeout)
{
   Debug::ft("SysTcpSocket.Poll");

   if(size == 0) return 0;
   int delay = (timeout != TIMEOUT_NEVER ? timeout.count() : -1);

   //  Create an array for the sockets and their flags.
   //
   std::unique_ptr< pollfd[] > list(new pollfd[size]);

   for(size_t i = 0; i < size; ++i)
   {
      list[i].fd = sockets[i]->Socket();
      auto& inFlags = sockets[i]->inFlags_;
      auto& requests = list[i].events;

      requests = 0;
      if(inFlags.none()) continue;

      if(inFlags.test(PollWrite)) requests |= POLLWRNORM;
      if(inFlags.test(PollWriteOob)) requests |= POLLWRBAND;
      if(inFlags.test(PollRead)) requests |= POLLRDNORM;
      if(inFlags.test(PollReadOob)) requests |= POLLRDBAND;
   }

   auto ready = poll(list.get(), size, delay);

   if(ready < 0)
   {
      return sockets[0]->OutputLog(NetworkSocketError, "WSAPoll", errno);
   }

   //  Save the status of each socket before LIST gets deleted.
   //
   for(size_t i = 0; i < size; ++i)
   {
      auto results = list[i].revents;
      auto& outFlags = sockets[i]->outFlags_;

      outFlags.reset();
      if(results == 0) continue;

      if((results & POLLERR) != 0) outFlags.set(PollError);
      if((results & POLLHUP) != 0) outFlags.set(PollHungUp);
      if((results & POLLNVAL) != 0) outFlags.set(PollInvalid);
      if((results & POLLWRNORM) != 0) outFlags.set(PollWrite);
      if((results & POLLWRBAND) != 0) outFlags.set(PollWriteOob);
      if((results & POLLRDNORM) != 0) outFlags.set(PollRead);
      if((results & POLLRDBAND) != 0) outFlags.set(PollReadOob);
   }

   list.reset();
   return ready;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Recv = "SysTcpSocket.Recv";

word SysTcpSocket::Recv(byte_t* buff, size_t size)
{
   Debug::ft(SysTcpSocket_Recv);

   if(buff == nullptr)
   {
      Debug::SwLog(SysTcpSocket_Recv, "invalid buffer", 0);
      return -2;
   }

   if(size == 0)
   {
      Debug::SwLog(SysTcpSocket_Recv, "invalid size", size);
      return -2;
   }

   auto rcvd = recv(Socket(), reinterpret_cast< char* >(buff), size, 0);
   TraceEvent(NwTrace::Recv, rcvd);

   if(rcvd < 0)
   {
      return SetError(errno);
   }

   NetworkIsUp();
   return rcvd;
}

//------------------------------------------------------------------------------

bool SysTcpSocket::RemAddr(SysIpL3Addr& remAddr)
{
   Debug::ft("SysTcpSocket.RemAddr");

   sockaddr_in ipv4peer;
   sockaddr_in6 ipv6peer;
   sockaddr* peer = nullptr;
   socklen_t peersize = 0;

   auto ipv6 = IpPortRegistry::UseIPv6();

   if(ipv6)
   {
      peer = (sockaddr*) &ipv6peer;
      peersize = sizeof(ipv6peer);
   }
   else
   {
      peer = (sockaddr*) &ipv4peer;
      peersize = sizeof(ipv4peer);
   }

   if(getpeername(Socket(), peer, &peersize) != 0)
   {
      OutputLog(NetworkSocketError, "getpeername", errno);
      return false;
   }

   if(ipv6)
      remAddr = SysIpL3Addr(ipv6peer.sin6_addr.s6_addr16,
         ipv6peer.sin6_port, IpTcp, this);
   else
      remAddr = SysIpL3Addr(ipv4peer.sin_addr.s_addr,
         ipv4peer.sin_port, IpTcp, this);
   return true;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Send = "SysTcpSocket.Send";

word SysTcpSocket::Send(const byte_t* data, size_t size)
{
   Debug::ft(SysTcpSocket_Send);

   if(data == nullptr)
   {
      Debug::SwLog(SysTcpSocket_Send, "invalid data", 0);
      return -2;
   }

   if(size == 0)
   {
      Debug::SwLog(SysTcpSocket_Send, "invalid size", size);
      return -2;
   }

   auto sent = send(Socket(), reinterpret_cast< const char* >(data), size, 0);

   if(sent < 0)
   {
      sent = SetError(errno);
      if(GetError() == EWOULDBLOCK) sent = 0;
   }
   else
   {
      NetworkIsUp();
   }

   TraceEvent(NwTrace::Send, sent);
   return sent;
}

//------------------------------------------------------------------------------

bool SysTcpSocket::SetClose(bool graceful)
{
   Debug::ft("SysTcpSocket.SetClose");

   linger linger_opts;
   linger_opts.l_onoff = 0;
   linger_opts.l_linger = (graceful ? 0 : 1);

   if(setsockopt(Socket(), SOL_SOCKET, SO_LINGER,
      (char*) &linger_opts, sizeof(linger)) == 0)
   {
      return true;
   }

   OutputLog(NetworkSocketError, "setsockopt/LINGER", errno);
   return false;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_SetService = "SysTcpSocket.SetService";

SysSocket::AllocRc SysTcpSocket::SetService
   (const IpService* service, bool shared)
{
   Debug::ft(SysTcpSocket_SetService);

   //  Configure SERVICE's socket settings followed by its TCP settings.
   //
   auto rc = SysSocket::SetService(service, shared);
   if(rc != AllocOk) return rc;

   bool alive = static_cast< const TcpIpService* >(service)->Keepalive();

   if(setsockopt(Socket(), SOL_SOCKET, SO_KEEPALIVE,
      (const char*) &alive, sizeof(alive)) != 0)
   {
      OutputLog(NetworkSocketError, "setsockopt/KEEPALIVE", errno);
      return SetOptionError;
   }

   bool val;
   socklen_t valsize = sizeof(val);

   if(getsockopt(Socket(), SOL_SOCKET, SO_KEEPALIVE,
      (char*) &val, &valsize) != 0)
   {
      OutputLog(NetworkSocketError, "getsockopt/KEEPALIVE", errno);
      return GetOptionError;
   }

   if(val != alive)
   {
      Debug::SwLog(SysTcpSocket_SetService, "keepalive not set", val);
   }

   return AllocOk;
}
}
#endif
