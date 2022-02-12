//==============================================================================
//
//  NwTypes.h
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
#ifndef NWTYPES_H_INCLUDED
#define NWTYPES_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <memory>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  For distinguishing IPv4 and IPv6 addresses.
//
enum IpAddrFamily
{
   IPv4,
   IPv6
};

//  An IPv4 address.  Internally, it is stored in host order.
//
typedef unsigned long IPv4Addr;

//  An IPv6 address.  Internally, it is stored in host order and is also
//  used for all IPv4 addresses.  [0] is the most significant field.  If
//  SysIpL2Addr::SupportsIPv6 returns false, the non-IPv4 portion of the
//  address the address is zeroed.  If it returns true, an IPv4 address
//  is stored so that it is mapped to IPv6.
//
//  NOTE: The scope identifier (used in link-local addresses)
//  ====  is not supported.  Only IPv6 addresses with a zero
//        scope identifier can be used.
//
struct IPv6Addr
{
   union
   {
      uint8_t  u8[16];  // bytes: [12-15] overlay IPv4Addr
      uint16_t u16[8];  // quartets: usual format for IPv6
      uint32_t u32[4];  // [3] overlays IPv4Addr
   };

   //  Constructs the null address (all zeroes).
   //
   IPv6Addr();

   //  Returns true if this address is identical to THAT.
   //
   bool operator==(const IPv6Addr& that) const;

   //  Returns true if this address is different than THAT.
   //
   bool operator!=(const IPv6Addr& that) const;

   //  Sets the fields that map an IPv4 address to IPv6.
   //
   void SetAsMappedIPv4Addr();
};

//  The index into IPv6Addr.u32 for an entire IPv4 address.
//
constexpr int U32_IPv4_IDX = 3;

//  The indices into IPv6Addr.u8 for the bytes in an IPv4 address (A.B.C.D).
//
constexpr int U8_IPv4_A_IDX = 15;
constexpr int U8_IPv4_B_IDX = 14;
constexpr int U8_IPv4_C_IDX = 13;
constexpr int U8_IPv4_D_IDX = 12;

//  The quartet used at IPv6Addr.u16[5] to map an IPv4 address to IPv6.
//
constexpr uint16_t MappedIPv4Quartet = 0xffff;

//  The index into IPv6Addr.u16 for MappedIPv4Quartet.
//
constexpr int U16_MAPPED_IPv4_IDX = 5;

//  An IP port number.  Internally, it is stored in host order.
//
typedef uint16_t ipport_t;

//  IP port constants.
//
constexpr ipport_t NilIpPort = 0;
constexpr ipport_t FirstAppIpPort = 1024;
constexpr ipport_t LocalAddrTestIpPort = 30000;
constexpr ipport_t CipIpPort = 40000;
constexpr ipport_t PotsShelfIpPort = 40001;
constexpr ipport_t PotsCallIpPort = 40002;
constexpr ipport_t MaxIpPort = UINT16_MAX;
constexpr ipport_t LastAppIpPort = MaxIpPort;

//  IP protocols.
//
enum IpProtocol
{
   IpAny,        // wildcard
   IpUdp,
   IpTcp,
   IpProtocol_N  // number of IP protocols
};

//  Inserts a string for PROTO into STREAM.
//
std::ostream& operator<<(std::ostream& stream, IpProtocol proto);

//  The state of an IP address associated with this element.
//
enum IpAddrState
{
   Unverified,    // address has not yet been tested
   BindFailed,    // socket could not be bound to address
   SendFailed,    // address failed to send a test message
   RecvFailed,    // address failed to receive a test message
   Verified,      // socket bound; message sent and received
   IpAddrState_N  // number of states
};

//  Inserts a string for STATE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, IpAddrState state);

//  For reporting errors in network functions.
//
typedef NodeBase::word nwerr_t;

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
