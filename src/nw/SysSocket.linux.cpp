//==============================================================================
//
//  SysSocket.linux.cpp
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

#include "SysSocket.h"
#include <endian.h>
#include <errno.h>
#include <iosfwd>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Debug.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "NwLogs.h"
#include "NwTrace.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
uint32_t htonl(uint32_t hostlong) { return ::htonl(hostlong); }

uint64_t htonll(uint64_t hostllong) { return htobe64(hostllong); }

uint16_t htons(uint16_t hostshort) { return ::htons(hostshort); }

uint32_t ntohl(uint32_t netlong) { return ::ntohl(netlong); }

uint64_t ntohll(uint64_t netllong) { return be64toh(netllong); }

uint16_t ntohs(uint16_t netshort) { return ::ntohs(netshort); }

//------------------------------------------------------------------------------

constexpr unsigned long IO_Blocking = 0;
constexpr unsigned long IO_NonBlocking = 1;

//------------------------------------------------------------------------------

fn_name SysSocket_ctor2 = "SysSocket.ctor";

SysSocket::SysSocket(ipport_t port, const IpService* service, AllocRc& rc) :
   socket_(INVALID_SOCKET),
   error_(0),
   port_(NilIpPort),
   blocking_(true),
   tracing_(false)
{
   Debug::ft(SysSocket_ctor2);

   //  Allocate a socket for UDP or TCP.
   //
   rc = AllocOk;
   auto proto = service->Protocol();
   auto family = (IpPortRegistry::UseIPv6() ? AF_INET6 : AF_INET);

   switch(proto)
   {
   case IpUdp:
      socket_ = socket(family, SOCK_DGRAM, IPPROTO_UDP);
      break;
   case IpTcp:
      socket_ = socket(family, SOCK_STREAM, IPPROTO_TCP);
      break;
   default:
      Debug::SwLog(SysSocket_ctor2, "unexpected protocol", proto);
      SetError(EPROTONOSUPPORT);
      rc = AllocFailed;
      return;
   }

   if(socket_ == INVALID_SOCKET)
   {
      OutputLog(NetworkAllocFailure, "socket", errno);
      rc = AllocFailed;
      return;
   }

   if(family == AF_INET6)
   {
      //  Configure the socket to support both IPv4 and IPv6.  This
      //  must be done before the socket is bound.
      //
      uint32_t dual = 0;

      if(setsockopt(socket_, IPPROTO_IPV6, IPV6_V6ONLY,
         (const char*)&dual, sizeof(dual)) != 0)
      {
         OutputLog(NetworkSocketError, "setsockopt/IPV6_V6ONLY", errno);
         rc = SetOptionError;
         return;
      }
   }

   rc = SetService(service, true);
   if(rc != AllocOk) return;

   if(port == NilIpPort) return;

   //  The desired port is known, so bind the socket against it.
   //
   sockaddr* addr = nullptr;
   int addrsize = 0;
   sockaddr_in ipv4addr;
   sockaddr_in6 ipv6addr;

   if(family == AF_INET)
   {
      ipv4addr.sin_family = AF_INET;
      ipv4addr.sin_addr.s_addr = htonl(INADDR_ANY);
      ipv4addr.sin_port = htons(port);
      addr = (sockaddr*) &ipv4addr;
      addrsize = sizeof(ipv4addr);
   }
   else
   {
      ipv6addr.sin6_family = AF_INET6;
      ipv6addr.sin6_addr = in6addr_any;
      ipv6addr.sin6_port = htons(port);
      ipv6addr.sin6_flowinfo = 0;
      ipv6addr.sin6_scope_id = 0;
      addr = (sockaddr*) &ipv6addr;
      addrsize = sizeof(ipv6addr);
   }

   if(bind(socket_, addr, addrsize) != 0)
   {
      OutputLog(NetworkSocketError, "bind", errno);
      rc = BindError;
      return;
   }

   port_ = port;
}

//------------------------------------------------------------------------------

