//==============================================================================
//
//  NwDaemons.cpp
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
#include "NwDaemons.h"
#include <ostream>
#include "DaemonRegistry.h"
#include "Debug.h"
#include "Formatters.h"
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
fixed_string TcpIoDaemonName = "tcp";

//------------------------------------------------------------------------------

fn_name TcpIoDaemon_ctor = "TcpIoDaemon.ctor";

TcpIoDaemon::TcpIoDaemon(const TcpIpService* service, ipport_t port) :
   Daemon(MakeName(port).c_str(), 1),
   service_(service),
   port_(port)
{
   Debug::ft(TcpIoDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name TcpIoDaemon_dtor = "TcpIoDaemon.dtor";

TcpIoDaemon::~TcpIoDaemon()
{
   Debug::ft(TcpIoDaemon_dtor);
}

//------------------------------------------------------------------------------

fn_name TcpIoDaemon_CreateThread = "TcpIoDaemon.CreateThread";

Thread* TcpIoDaemon::CreateThread()
{
   Debug::ft(TcpIoDaemon_CreateThread);

   return new TcpIoThread(this, service_, port_);
}

//------------------------------------------------------------------------------

void TcpIoDaemon::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Daemon::Display(stream, prefix, options);

   stream << prefix << "service : " << strObj(service_) << CRLF;
   stream << prefix << "port    : " << port_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name TcpIoDaemon_GetDaemon = "TcpIoDaemon.GetDaemon";

TcpIoDaemon* TcpIoDaemon::GetDaemon(const TcpIpService* service, ipport_t port)
{
   Debug::ft(TcpIoDaemon_GetDaemon);

   auto reg = Singleton< DaemonRegistry >::Instance();
   auto name = MakeName(port);
   auto daemon = static_cast< TcpIoDaemon* >(reg->FindDaemon(name.c_str()));

   if(daemon == nullptr)
   {
      daemon = new TcpIoDaemon(service, port);
   }

   return daemon;
}

//------------------------------------------------------------------------------

fn_name TcpIoDaemon_MakeName = "TcpIoDaemon.MakeName";

string TcpIoDaemon::MakeName(ipport_t port)
{
   Debug::ft(TcpIoDaemon_MakeName);

   //  A Daemon requires a unique name, so append the port number
   //  to the basic name.
   //
   string name(TcpIoDaemonName);
   name.push_back('_');
   name.append(std::to_string(port));
   return name;
}

//------------------------------------------------------------------------------

void TcpIoDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string UdpIoDaemonName = "udp";

//------------------------------------------------------------------------------

fn_name UdpIoDaemon_ctor = "UdpIoDaemon.ctor";

UdpIoDaemon::UdpIoDaemon(const UdpIpService* service, ipport_t port) :
   Daemon(MakeName(port).c_str(), 1),
   service_(service),
   port_(port)
{
   Debug::ft(UdpIoDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name UdpIoDaemon_dtor = "UdpIoDaemon.dtor";

UdpIoDaemon::~UdpIoDaemon()
{
   Debug::ft(UdpIoDaemon_dtor);
}

//------------------------------------------------------------------------------

fn_name UdpIoDaemon_CreateThread = "UdpIoDaemon.CreateThread";

Thread* UdpIoDaemon::CreateThread()
{
   Debug::ft(UdpIoDaemon_CreateThread);

   return new UdpIoThread(this, service_, port_);
}

//------------------------------------------------------------------------------

void UdpIoDaemon::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Daemon::Display(stream, prefix, options);

   stream << prefix << "service : " << strObj(service_) << CRLF;
   stream << prefix << "port    : " << port_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name UdpIoDaemon_GetDaemon = "UdpIoDaemon.GetDaemon";

UdpIoDaemon* UdpIoDaemon::GetDaemon(const UdpIpService* service, ipport_t port)
{
   Debug::ft(UdpIoDaemon_GetDaemon);

   auto reg = Singleton< DaemonRegistry >::Instance();
   auto name = MakeName(port);
   auto daemon = static_cast< UdpIoDaemon* >(reg->FindDaemon(name.c_str()));

   if(daemon == nullptr)
   {
      daemon = new UdpIoDaemon(service, port);
   }

   return daemon;
}

//------------------------------------------------------------------------------

fn_name UdpIoDaemon_MakeName = "UdpIoDaemon.MakeName";

string UdpIoDaemon::MakeName(ipport_t port)
{
   Debug::ft(UdpIoDaemon_MakeName);

   //  A Daemon requires a unique name, so append the port number
   //  to the basic name.
   //
   string name(UdpIoDaemonName);
   name.push_back('_');
   name.append(std::to_string(port));
   return name;
}

//------------------------------------------------------------------------------

void UdpIoDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}
}
