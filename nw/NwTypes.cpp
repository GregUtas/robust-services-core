//==============================================================================
//
//  NwTypes.cpp
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
#include "NwTypes.h"
#include <winsock2.h>
#include "SysTypes.h"

using std::ostream;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
uint32_t htonl(uint32_t hostlong) { return ::htonl(hostlong); }

uint64_t htonll(uint64_t hostllong) { return ::htonll(hostllong); }

uint16_t htons(uint16_t hostshort) { return ::htons(hostshort); }

uint32_t ntohl(uint32_t netlong) { return ::ntohl(netlong); }

uint64_t ntohll(uint32_t netllong) { return ::ntohll(netllong); }

uint16_t ntohs(uint16_t netshort) { return ::ntohs(netshort); }

fixed_string ProtocolStrings[IpProtocol_N + 1] =
{
   "Any",
   "UDP",
   "TCP",
   ERROR_STR
};

ostream& operator<<(ostream& stream, IpProtocol proto)
{
   if((proto >= 0) && (proto < IpProtocol_N))
      stream << ProtocolStrings[proto];
   else
      stream << ProtocolStrings[IpProtocol_N];
   return stream;
}
}