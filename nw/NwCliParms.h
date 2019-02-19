//==============================================================================
//
//  NwCliParms.h
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
#ifndef NWCLIPARMS_H_INCLUDED
#define NWCLIPARMS_H_INCLUDED

#include "CliIntParm.h"
#include "CliText.h"
#include "CliTextParm.h"
#include "SysTypes.h"

using namespace NodeBase;

namespace NodeBase
{
   class CliCommand;
}

namespace NetworkBase
{
   class SysIpL3Addr;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Strings used by commands in the Network increment.
//
extern fixed_string NoHostAddrExpl;
extern fixed_string NoHostInfoExpl;
extern fixed_string NoHostNameExpl;
extern fixed_string NoIpPortExpl;

//------------------------------------------------------------------------------
//
//  Parameter for a string that specifies a host name.
//
class HostNameMandParm : public CliTextParm
{
public: HostNameMandParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an IP address and optional port number.  Must be subclassed
//  to provide HELP and TEXT.
//
class IpAddrParm : public CliText
{
public:
   virtual ~IpAddrParm() = default;
protected:
   IpAddrParm(const char* help, const char* text);
};

//------------------------------------------------------------------------------
//
//  Parameters for an IP port number.
//
class IpPortMandParm : public CliIntParm
{
public: IpPortMandParm();
};

class IpPortOptParm : public CliIntParm
{
public: IpPortOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an IP port's service name.
//
class ServiceNameOptParm : public CliTextParm
{
public: ServiceNameOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters that support trace tools.
//
class PeerText : public IpAddrParm
{
public: PeerText();
};

class PeersText : public CliText
{
public: PeersText();
};

class PortText : public CliText
{
public: PortText();
};

class PortsText : public CliText
{
public: PortsText();
};

//------------------------------------------------------------------------------
//
//  Function for obtaining a SysIpL3Addr.
//
bool GetIpL3Addr(SysIpL3Addr& input, const CliCommand& comm, CliThread& cli);
}
#endif