c_string SysSocket::AlarmName(nwerr_t errval)
{
   Debug::ft("SysSocket.AlarmName");

   switch(errval)
   {
   case ENETDOWN:
      return NetworkAlarmName;
   }

   return EMPTY_STR;
}

//------------------------------------------------------------------------------

void SysSocket::Close(bool disconnecting)
{
   Debug::ft("SysSocket.Close");

   if(IsValid())
   {
      TraceEvent(NwTrace::Close, disconnecting);

      if(close(Socket()) != 0)
      {
         OutputLog(NetworkSocketError, "close", errno);
      }

      Invalidate();
   }
}

//------------------------------------------------------------------------------

bool SysSocket::Empty()
{
   Debug::ft("SysSocket.Empty");

   unsigned long bytecount = 0;

   //  Find out how many bytes are waiting to be read from the socket.
   //
   if(ioctl(socket_, FIONREAD, &bytecount) != 0)
   {
      OutputLog(NetworkSocketError, "ioctl/FIONREAD", errno);
      return true;
   }

   return (bytecount == 0);
}

//------------------------------------------------------------------------------

void SysSocket::Invalidate()
{
   Debug::ftnt("SysSocket.Invalidate");

   socket_ = INVALID_SOCKET;
   port_ = NilIpPort;
}

//------------------------------------------------------------------------------

bool SysSocket::IsValid() const
{
   return (socket_ != INVALID_SOCKET);
}

//------------------------------------------------------------------------------

bool SysSocket::SetBlocking(bool blocking)
{
   Debug::ft("SysSocket.SetBlocking");

   if(blocking_ == blocking) return true;

   auto mode = (blocking ? IO_Blocking : IO_NonBlocking);

   if(ioctl(socket_, FIONBIO, &mode) == 0)
   {
      blocking_ = blocking;
      return true;
   }

   OutputLog(NetworkSocketError, "ioctl/FIONBIO", errno);
   return false;
}

//------------------------------------------------------------------------------

fn_name SysSocket_SetService = "SysSocket.SetService";

SysSocket::AllocRc SysSocket::SetService(const IpService* service, bool shared)
{
   Debug::ft(SysSocket_SetService);

   auto rc = AllocOk;
   size_t max, rxSize, txSize;
   socklen_t maxsize = sizeof(max);

   if(shared)
   {
      rxSize = service->RxSize();
      txSize = service->TxSize();
   }
   else
   {
      service->GetAppSocketSizes(rxSize, txSize);
   }

   if(setsockopt(socket_, SOL_SOCKET, SO_RCVBUF,
      (const char*) &rxSize, sizeof(rxSize)) != 0)
   {
      OutputLog(NetworkSocketError, "setsockopt/RCVBUF", errno);
      return SetOptionError;
   }

   if(getsockopt(socket_, SOL_SOCKET, SO_RCVBUF,
      (char*) &max, &maxsize) != 0)
   {
      OutputLog(NetworkSocketError, "getsockopt/RCVBUF", errno);
      return GetOptionError;
   }

   if(max < rxSize)
   {
      Debug::SwLog(SysSocket_SetService, "rx size too large", rxSize);
   }

   if(setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
      (const char*) &txSize, sizeof(txSize)) != 0)
   {
      OutputLog(NetworkSocketError, "setsockopt/SNDBUF", errno);
      return SetOptionError;
   }

   if(getsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
      (char*) &max, &maxsize) != 0)
   {
      OutputLog(NetworkSocketError, "getsockopt/SNDBUF", errno);
      return GetOptionError;
   }

   if(max < txSize)
   {
      Debug::SwLog(SysSocket_SetService, "tx size too large", txSize);
   }

   return rc;
}

//------------------------------------------------------------------------------

bool SysSocket::StartLayer()
{
   Debug::ft("SysSocket.StartLayer");

   //  Linux does not require a process to set up the network layer.
   //
   return ReportLayerStart(EMPTY_STR);
}

//------------------------------------------------------------------------------

void SysSocket::StopLayer()
{
   Debug::ft("SysSocket.StopLayer");

   //  Linux does not require a process to take down the network layer.
}
}
#endif
