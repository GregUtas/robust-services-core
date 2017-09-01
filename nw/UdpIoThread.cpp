//==============================================================================
//
//  UdpIoThread.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "UdpIoThread.h"
#include <sstream>
#include "Clock.h"
#include "Debug.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "Log.h"
#include "NwTrace.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "SysUdpSocket.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name UdpIoThread_ctor = "UdpIoThread.ctor";

UdpIoThread::UdpIoThread(Faction faction, ipport_t port,
   size_t rxSize, size_t txSize) : IoThread(faction, port, rxSize, txSize)
{
   Debug::ft(UdpIoThread_ctor);

   ipPort_ = Singleton< IpPortRegistry >::Instance()->GetPort(port_, IpUdp);

   if(ipPort_ == nullptr)
   {
      Debug::SwErr(UdpIoThread_ctor, port_, 0);
   }
}

//------------------------------------------------------------------------------

fn_name UdpIoThread_dtor = "UdpIoThread.dtor";

UdpIoThread::~UdpIoThread()
{
   Debug::ft(UdpIoThread_dtor);

   ReleaseResources();
}

//------------------------------------------------------------------------------

const char* UdpIoThread::AbbrName() const
{
   return "udpio";
}

//------------------------------------------------------------------------------

fn_name UdpIoThread_Cleanup = "UdpIoThread.Cleanup";

void UdpIoThread::Cleanup()
{
   Debug::ft(UdpIoThread_Cleanup);

   ReleaseResources();
   Thread::Cleanup();
}

//------------------------------------------------------------------------------

fn_name UdpIoThread_Enter = "UdpIoThread.Enter";

void UdpIoThread::Enter()
{
   Debug::ft(UdpIoThread_Enter);

   word rcvd = 0;

   //  Exit if an IP port is not assigned to this thread.
   //
   if(ipPort_ == nullptr)
   {
      OutputLog(0);
      return;
   }

   //  If a UDP socket is already assigned to our port, reuse it: this
   //  occurs when being reentered after a trap.  If no socket exists,
   //  create one, of the desired size, bound to our port.  Generate a
   //  log and exit if this fails.
   //
   auto socket = static_cast< SysUdpSocket* >(ipPort_->GetSocket());

   if(socket == nullptr)
   {
      auto rc = SysSocket::AllocOk;

      socket = new SysUdpSocket(port_, rxSize_, txSize_, rc);

      if(rc != SysSocket::AllocOk)
      {
         delete socket;
         OutputLog(0x100 + rc);
         return;
      }

      if(!ipPort_->SetSocket(socket))
      {
         delete socket;
         OutputLog(1);
         return;
      }
   }

   //  Make all messages look as if they arrived on our IP address and
   //  port, regardless of how they were actually addressed.
   //
   auto host = IpPortRegistry::HostAddress();
   rxAddr_ = SysIpL3Addr(host, port_, IpUdp, nullptr);

   //  Enter a loop that keeps waiting forever to receive the next message.
   //  Pause after receiving a threshhold number of messages in a row.
   //
   while(true)
   {
      //e An I/O thread should not allow its receive buffer to overflow.
      //  This conflicts with the need to yield to allow other work.  The
      //  work time per faction (as opposed to per thread) could help to
      //  resolve this, with I/O threads perhaps having their own faction.
      //  However, some ports are more important than others (e.g. remote
      //  operations messages should be dropped before payload messages).
      //
      ConditionalPause(95);

      if(socket->Empty())
      {
         ipPort_->RecvsInSequence(recvs_);
         socket->SetBlocking(true);

         EnterBlockingOperation(BlockedOnNetwork, UdpIoThread_Enter);
         {
            rcvd =
               socket->RecvFrom(buffer_, SysUdpSocket::MaxUdpSize(), txAddr_);
         }
         ExitBlockingOperation(UdpIoThread_Enter);

         socket->TracePeer(NwTrace::RecvFrom, port_, txAddr_, rcvd);
         recvs_ = 0;
      }
      else
      {
         socket->SetBlocking(false);
         rcvd = socket->RecvFrom(buffer_, SysUdpSocket::MaxUdpSize(), txAddr_);
         socket->TracePeer(NwTrace::RecvFrom, port_, txAddr_, rcvd);
      }

      ++recvs_;
      ticks0_ = Clock::TicksNow();

      if(rcvd < 0)
      {
         //s Handle Recvfrom() error.
         //  For now, take a short break and hope the problem goes away.
         //  WSAEWOULDBLOCK is a chronic occurrence on Windows, which is
         //  curious because our socket is non-blocking.
         //
         if(rcvd == -1)
         {
            auto log = Log::Create("UDP RECVFROM ERROR");

            if(log != nullptr)
            {
               *log << "port=" << port_;
               *log << " errval=" << socket->GetError() << CRLF;
               Log::Spool(log);
            }
         }

         Pause(20);
         recvs_ = 0;
         continue;
      }

      //  Pass the message to the input handler.
      //
      ipPort_->BytesRcvd(rcvd);
      InvokeHandler(*ipPort_, buffer_, rcvd);
   }
}

//------------------------------------------------------------------------------

fn_name UdpIoThread_OutputLog = "UdpIoThread.OutputLog";

void UdpIoThread::OutputLog(debug32_t errval) const
{
   Debug::ft(UdpIoThread_OutputLog);

   auto log = Log::Create("UDP I/O THREAD FAILURE");

   if(log != nullptr)
   {
      *log << "port=" << port_;
      *log << " errval=" << errval << CRLF;
      Log::Spool(log);
   }
}

//------------------------------------------------------------------------------

void UdpIoThread::Patch(sel_t selector, void* arguments)
{
   IoThread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name UdpIoThread_ReleaseResources = "UdpIoThread.ReleaseResources";

void UdpIoThread::ReleaseResources()
{
   Debug::ft(UdpIoThread_ReleaseResources);

   if(ipPort_ != nullptr)
   {
      auto socket = static_cast< SysUdpSocket* >(ipPort_->GetSocket());
      if(socket != nullptr) delete socket;
      ipPort_->SetSocket(nullptr);
   }
}

//------------------------------------------------------------------------------

fn_name UdpIoThread_Unblock = "UdpIoThread.Unblock";

void UdpIoThread::Unblock()
{
   Debug::ft(UdpIoThread_Unblock);

   //  Delete the thread's socket.  If it is blocked on Recvfrom, this should
   //  unblock it.
   //
   ReleaseResources();
}
}
