//==============================================================================
//
//  NwCliParms.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef NWCLIPARMS_H_INCLUDED
#define NWCLIPARMS_H_INCLUDED

#include "CliIntParm.h"
#include "CliText.h"
#include "CliTextParm.h"
#include "SysTypes.h"

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
extern NodeBase::fixed_string NoHostAddrExpl;
extern NodeBase::fixed_string NoHostInfoExpl;
extern NodeBase::fixed_string NoHostNameExpl;
extern NodeBase::fixed_string NoIpPortExpl;
extern NodeBase::fixed_string NoIpServiceExpl;

//------------------------------------------------------------------------------
//
//  Parameter for a string that specifies a host name.
//
class HostNameMandParm : public NodeBase::CliTextParm
{
public: HostNameMandParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an IP address and optional port number.
//
class IpAddrParm : public NodeBase::CliText
{
public:
   IpAddrParm(NodeBase::c_string help, NodeBase::c_string text);
   virtual ~IpAddrParm() = default;
};

//------------------------------------------------------------------------------
//
//  Parameters for an IP port number.
//
class IpPortMandParm : public NodeBase::CliIntParm
{
public: IpPortMandParm();
};

class IpPortOptParm : public NodeBase::CliIntParm
{
public: IpPortOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an IP port's service name.
//
class ServiceNameOptParm : public NodeBase::CliTextParm
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

class PeersText : public NodeBase::CliText
{
public: PeersText();
};

class PortText : public NodeBase::CliText
{
public: PortText();
};

class PortsText : public NodeBase::CliText
{
public: PortsText();
};

//------------------------------------------------------------------------------
//
//  Function for obtaining a SysIpL3Addr.
//
bool GetIpL3Addr(SysIpL3Addr& input,
   const NodeBase::CliCommand& comm, NodeBase::CliThread& cli);
}
#endif
