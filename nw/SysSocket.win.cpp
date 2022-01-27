//==============================================================================
//
//  SysSocket.win.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifdef OS_WIN

#include "SysSocket.h"
#include <iosfwd>
#include <sstream>
#include <winerror.h>
#include <winsock2.h>
#include <windows.h>   // must follow winsock2.h
#include <ws2tcpip.h>
#include "Debug.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "NwTrace.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
uint32_t htonl(uint32_t hostlong) { return ::htonl(hostlong); }

uint64_t htonll(uint64_t hostllong) { return ::htonll(hostllong); }

uint16_t htons(uint16_t hostshort) { return ::htons(hostshort); }

uint32_t ntohl(uint32_t netlong) { return ::ntohl(netlong); }

uint64_t ntohll(uint32_t netllong) { return ::ntohll(netllong); }

uint16_t ntohs(uint16_t netshort) { return ::ntohs(netshort); }

//------------------------------------------------------------------------------

constexpr u_long IO_Blocking = 0;
constexpr u_long IO_NonBlocking = 1;

//------------------------------------------------------------------------------

fn_name SysSocket_ctor2 = "SysSocket.ctor";

SysSocket::SysSocket(ipport_t port, const IpService* service, AllocRc& rc) :
   socket_(INVALID_SOCKET),
   blocking_(true),
   tracing_(false),
   error_(0)
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
      SetError(WSAENOPROTOOPT);
      rc = AllocFailed;
      return;
   }

   if(socket_ == INVALID_SOCKET)
   {
      SetError();
      rc = AllocFailed;
      return;
   }

   if(family == AF_INET6)
   {
      //  Configure the socket to support both IPv4 and IPv6.  This
      //  must be done before the socket is bound.
      //
      DWORD dual = 0;

      if(setsockopt(socket_, IPPROTO_IPV6, IPV6_V6ONLY,
         (const char*)&dual, sizeof(dual)) == SOCKET_ERROR)
      {
         SetError();
         rc = SetOptionError;
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

   if(bind(socket_, addr, addrsize) == SOCKET_ERROR)
   {
      SetError();
      rc = BindError;
   }
}

//------------------------------------------------------------------------------

void SysSocket::Close(bool disconnecting)
{
   Debug::ft("SysSocket.Close");

   if(IsValid())
   {
      TraceEvent(NwTrace::Close, disconnecting);
      if(closesocket(Socket()) == SOCKET_ERROR) SetError();
      Invalidate();
   }
}

//------------------------------------------------------------------------------

bool SysSocket::Empty()
{
   Debug::ft("SysSocket.Empty");

   u_long bytecount = 0;

   //  Find out how many bytes are waiting to be read from the socket.
   //
   if(ioctlsocket(socket_, FIONREAD, &bytecount) != NO_ERROR)
   {
      SetError();
      return true;
   }

   return (bytecount == 0);
}

//------------------------------------------------------------------------------

void SysSocket::Invalidate()
{
   Debug::ftnt("SysSocket.Invalidate");

   socket_ = INVALID_SOCKET;
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

   if(ioctlsocket(socket_, FIONBIO, &mode) == NO_ERROR)
   {
      blocking_ = blocking;
      return true;
   }

   //s Handle ioctlsocket() error.
   //
   SetError();
   return false;
}

//------------------------------------------------------------------------------

word SysSocket::SetError()
{
   Debug::ft("SysSocket.SetError");

   error_ = WSAGetLastError();

   switch(error_)
   {
   case WSAENETDOWN:
   case WSASYSNOTREADY:
   case WSANOTINITIALISED:
      SetStatus(false, std::to_string(error_));
      break;
   }

   return -1;
}

//------------------------------------------------------------------------------

fn_name SysSocket_SetService = "SysSocket.SetService";

SysSocket::AllocRc SysSocket::SetService(const IpService* service, bool shared)
{
   Debug::ft(SysSocket_SetService);

   auto rc = AllocOk;
   size_t max, rxSize, txSize;
   int maxsize = sizeof(max);

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
      (const char*) &rxSize, sizeof(rxSize)) == SOCKET_ERROR)
   {
      SetError();
      return SetOptionError;
   }

   if(getsockopt(socket_, SOL_SOCKET, SO_RCVBUF,
      (char*) &max, &maxsize) == SOCKET_ERROR)
   {
      SetError();
      return GetOptionError;
   }

   if(max < rxSize)
      Debug::SwLog(SysSocket_SetService, "rx size too large", rxSize);

   if(setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
      (const char*) &txSize, sizeof(txSize)) == SOCKET_ERROR)
   {
      SetError();
      return SetOptionError;
   }

   if(getsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
      (char*) &max, &maxsize) == SOCKET_ERROR)
   {
      SetError();
      return GetOptionError;
   }

   if(max < txSize)
      Debug::SwLog(SysSocket_SetService, "tx size too large", txSize);
   return rc;
}

//------------------------------------------------------------------------------

bool SysSocket::StartLayer()
{
   Debug::ft("SysSocket.StartLayer");

   WSAData wsaData;
   auto wVersionRequested = MAKEWORD(2, 2);
   auto err = WSAStartup(wVersionRequested, &wsaData);

   if(err != 0)
   {
      return ReportLayerStart(std::to_string(err));
   }

   auto rls = HIBYTE(wsaData.wVersion);
   auto dot = LOBYTE(wsaData.wVersion);

   if((rls != 2) || (dot != 2))
   {
      std::ostringstream stream;
      stream << rls << '.' << dot;
      WSACleanup();
      return ReportLayerStart(stream.str());
   }

   return ReportLayerStart(EMPTY_STR);
}

//------------------------------------------------------------------------------

void SysSocket::StopLayer()
{
   Debug::ft("SysSocket.StopLayer");

   string err;

   if(WSACleanup() != 0)
   {
      err = std::to_string(WSAGetLastError());
   }

   ReportLayerStop(err);
}
}
#endif
