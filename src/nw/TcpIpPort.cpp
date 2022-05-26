//==============================================================================
//
//  TcpIpPort.cpp
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
#include "TcpIpPort.h"
#include "Debug.h"
#include "NwDaemons.h"
#include "Restart.h"
#include "SysTcpSocket.h"
#include "SysTypes.h"
#include "TcpIoThread.h"
#include "TcpIpService.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
TcpIpPort::TcpIpPort(ipport_t port, const IpService* service) :
   IpPort(port, service)
{
   Debug::ft("TcpIpPort.ctor");
}

//------------------------------------------------------------------------------

TcpIpPort::~TcpIpPort()
{
   Debug::ftnt("TcpIpPort.dtor");
}

//------------------------------------------------------------------------------

fn_name TcpIpPort_CreateAppSocket = "TcpIpPort.CreateAppSocket";

SysTcpSocket* TcpIpPort::CreateAppSocket()
{
   Debug::ft(TcpIpPort_CreateAppSocket);

   //  If there is no I/O thread running on this port, create it after
   //  generating a log.
   //
   auto thread = static_cast<TcpIoThread*>(GetThread());

   if(thread == nullptr)
   {
      if(Restart::GetStage() == ShuttingDown) return nullptr;
      Debug::SwLog(TcpIpPort_CreateAppSocket, "I/O thread not found", 0);
      thread = static_cast<TcpIoThread*>(CreateThread());
      if(thread == nullptr) return nullptr;
   }

   //  Create the socket and register it with the I/O thread.
   //
   auto svc = static_cast<const TcpIpService*>(GetService());
   auto rc = SysSocket::AllocFailed;

   SysTcpSocketPtr socket(new SysTcpSocket(NilIpPort, svc, rc));

   if(rc != SysSocket::AllocOk)
   {
      return nullptr;
   }

   if(!thread->InsertSocket(socket.get())) return nullptr;
   return socket.release();
}

//------------------------------------------------------------------------------

IoThread* TcpIpPort::CreateIoThread()
{
   Debug::ft("TcpIpPort.CreateIoThread");

   auto svc = static_cast<const TcpIpService*>(GetService());
   auto daemon = TcpIoDaemon::GetDaemon(svc, GetPort());
   return new TcpIoThread(daemon, svc, GetPort());
}

//------------------------------------------------------------------------------

void TcpIpPort::Patch(sel_t selector, void* arguments)
{
   IpPort::Patch(selector, arguments);
}
}
