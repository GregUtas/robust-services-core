//==============================================================================
//
//  NwTypes.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NWTYPES_H_INCLUDED
#define NWTYPES_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <memory>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  An IP port number.  Interally, an IP port should be stored in host order.
//
typedef uint16_t ipport_t;

//  IP port constants.
//
enum IpPortIds
{
   NilIpPort = 0,
   FirstAppIpPort = 1024,
   CipIpPort = 40000,
   PotsShelfIpPort = 40001,
   PotsCallIpPort = 40002,
   MaxIpPort = UINT16_MAX,
   LastAppIpPort = MaxIpPort
};

//  An IPv4 address.  Interally, an IP address should be stored in host order.
//
typedef uint32_t ipv4addr_t;

//  IP protocols.
//
enum IpProtocol
{
   IpAny,        // wildcard
   IpUdp,
   IpTcp,
   IpProtocol_N  // numer of IP protocols
};

//  Inserts a string for PROTO into STREAM.
//
std::ostream& operator<<(std::ostream& stream, IpProtocol proto);

//  Forward declarations.
//
class InputHandler;
class IoThread;
class IpBuffer;
class IpPort;
class IpPortCfgParm;
class IpService;
class SysIpL3Addr;
class SysSocket;
class SysTcpSocket;

typedef std::unique_ptr< IpBuffer > IpBufferPtr;
typedef std::unique_ptr< IpPortCfgParm > IpPortCfgParmPtr;
typedef std::unique_ptr< SysTcpSocket > SysTcpSocketPtr;
}
#endif