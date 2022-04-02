//==============================================================================
//
//  LocalAddrTest.cpp
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
#include "LocalAddrTest.h"
#include <ostream>
#include <string>
#include "CfgParmRegistry.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "IpServiceCfg.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
LocalAddrHandler::LocalAddrHandler(IpPort* port) : InputHandler(port)
{
   Debug::ft("LocalAddrHandler.ctor");
}

//------------------------------------------------------------------------------

LocalAddrHandler::~LocalAddrHandler()
{
   Debug::ftnt("LocalAddrHandler.dtor");
}

//------------------------------------------------------------------------------

void LocalAddrHandler::Patch(sel_t selector, void* arguments)
{
   InputHandler::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void LocalAddrHandler::ReceiveBuff
   (IpBufferPtr& buff, size_t size, Faction faction) const
{
   Debug::ft("LocalAddrHandler.ReceiveBuff");

   //  Record that the source address successfully received a message.
   //
   Singleton< IpPortRegistry >::Instance()->TestAdvance();
}

//==============================================================================

fixed_string LocalAddrUdpKey = "LocalTestUdp";
fixed_string LocalAddrUdpExpl = "Create UDP I/O thread for Local Address Test";

SendLocalIpService::SendLocalIpService()
{
   Debug::ft("SendLocalIpService.ctor");

   enabled_.reset
      (new IpServiceCfg(LocalAddrUdpKey, "F", LocalAddrUdpExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*enabled_);
}

//------------------------------------------------------------------------------

SendLocalIpService::~SendLocalIpService()
{
   Debug::ftnt("SendLocalIpService.dtor");
}

//------------------------------------------------------------------------------

InputHandler* SendLocalIpService::CreateHandler(IpPort* port) const
{
   Debug::ft("SendLocalIpService.CreateHandler");

   return new LocalAddrHandler(port);
}

//------------------------------------------------------------------------------

fixed_string LocalAddrsServiceStr = "Local Address Test/UDP";
fixed_string LocalAddrsServiceExpl = "Local Address Test Protocol";

CliText* SendLocalIpService::CreateText() const
{
   Debug::ft("SendLocalIpService.CreateText");

   return new CliText(LocalAddrsServiceStr, LocalAddrsServiceExpl);
}

//------------------------------------------------------------------------------

bool SendLocalIpService::Enabled() const
{
   return enabled_->CurrValue();
}

//------------------------------------------------------------------------------

void SendLocalIpService::Patch(sel_t selector, void* arguments)
{
   UdpIpService::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void SendLocalIpService::Shutdown(RestartLevel level)
{
   Debug::ft("SendLocalIpService.Shutdown");

   FunctionGuard guard(Guard_ImmUnprotect);
   Restart::Release(enabled_);

   IpService::Shutdown(level);
}

//------------------------------------------------------------------------------

void SendLocalIpService::Startup(RestartLevel level)
{
   Debug::ft("SendLocalIpService.Startup");

   if(enabled_ == nullptr)
   {
      FunctionGuard guard(Guard_ImmUnprotect);
      enabled_.reset
         (new IpServiceCfg(LocalAddrUdpKey, "F", LocalAddrUdpExpl, this));
      Singleton< CfgParmRegistry >::Instance()->BindParm(*enabled_);
   }

   IpService::Startup(level);
}

//==============================================================================

SendLocalThread::SendLocalThread() : Thread(MaintenanceFaction),
   retest_(false)
{
   Debug::ft("SendLocalThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

SendLocalThread::~SendLocalThread()
{
   Debug::ftnt("SendLocalThread.dtor");
}

//------------------------------------------------------------------------------

c_string SendLocalThread::AbbrName() const
{
   return "locsend";
}

//------------------------------------------------------------------------------

void SendLocalThread::Destroy()
{
   Debug::ft("SendLocalThread.Destroy");

   Singleton< SendLocalThread >::Destroy();
}

//------------------------------------------------------------------------------

void SendLocalThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "retest : " << retest_ << CRLF;
}

//------------------------------------------------------------------------------

void SendLocalThread::Enter()
{
   Debug::ft("SendLocalThread.Enter");

   //  Exit if the local address service is not enabled.
   //
   auto reg = Singleton< IpPortRegistry >::Instance();
   auto svc = Singleton< SendLocalIpService >::Instance();
   if(!svc->Enabled()) return;

   //  Inform the registry that the test is starting.  Wait briefly if the
   //  UDP I/O thread that will receive our message needs more time to bind
   //  a socket to our port.
   //
   auto port = svc->Port();
   auto ipPort = reg->GetPort(port, IpUdp);
   if(ipPort == nullptr) return;

   reg->TestBegin();

   for(auto i = 4; i > 0; --i)
   {
      if(ipPort->GetSocket() != nullptr) break;
      Pause(250 * ONE_mSEC);
   }

   if(ipPort->GetSocket() != nullptr)
   {
      reg->TestAdvance();

      //  Send a message to the UDP I/O thread.  On success, give the
      //  UDP I/O thread time to receive the message.
      //
      SysIpL3Addr addr(IpPortRegistry::LocalAddr(), port);
      IpBufferPtr buff(new IpBuffer(MsgOutgoing, 0, sizeof(SysIpL3Addr)));
      auto payload = reinterpret_cast< SysIpL3Addr* >(buff->PayloadPtr());

      buff->SetTxAddr(addr);
      buff->SetRxAddr(addr);
      *payload = addr;

      if(buff->Send(true))
      {
         reg->TestAdvance();
         Pause(2 * ONE_SEC);
      }
   }

   //  The test has ended.  If it was initiated from the CLI, inform the
   //  CLI thread that the test has been completed.
   //
   reg->TestEnd();

   if(retest_)
   {
      Singleton< CliThread >::Instance()->Interrupt();
   }
}

//------------------------------------------------------------------------------

void SendLocalThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void SendLocalThread::Retest()
{
   Debug::ft("SendLocalThread.Retest");

   retest_ = true;
   Interrupt();
}

//==============================================================================

LocalAddrRetest::LocalAddrRetest(secs_t timeout) :
   Deferred(*Singleton< IpPortRegistry >::Instance(), timeout, false)
{
   Debug::ft("LocalAddrRetest.ctor");
}

//------------------------------------------------------------------------------

LocalAddrRetest::~LocalAddrRetest()
{
   Debug::ftnt("LocalAddrRetest.dtor");
}

//------------------------------------------------------------------------------

void LocalAddrRetest::EventHasOccurred(Event event)
{
   Debug::ft("LocalAddrRetest.EventHasOccurred");

   //  SendLocalThread currently exits when it completes its test.  But if
   //  its design changes to just sleep until the next test, it would need
   //  to be awoken instead.
   //
   auto thread = Singleton< SendLocalThread >::Extant();

   if(thread == nullptr)
      Singleton< SendLocalThread >::Instance();
   else
      thread->Interrupt();
}

//------------------------------------------------------------------------------

void LocalAddrRetest::Patch(sel_t selector, void* arguments)
{
   Deferred::Patch(selector, arguments);
}
}
