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
#include <string>
#include "Debug.h"
#include "Log.h"
#include "NwLogs.h"
#include "SysTcpSocket.h"
#include "SysTypes.h"
#include "TcpIoThread.h"
#include "TcpIpService.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name TcpIpPort_ctor = "TcpIpPort.ctor";

TcpIpPort::TcpIpPort(ipport_t port, const IpService* service) :
   IpPort(port, service)
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

SysTcpSocket* TcpIpPort::CreateAppSocket()
{
   Debug::ft(TcpIpPort_CreateAppSocket);

   auto rc = SysSocket::AllocFailed;

   //  If there is no I/O thread running on this port, create it after
   //  generating a log.
   //
   auto thread = static_cast< TcpIoThread* >(GetThread());

   if(thread == nullptr)
   {
      if(Restart::GetStatus() == ShuttingDown) return nullptr;
      Debug::SwLog(TcpIpPort_CreateAppSocket, "I/O thread not found", 0);
      thread = static_cast< TcpIoThread* >(CreateIoThread());
      if(thread == nullptr) return nullptr;
   }

   //  Create the socket and register it with the I/O thread.
   //
   auto svc = static_cast< const TcpIpService* >(GetService());
   SysTcpSocketPtr socket(new SysTcpSocket(NilIpPort, svc, rc));
   if(socket == nullptr) return nullptr;

   if(rc != SysSocket::AllocOk)
   {
      auto log = Log::Create(NetworkLogGroup, NetworkAllocFailure);

      if(log != nullptr)
      {
         *log << Log::Tab << "TCP socket: rc=" << rc;
         *log << " errval=" << socket->GetError();
         Log::Submit(log);
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

   auto svc = static_cast< const TcpIpService* >(GetService());
   return new TcpIoThread(svc, GetPort());
}

//------------------------------------------------------------------------------

void TcpIpPort::Patch(sel_t selector, void* arguments)
{
   IpPort::Patch(selector, arguments);
}
}
