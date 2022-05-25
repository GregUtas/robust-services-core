//==============================================================================
//
//  NwTypes.cpp
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
#include "NwTypes.h"
#include <ostream>
#include "Debug.h"
#include "SysIpL2Addr.h"

using std::ostream;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
IPv6Addr::IPv6Addr() : u32{0}
{
   Debug::ft("IPv6Addr.ctor");
}

//------------------------------------------------------------------------------

bool IPv6Addr::operator==(const IPv6Addr& that) const
{
   for(int i = 3; i >= 0; --i)
   {
      if(this->u32[i] != that.u32[i]) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

bool IPv6Addr::operator!=(const IPv6Addr& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

void IPv6Addr::SetAsMappedIPv4Addr()
{
   Debug::ft("IPv6Addr.SetAsMappedIPv4Addr");

   //  Don't map an IPv4 address if IPv6 is not supported.
   //
   if(!SysIpL2Addr::SupportsIPv6()) return;
   u16[U16_MAPPED_IPv4_IDX] = MappedIPv4Quartet;
   for(int i = U16_MAPPED_IPv4_IDX - 1; i >= 0; --i) u16[i] = 0;
}

//==============================================================================

fixed_string IpAddrStateStrings[IpAddrState_N + 1] =
{
   "unverified",
   "bind failed",
   "send failed",
   "recv failed",
   "verified",
   ERROR_STR
};

ostream& operator<<(ostream& stream, IpAddrState state)
{
   if((state >= 0) && (state < IpAddrState_N))
      stream << IpAddrStateStrings[state];
   else
      stream << IpAddrStateStrings[IpAddrState_N];
   return stream;
}

//==============================================================================

fixed_string ProtocolStrings[IpProtocol_N + 1] =
{
   "Any",
   "UDP",
   "TCP",
   ERROR_STR
};

ostream& operator<<(ostream& stream, IpProtocol proto)
{
   if(proto < IpProtocol_N)
      stream << ProtocolStrings[proto];
   else
      stream << ProtocolStrings[IpProtocol_N];
   return stream;
}
}
