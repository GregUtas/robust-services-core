//==============================================================================
//
//  SysSocket.win.cpp
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
#ifdef OS_WIN
#include "SysSocket.h"
#include <iosfwd>
#include <sstream>
#include <winsock2.h>  // must precede windows.h
#include <windows.h>
#include <winerror.h>
#include "Debug.h"
#include "IpService.h"
#include "Log.h"
#include "NwLogs.h"
#include "NwTrace.h"

using namespace NodeBase;

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

const u_long IO_Blocking = 0;
const u_long IO_NonBlocking = 1;

//------------------------------------------------------------------------------

fn_name SysSocket_ctor2 = "SysSocket.ctor";

SysSocket::SysSocket(ipport_t port, const IpService* service, AllocRc& rc) :
   socket_(INVALID_SOCKET),
   blocking_(true),
   tracing_(false),
   error_(0)
{
   Debug::ft(SysSocket_ctor2);

   sockaddr_in addr;

   rc = AllocOk;
   auto proto = service->Protocol();

   switch(proto)
   {
   case IpUdp:
      socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      break;
   case IpTcp:
      socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

   rc = SetService(service, true);
   if(rc != AllocOk) return;

   if(port == NilIpPort) return;

   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);

   if(bind(socket_, (sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR)
   {
      SetError();
      rc = BindError;
   }
}

//------------------------------------------------------------------------------

fn_name SysSocket_Close = "SysSocket.Close";

void SysSocket::Close(bool disconnecting)
{
   Debug::ft(SysSocket_Close);

   if(IsValid())
   {
      TraceEvent(NwTrace::Close, disconnecting);
      if(closesocket(Socket()) == SOCKET_ERROR) SetError();
      Invalidate();
   }
}

//------------------------------------------------------------------------------

fn_name SysSocket_Empty = "SysSocket.Empty";

bool SysSocket::Empty()
{
   Debug::ft(SysSocket_Empty);

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

fn_name SysSocket_Invalidate = "SysSocket.Invalidate";

void SysSocket::Invalidate()
{
   Debug::ftnt(SysSocket_Invalidate);

   socket_= INVALID_SOCKET;
}

//------------------------------------------------------------------------------

bool SysSocket::IsValid() const
{
   return (socket_ != INVALID_SOCKET);
}

//------------------------------------------------------------------------------

fn_name SysSocket_SetBlocking = "SysSocket.SetBlocking";

bool SysSocket::SetBlocking(bool blocking)
{
   Debug::ft(SysSocket_SetBlocking);

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

fn_name SysSocket_SetError = "SysSocket.SetError";

word SysSocket::SetError()
{
   Debug::ft(SysSocket_SetError);

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

   auto rc = SysSocket::AllocOk;
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

fn_name SysSocket_StartLayer = "SysSocket.StartLayer";

bool SysSocket::StartLayer()
{
   Debug::ft(SysSocket_StartLayer);

   WSAData wsaData;

   auto wVersionRequested = MAKEWORD(2, 2);

   auto err = WSAStartup(wVersionRequested, &wsaData);

   if(err != 0)
   {
      SetStatus(false, std::to_string(err));
      return false;
   }

   auto rls = HIBYTE(wsaData.wVersion);
   auto dot = LOBYTE(wsaData.wVersion);

   if((rls != 2) || (dot != 2))
   {
      std::ostringstream stream;
      stream << rls << '.' << dot;
      SetStatus(false, stream.str());
      WSACleanup();
      return false;
   }

   //  To indicate that the network is available, generate a log without
   //  trying to modify the network alarm, which is currently off.
   //
   auto log = Log::Create(NetworkLogGroup, NetworkAvailable);
   if(log != nullptr) Log::Submit(log);
   return true;
}

//------------------------------------------------------------------------------

fn_name SysSocket_StopLayer = "SysSocket.StopLayer";

void SysSocket::StopLayer()
{
   Debug::ft(SysSocket_StopLayer);

   if(WSACleanup() != 0)
   {
      auto log = Log::Create(NetworkLogGroup, NetworkShutdownFailure);

      if(log != nullptr)
      {
         *log << Log::Tab << "errval=" << WSAGetLastError();
         Log::Submit(log);
      }
   }
}
}
#endif
