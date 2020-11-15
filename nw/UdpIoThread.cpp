//==============================================================================
//
//  UdpIoThread.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "UdpIoThread.h"
#include <sstream>
#include <string>
#include "Alarm.h"
#include "Debug.h"
#include "Duration.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "Log.h"
#include "NbTypes.h"
#include "NwLogs.h"
#include "NwTrace.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "SysUdpSocket.h"
#include "TimePoint.h"
#include "UdpIpService.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name UdpIoThread_ctor = "UdpIoThread.ctor";

UdpIoThread::UdpIoThread(Daemon* daemon,
   const UdpIpService* service, ipport_t port) : IoThread(daemon, service, port)
{
   Debug::ft(UdpIoThread_ctor);

   ipPort_ = Singleton< IpPortRegistry >::Instance()->GetPort(port_, IpUdp);

   if(ipPort_ != nullptr)
      ipPort_->SetThread(this);
   else
      Debug::SwLog(UdpIoThread_ctor, "port not configured", port_);

   SetInitialized();
}

//------------------------------------------------------------------------------

UdpIoThread::~UdpIoThread()
{
   Debug::ftnt("UdpIoThread.dtor");

   ReleaseResources();
}

//------------------------------------------------------------------------------

c_string UdpIoThread::AbbrName() const
{
   return "udpio";
}

//------------------------------------------------------------------------------

void UdpIoThread::ClearAlarm() const
{
   Debug::ft("UdpIoThread.ClearAlarm");

   auto alarm = ipPort_->GetAlarm();
   if(alarm == nullptr) return;

   auto log = alarm->Create(NetworkLogGroup, NetworkServiceAvailable, NoAlarm);
   if(log == nullptr) return;

   *log << Log::Tab << "UDP: port=" << port_;
   Log::Submit(log);
}

//------------------------------------------------------------------------------

fn_name UdpIoThread_Enter = "UdpIoThread.Enter";

void UdpIoThread::Enter()
{
   Debug::ft(UdpIoThread_Enter);

   word rcvd = 0;

   //  Exit if an IP port is not assigned to this thread.
   //
   if(ipPort_ == nullptr) return;

   //  If a UDP socket is already assigned to our port, reuse it: this
   //  occurs when being reentered after a trap.  If no socket exists,
   //  create one, of the desired size, bound to our port.  Generate a
   //  log and exit if this fails.
   //
   auto socket = static_cast< SysUdpSocket* >(ipPort_->GetSocket());

   if(socket == nullptr)
   {
      auto svc = static_cast< const UdpIpService* >(ipPort_->GetService());
      auto rc = SysSocket::AllocOk;

      socket = new SysUdpSocket(port_, svc, rc);

      if(rc != SysSocket::AllocOk)
      {
         delete socket;
         RaiseAlarm(0x100 + rc);
         return;
      }

      if(!ipPort_->SetSocket(socket))
      {
         delete socket;
         RaiseAlarm(1);
         return;
      }
   }

   //  Make all messages look as if they arrived on our IP address and
   //  port, regardless of how they were actually addressed.  Clear any
   //  alarm that indicates our service is unavailable.
   //
   auto host = IpPortRegistry::HostAddress();
   rxAddr_ = SysIpL3Addr(host, port_, IpUdp, nullptr);
   ClearAlarm();

   //  Enter a loop that keeps waiting forever to receive the next message.
   //  Pause after receiving a threshold number of messages in a row.
   //
   while(true)
   {
      //  An I/O thread should not allow its receive buffer to overflow.
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
      time_ = TimePoint::Now();

      if(rcvd < 0)
      {
         //s Handle Recvfrom() error.
         //  For now, take a short break and hope the problem goes away.
         //  WSAEWOULDBLOCK is a chronic occurrence on Windows, which is
         //  curious because our socket is non-blocking.
         //
         if(rcvd == -1)
         {
            auto log = Log::Create(NetworkLogGroup, NetworkSocketError);

            if(log != nullptr)
            {
               *log << Log::Tab << "RecvFrom: port=" << port_;
               *log << " errval=" << socket->GetError();
               Log::Submit(log);
            }
         }

         Pause(Duration(20, mSECS));
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

void UdpIoThread::Patch(sel_t selector, void* arguments)
{
   IoThread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void UdpIoThread::RaiseAlarm(debug64_t errval) const
{
   Debug::ft("UdpIoThread.RaiseAlarm");

   auto alarm = ipPort_->GetAlarm();
   if(alarm == nullptr) return;

   auto log = alarm->Create(NetworkLogGroup, NetworkServiceFailure, MajorAlarm);
   if(log == nullptr) return;

   *log << Log::Tab << "UDP: port=" << port_ << " errval=" << errval;
   Log::Submit(log);
}

//------------------------------------------------------------------------------

void UdpIoThread::ReleaseResources()
{
   Debug::ft("UdpIoThread.ReleaseResources");

   if(ipPort_ != nullptr)
   {
      auto socket = static_cast< SysUdpSocket* >(ipPort_->GetSocket());
      delete socket;
      ipPort_->SetSocket(nullptr);
   }
}

//------------------------------------------------------------------------------

void UdpIoThread::Unblock()
{
   Debug::ft("UdpIoThread.Unblock");

   //  Delete the thread's socket.  If it is blocked on Recvfrom, this should
   //  unblock it.
   //
   ReleaseResources();
}
}
