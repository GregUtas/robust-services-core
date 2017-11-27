//==============================================================================
//
//  NwCliParms.cpp
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
#include "NwCliParms.h"
#include <iosfwd>
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "NwTypes.h"
#include "SysIpL3Addr.h"

using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fixed_string NoHostAddrExpl = "Host address not found.";
fixed_string NoHostInfoExpl = "Host information not found.";
fixed_string NoHostNameExpl = "Host name not found.";
fixed_string NoIpPortExpl   = "Nothing is registered against that IP port.";

//------------------------------------------------------------------------------

fixed_string HostNameMandExpl = "name of host";

HostNameMandParm::HostNameMandParm() : CliTextParm(HostNameMandExpl) { }

//------------------------------------------------------------------------------

class IpAddrTextParm : public CliTextParm
{
public: IpAddrTextParm();
};

fixed_string IpAddrTextParmExpl = "IP address and optional port: n.n.n.n[:p]";

IpAddrTextParm::IpAddrTextParm() : CliTextParm(IpAddrTextParmExpl) { }

IpAddrParm::IpAddrParm(const char* help, const char* text) :
   CliText(help, text)
{
   BindParm(*new IpAddrTextParm);
}

//------------------------------------------------------------------------------

fixed_string PeersTextStr = "peers";
fixed_string PeersTextExpl = "all included/excluded peers";

PeersText::PeersText() : CliText(PeersTextExpl, PeersTextStr) { }

//------------------------------------------------------------------------------

fixed_string PeerTextStr = "peer";
fixed_string PeerTextExpl = "messages to/from from a specific peer address/port";

PeerText::PeerText() : IpAddrParm(PeerTextExpl, PeerTextStr) { }

//------------------------------------------------------------------------------

fixed_string PortTextStr = "port";
fixed_string PortTextExpl = "messages received by a specific IP port";

PortText::PortText() : CliText(PortTextExpl, PortTextStr)
{
   BindParm(*new IpPortMandParm);
}

//------------------------------------------------------------------------------

fixed_string PortsTextStr = "ports";
fixed_string PortsTextExpl = "all included/excluded IP ports";

PortsText::PortsText() : CliText(PortsTextExpl, PortsTextStr) { }

//------------------------------------------------------------------------------

fn_name NetworkBase_GetIpL3Addr = "NetworkBase.GetIpL3Addr";

bool GetIpL3Addr(SysIpL3Addr& input, const CliCommand& comm, CliThread& cli)
{
   Debug::ft(NetworkBase_GetIpL3Addr);

   string s;
   char c;
   int n;
   ipv4addr_t addr = 0;
   ipport_t port = 0;

   //  Get the IP address string (n.n.n.n[:p]) and put it into a stream.
   //
   if(!comm.GetString(s, cli)) return false;
   cli.EndOfInput(false);

   std::istringstream stream(s);

   //  Extract the "n.n.n.n" portion.
   //
   for(auto i = 0; i <= 3; ++i)
   {
      stream >> n;
      if(!stream) return false;
      if((n < 0) || (n > 255)) return false;
      addr = (addr << 8) + n;

      if(i != 3)
      {
         stream >> c;
         if(!stream) return false;
         if(c != '.') return false;
      }
   }

   //  Extract the optional ":p" portion.
   //
   stream >> c;
   if(!stream)
   {
      input = SysIpL3Addr(addr, NilIpPort);
      return true;
   }

   if(c != ':') return false;

   stream >> n;
   if(!stream) return false;
   if((n < 0) || (n > 0xffff)) return false;

   input = SysIpL3Addr(addr, port);
   return true;
}

//------------------------------------------------------------------------------

fixed_string IpPortMandExpl = "ipport_t";

IpPortMandParm::IpPortMandParm() : CliIntParm(IpPortMandExpl, 0, MaxIpPort) { }

fixed_string IpPortOptExpl = "ipport_t";

IpPortOptParm::IpPortOptParm() :
   CliIntParm(IpPortOptExpl, 0, MaxIpPort, true) { }

//------------------------------------------------------------------------------

fixed_string ServiceNameOptExpl = "name of IP service (or port number)";

ServiceNameOptParm::ServiceNameOptParm() :
   CliTextParm(ServiceNameOptExpl, true) { }
}
