//==============================================================================
//
//  NwDaemons.cpp
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
#include "NwDaemons.h"
#include "Deferred.h"
#include <ostream>
#include <string>
#include "DaemonRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "IpPortRegistry.h"
#include "Singleton.h"
#include "TcpIoThread.h"
#include "TcpIpService.h"
#include "UdpIoThread.h"
#include "UdpIpService.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
class IoThreadRecreator : public Deferred
{
public:
   //  Recreates an I/O thread for SERVICE and PORT, after TIMEOUT seconds,
   //  on behalf of DAEMON.
   //
   IoThreadRecreator(IoDaemon* daemon,
      const IpService* service, ipport_t port, secs_t timeout);

   //  Not subclassed.
   //
   ~IoThreadRecreator();

   //  Overridden to display member variables.
   //
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates the I/O thread.
   //
   void EventHasOccurred(Event event) override;

   //  The daemon for the I/O thread.
   //
   IoDaemon* const daemon_;

   //  The service for the I/O thread.
   //
   const IpService* const service_;

   //  The port for the I/O thread.
   //
   const ipport_t port_;
};

//------------------------------------------------------------------------------

IoThreadRecreator::IoThreadRecreator(IoDaemon* daemon,
   const IpService* service, ipport_t port, secs_t timeout) :
   Deferred(*Singleton< IpPortRegistry >::Instance(), timeout, false),
   daemon_(daemon),
   service_(service),
   port_(port)
{
   Debug::ft("IoThreadRecreator.ctor");
}

//------------------------------------------------------------------------------

IoThreadRecreator::~IoThreadRecreator()
{
   Debug::ftnt("IoThreadRecreator.dtor");

   daemon_->RecreatorDeleted();
}

//------------------------------------------------------------------------------

void IoThreadRecreator::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Deferred::Display(stream, prefix, options);

   stream << prefix << "daemon  : " << strObj(daemon_) << CRLF;
   stream << prefix << "service : " << strObj(service_) << CRLF;
   stream << prefix << "port    : " << port_ << CRLF;
}

//------------------------------------------------------------------------------

void IoThreadRecreator::EventHasOccurred(Event event)
{
   Debug::ft("IoThreadRecreator.EventHasOccurred");

   daemon_->CreateIoThread(service_, port_);
}

//------------------------------------------------------------------------------

void IoThreadRecreator::Patch(sel_t selector, void* arguments)
{
   Deferred::Patch(selector, arguments);
}

//==============================================================================

IoDaemon::IoDaemon(c_string name, const IpService* service, ipport_t port) :
   Daemon(name, 1, true),
   service_(service),
   port_(port),
   lastCreation_(TimePoint::Now()),
   backoffSecs_(0),
   recreator_(nullptr)
{
   Debug::ft("IoDaemon.ctor");
}

//------------------------------------------------------------------------------

fn_name IoDaemon_CreateIoThread = "IoDaemon.CreateIoThread";

Thread* IoDaemon::CreateIoThread(const IpService* service, ipport_t port)
{
   Debug::ft(IoDaemon_CreateIoThread);

   Debug::SwLog(IoDaemon_CreateIoThread, strOver(this), service->Protocol());
   return nullptr;
}

//------------------------------------------------------------------------------

constexpr secs_t BackOffSecs = 4;

const Duration MinExitTime = Duration(4, SECS);

