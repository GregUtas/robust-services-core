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
#include "Debug.h"
#include "Memory.h"

using std::ostream;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name NetworkBase_HostToNetwork = "NetworkBase.HostToNetwork";

void HostToNetwork(byte_t* start, size_t size, uint8_t word)
{
   Debug::ft(NetworkBase_HostToNetwork);

   size_t odd = 0;

   switch(word)
   {
   case 0:
   case 1:
      //
      //  No conversion is required.
      //
      return;

   case 2:
   {
      odd = (size & 0x01);
      uint16_t* data = reinterpret_cast< uint16_t* >(start);

      for(size_t i = 0; i < (size >> 1); ++i)
      {
         data[i] = ::htons(data[i]);
      }

      break;
   }

   case 4:
   {
      odd = (size & 0x03);
      uint32_t* data = reinterpret_cast< uint32_t* >(start);

      for(size_t i = 0; i < (size >> 2); ++i)
      {
         data[i] = ::htonl(data[i]);
      }

      break;
   }

   case 8:
   {
      odd = (size & 0x07);
      uint64_t* data = reinterpret_cast< uint64_t* >(start);

      for(size_t i = 0; i < (size >> 3); ++i)
      {
         data[i] = ::htonll(data[i]);
      }

      break;
   }

   default:
      Debug::SwLog(NetworkBase_HostToNetwork, word, 0);
   }

   if(odd != 0) Debug::SwLog(NetworkBase_HostToNetwork, word, odd);
}

//------------------------------------------------------------------------------

uint32_t htonl(uint32_t hostlong) { return ::htonl(hostlong); }

uint64_t htonll(uint64_t hostllong) { return ::htonll(hostllong); }

uint16_t htons(uint16_t hostshort) { return ::htons(hostshort); }

//------------------------------------------------------------------------------

fn_name NetworkBase_NetworkToHost = "NetworkBase.NetworkToHost";

void NetworkToHost(byte_t* dest, const byte_t* src, size_t size, uint8_t word)
{
   Debug::ft(NetworkBase_NetworkToHost);

   size_t odd = 0;

   switch(word)
   {
   case 0:
   case 1:
      Memory::Copy(dest, src, size);
      break;

   case 2:
   {
      odd = (size & 0x01);
      const uint16_t* from = reinterpret_cast< const uint16_t* >(src);
      uint16_t* to = reinterpret_cast< uint16_t* >(dest);

      for(size_t i = 0; i < (size >> 1); ++i)
      {
         to[i] = ::ntohs(from[i]);
      }

      break;
   }

   case 4:
   {
      odd = (size & 0x03);
      const uint32_t* from = reinterpret_cast< const uint32_t* >(src);
      uint32_t* to = reinterpret_cast< uint32_t* >(dest);

      for(size_t i = 0; i < (size >> 2); ++i)
      {
         to[i] = ::ntohs(from[i]);
      }

      break;
   }

   case 8:
   {
      odd = (size & 0x07);
      const uint64_t* from = reinterpret_cast< const uint64_t* >(src);
      uint64_t* to = reinterpret_cast< uint64_t* >(dest);

      for(size_t i = 0; i < (size >> 3); ++i)
      {
         to[i] = ::ntohs(from[i]);
      }

      break;
   }

   default:
      Debug::SwLog(NetworkBase_NetworkToHost, word, 0);
   }

   if(odd != 0) Debug::SwLog(NetworkBase_NetworkToHost, word, odd);
}

//------------------------------------------------------------------------------

uint32_t ntohl(uint32_t netlong) { return ::ntohl(netlong); }

uint64_t ntohll(uint32_t netllong) { return ::ntohll(netllong); }

uint16_t ntohs(uint16_t netshort) { return ::ntohs(netshort); }

//------------------------------------------------------------------------------

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
