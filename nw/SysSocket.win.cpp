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
#include <sstream>
#include <winsock2.h>  // must precede windows.h
#include <windows.h>
#include "Debug.h"
#include "Log.h"
#include "NwTrace.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
const u_long IO_Blocking = 0;
const u_long IO_NonBlocking = 1;

//------------------------------------------------------------------------------

fn_name SysSocket_ctor2 = "SysSocket.ctor";

SysSocket::SysSocket(ipport_t port, IpProtocol proto,
   size_t rxSize, size_t txSize, AllocRc& rc) :
   socket_(INVALID_SOCKET),
   blocking_(true),
   disconnecting_(false),
   tracing_(false),
   error_(0)
{
   Debug::ft(SysSocket_ctor2);

   sockaddr_in addr;

   rc = AllocOk;

   switch(proto)
   {
   case IpUdp:
      socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      break;
   case IpTcp:
      socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      break;
   default:
      Debug::SwErr(SysSocket_ctor2, proto, 0);
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

   rc = SetBuffSizes(rxSize, txSize);
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

void SysSocket::Close()
{
   Debug::ft(SysSocket_Close);

   if(socket_ != INVALID_SOCKET)
   {
      TraceEvent(NwTrace::Close, disconnecting_);
      if(closesocket(socket_) == SOCKET_ERROR) SetError();
      socket_ = INVALID_SOCKET;
   }
}

//------------------------------------------------------------------------------

fn_name SysSocket_Disconnect = "SysSocket.Disconnect";

void SysSocket::Disconnect()
{
   Debug::ft(SysSocket_Disconnect);

   if(!disconnecting_ && (socket_ != INVALID_SOCKET))
   {
      TraceEvent(NwTrace::Disconnect, 0);
      if(shutdown(socket_, SD_SEND) == SOCKET_ERROR) SetError();
      disconnecting_ = true;
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
   Debug::ft(SysSocket_Invalidate);

   socket_= INVALID_SOCKET;
}

//------------------------------------------------------------------------------

fn_name SysSocket_IsValid = "SysSocket.IsValid";

bool SysSocket::IsValid() const
{
   Debug::ft(SysSocket_IsValid);

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

fn_name SysSocket_SetBuffSizes = "SysSocket.SetBuffSizes";

SysSocket::AllocRc SysSocket::SetBuffSizes(size_t rxSize, size_t txSize)
{
   Debug::ft(SysSocket_SetBuffSizes);

   auto rc = SysSocket::AllocOk;
   size_t max;
   int maxsize = sizeof(max);

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
      Debug::SwErr(SysSocket_SetBuffSizes, max, rxSize);

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
      Debug::SwErr(SysSocket_SetBuffSizes, max, txSize);
   return rc;
}

//------------------------------------------------------------------------------

fn_name SysSocket_SetError2 = "SysSocket.SetError";

word SysSocket::SetError()
{
   Debug::ft(SysSocket_SetError2);

   error_ = WSAGetLastError();
   return -1;
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
      auto log = Log::Create("SOCKETS STARTUP FAILURE");

      if(log != nullptr)
      {
         *log << "errval=" << err << CRLF;
         Log::Spool(log);
      }

      return false;
   }

   auto rls = HIBYTE(wsaData.wVersion);
   auto dot = LOBYTE(wsaData.wVersion);

   if((rls != 2) || (dot != 2))
   {
      auto log = Log::Create("SOCKETS STARTUP FAILURE");

      if(log != nullptr)
      {
         *log << "errval=" << rls << '.' << dot << CRLF;
         Log::Spool(log);
      }

      WSACleanup();
      return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name SysSocket_StopLayer = "SysSocket.StopLayer";

void SysSocket::StopLayer()
{
   Debug::ft(SysSocket_StopLayer);

   if(WSACleanup() != 0)
   {
      auto log = Log::Create("SOCKETS SHUTDOWN FAILURE");

      if(log != nullptr)
      {
         *log << "errval=" << WSAGetLastError() << CRLF;
         Log::Spool(log);
      }
   }
}
}
#endif