Thread* IoDaemon::CreateThread()
{
   Debug::ft("IoDaemon.CreateThread");

   //  If the thread was last created more than 4 seconds ago, create it
   //  immediately.  Reset the backoff time to 4 seconds so that we will
   //  wait to recreate it if it exits quickly.
   //
   auto now = TimePoint::Now();

   if((now - lastCreation_) > MinExitTime)
   {
      backoffSecs_ = BackOffSecs;
      lastCreation_ = now;
      return CreateIoThread(service_, port_);
   }

   //  The last thread exited quickly, so unless a work item already exists
   //  as the result of a previous thread also having exited, queue one to
   //  recreate the thread after the backoff time.  If that thread also exits
   //  quickly, the backoff time will be doubled (but capped at 64 seconds).
   //
   if(recreator_ == nullptr)
   {
      recreator_ = new IoThreadRecreator(this, service_, port_, backoffSecs_);
      if(backoffSecs_ < 64) backoffSecs_ <<= 1;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void IoDaemon::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Daemon::Display(stream, prefix, options);

   stream << prefix << "service      : " << strObj(service_) << CRLF;
   stream << prefix << "port         : " << port_ << CRLF;
   stream << prefix << "lastCreation : " << lastCreation_.to_str() << CRLF;
   stream << prefix << "backoffSecs  : " << backoffSecs_ << CRLF;
}

//------------------------------------------------------------------------------

void IoDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void IoDaemon::RecreatorDeleted()
{
   Debug::ft("IoDaemon.RecreatorDeleted");

   recreator_ = nullptr;
}

//==============================================================================

fixed_string TcpIoDaemonName = "tcp";

//------------------------------------------------------------------------------
//
//  Returns the name for the daemon that manages the TCP I/O thread on PORT.
//
static string MakeTcpName(ipport_t port)
{
   Debug::ft("NetworkBase.MakeTcpName");

   //  A Daemon requires a unique name, so append the port number
   //  to the basic name.
   //
   string name(TcpIoDaemonName);
   name.push_back('_');
   name.append(std::to_string(port));
   return name;
}

//------------------------------------------------------------------------------

TcpIoDaemon::TcpIoDaemon(const TcpIpService* service, ipport_t port) :
   IoDaemon(MakeTcpName(port).c_str(), service, port)
{
   Debug::ft("TcpIoDaemon.ctor");
}

//------------------------------------------------------------------------------

Thread* TcpIoDaemon::CreateIoThread(const IpService* service, ipport_t port)
{
   Debug::ft("TcpIoDaemon.CreateIoThread");

   return new TcpIoThread
      (this, static_cast< const TcpIpService* >(service), port);
}

//------------------------------------------------------------------------------

TcpIoDaemon* TcpIoDaemon::GetDaemon(const TcpIpService* service, ipport_t port)
{
   Debug::ft("TcpIoDaemon.GetDaemon");

   auto reg = Singleton< DaemonRegistry >::Instance();
   auto name = MakeTcpName(port);
   auto daemon = static_cast< TcpIoDaemon* >(reg->FindDaemon(name.c_str()));

   if(daemon != nullptr) return daemon;
   return new TcpIoDaemon(service, port);
}

//------------------------------------------------------------------------------

void TcpIoDaemon::Patch(sel_t selector, void* arguments)
{
   IoDaemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string UdpIoDaemonName = "udp";

//------------------------------------------------------------------------------
//
//  Returns the name for the daemon that manages the UDP I/O thread on PORT.
//
static string MakeUdpName(ipport_t port)
{
   Debug::ft("NetworkBase.MakeUdpName");

   //  A Daemon requires a unique name, so append the port number
   //  to the basic name.
   //
   string name(UdpIoDaemonName);
   name.push_back('_');
   name.append(std::to_string(port));
   return name;
}

//------------------------------------------------------------------------------

UdpIoDaemon::UdpIoDaemon(const UdpIpService* service, ipport_t port) :
   IoDaemon(MakeUdpName(port).c_str(), service, port)
{
   Debug::ft("UdpIoDaemon.ctor");
}

//------------------------------------------------------------------------------

Thread* UdpIoDaemon::CreateIoThread(const IpService* service, ipport_t port)
{
   Debug::ft("UdpIoDaemon.CreateIoThread");

   return new UdpIoThread
      (this, static_cast<const UdpIpService*>(service), port);
}

//------------------------------------------------------------------------------

UdpIoDaemon* UdpIoDaemon::GetDaemon(const UdpIpService* service, ipport_t port)
{
   Debug::ft("UdpIoDaemon.GetDaemon");

   auto reg = Singleton< DaemonRegistry >::Instance();
   auto name = MakeUdpName(port);
   auto daemon = static_cast< UdpIoDaemon* >(reg->FindDaemon(name.c_str()));

   if(daemon != nullptr) return daemon;
   return new UdpIoDaemon(service, port);
}

//------------------------------------------------------------------------------

void UdpIoDaemon::Patch(sel_t selector, void* arguments)
{
   IoDaemon::Patch(selector, arguments);
}
}
