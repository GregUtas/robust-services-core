//==============================================================================
//
//  TcpIpPort.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

namespace NodeBase
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