//==============================================================================
//
//  TcpIpPort.cpp
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
#include "TcpIpPort.h"
#include <sstream>
#include "Debug.h"
#include "Log.h"
#include "SysTcpSocket.h"
#include "SysTypes.h"
#include "TcpIoThread.h"
#include "TcpIpService.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name TcpIpPort_ctor = "TcpIpPort.ctor";

TcpIpPort::TcpIpPort(ipport_t port, IpService* service) : IpPort(port, service)
{
   Debug::ft(TcpIpPort_ctor);
}

//------------------------------------------------------------------------------

fn_name TcpIpPort_dtor = "TcpIpPort.dtor";

TcpIpPort::~TcpIpPort()
{
   Debug::ft(TcpIpPort_dtor);
}

//------------------------------------------------------------------------------

fn_name TcpIpPort_CreateAppSocket = "TcpIpPort.CreateAppSocket";

SysSocket* TcpIpPort::CreateAppSocket(size_t rxSize, size_t txSize)
{
   Debug::ft(TcpIpPort_CreateAppSocket);

   auto rc = SysSocket::AllocFailed;

   //  If there is no I/O thread running on this port, create it after
   //  generating a log.
   //
   auto thread = GetThread();

   if(thread == nullptr)
   {
      Debug::SwErr(TcpIpPort_CreateAppSocket, 0, 0);
      thread = CreateIoThread();
      if(thread == nullptr) return nullptr;
   }

   //  Create the socket and register it with the I/O thread.
   //
   auto socket = SysTcpSocketPtr
      (new SysTcpSocket(NilIpPort, rxSize, txSize, rc));
   if(socket == nullptr) return nullptr;

   if(rc != SysSocket::AllocOk)
   {
      auto log = Log::Create("TCP SOCKET ALLOCATION");

      if(log != nullptr)
      {
         *log << "rc=" << rc << " errval=" << socket->GetError() << CRLF;
         Log::Spool(log);
      }

      return nullptr;
   }

   if(!thread->InsertSocket(socket.get())) return nullptr;
   rc = SysSocket::AllocOk;
   return socket.release();
}

//------------------------------------------------------------------------------

fn_name TcpIpPort_CreateIoThread = "TcpIpPort.CreateIoThread";

IoThread* TcpIpPort::CreateIoThread()
{
   Debug::ft(TcpIpPort_CreateIoThread);

   auto svc = static_cast< TcpIpService* >(GetService());

   return new TcpIoThread(svc->GetFaction(), GetPort(),
      svc->RxSize(), svc->TxSize(), svc->MaxConns());
}

//------------------------------------------------------------------------------

void TcpIpPort::Patch(sel_t selector, void* arguments)
{
   IpPort::Patch(selector, arguments);
}
}
