//==============================================================================
//
//  NwCliParms.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include <string>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "NwTypes.h"
#include "SysIpL3Addr.h"

using namespace NodeBase;
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

HostNameMandParm::HostNameMandParm() :
   CliTextParm(HostNameMandExpl, false, 0) { }

//------------------------------------------------------------------------------

class IpAddrTextParm : public CliTextParm
{
public: IpAddrTextParm();
};

fixed_string IpAddrTextParmExpl = "IP address and optional port: n.n.n.n[:p]";

IpAddrTextParm::IpAddrTextParm() : CliTextParm(IpAddrTextParmExpl, false, 0) { }

IpAddrParm::IpAddrParm(c_string help, c_string text) :
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

bool GetIpL3Addr(SysIpL3Addr& input, const CliCommand& comm, CliThread& cli)
{
   Debug::ft("NetworkBase.GetIpL3Addr");

   string s;

   //  Set INPUT from the string for a layer 3 or layer 2 address.
   //
   if(!comm.GetString(s, cli)) return false;
   if(!cli.EndOfInput()) return false;
   input = SysIpL3Addr(s);
   return input.IsValid();
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
   CliTextParm(ServiceNameOptExpl, true, 0) { }
}
