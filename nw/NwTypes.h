//==============================================================================
//
//  NwTypes.h
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
#ifndef NWTYPES_H_INCLUDED
#define NWTYPES_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <memory>

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  An IP port number.  Interally, an IP port should be stored in host order.
//
typedef uint16_t ipport_t;

//  IP port constants.
//
constexpr ipport_t NilIpPort = 0;
constexpr ipport_t FirstAppIpPort = 1024;
constexpr ipport_t CipIpPort = 40000;
constexpr ipport_t PotsShelfIpPort = 40001;
constexpr ipport_t PotsCallIpPort = 40002;
constexpr ipport_t MaxIpPort = UINT16_MAX;
constexpr ipport_t LastAppIpPort = MaxIpPort;

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
