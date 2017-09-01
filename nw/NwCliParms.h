//==============================================================================
//
//  NwCliParms.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   class SysIpL3Addr;
}

//------------------------------------------------------------------------------

namespace NodeBase
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
   virtual ~IpAddrParm() { }
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
