//==============================================================================
//
//  SysIpL3Addr.linux.cpp
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
#ifdef OS_LINUX

#include "SysIpL3Addr.h"
#include <cstddef>
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "Debug.h"
#include "FunctionGuard.h"
#include "NwLogs.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name SysIpL3Addr_ctor = "SysIpL3Addr.ctor(name, service)";

SysIpL3Addr::SysIpL3Addr(const string& name,
   const string& service, IpProtocol& proto) :
   port_(NilIpPort),
   proto_(IpAny),
   socket_(nullptr)
{
   Debug::ft(SysIpL3Addr_ctor);

   addrinfo hints;
   addrinfo* info = nullptr;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;

   FunctionGuard guard(Guard_MakePreemptable);

   auto err = getaddrinfo(name.c_str(), service.c_str(), &hints, &info);

   if(err == 0)
   {
      switch(info->ai_family)
      {
      case AF_INET:
      {
         auto netaddr = (sockaddr_in*) info->ai_addr;
         NetworkToHost(netaddr->sin_addr.s_addr, netaddr->sin_port);

         switch(info->ai_protocol)
         {
         case 0:
            break;
         case IPPROTO_UDP:
            proto_ = IpUdp;
            break;
         case IPPROTO_TCP:
            proto_ = IpTcp;
            break;
         default:
            Debug::SwLog(SysIpL3Addr_ctor,
               "unsupported protocol", info->ai_protocol);
         }

         break;
      }

      case AF_INET6:
      {
         auto netaddr = (sockaddr_in6*) info->ai_addr;
         NetworkToHost(netaddr->sin6_addr.s6_addr16, netaddr->sin6_port);

         switch(info->ai_protocol)
         {
         case 0:
            break;
         case IPPROTO_UDP:
            proto_ = IpUdp;
            break;
         case IPPROTO_TCP:
            proto_ = IpTcp;
            break;
         default:
            Debug::SwLog(SysIpL3Addr_ctor,
               "unsupported protocol", info->ai_protocol);
         }

         break;
      }

      default:
         Debug::SwLog(SysIpL3Addr_ctor,
            "unsupported protocol family", info->ai_family);
      }

      freeaddrinfo(info);
   }
   else
   {
      OutputNwLog(NetworkFunctionError, "getaddrinfo", err);
   }

   proto = proto_;
}

//------------------------------------------------------------------------------

bool SysIpL3Addr::AddrToName(string& name, string& service) const
{
   Debug::ft("SysIpL3Addr.AddrToName");

   sockaddr* addrinfo = nullptr;
   size_t addrsize = 0;
   sockaddr_in ipv4addr;
   sockaddr_in6 ipv6addr;

   if(Family() == IPv4)
   {
      ipv4addr.sin_family = AF_INET;
      HostToNetwork(ipv4addr.sin_addr.s_addr, ipv4addr.sin_port);
      addrinfo = (sockaddr*) &ipv4addr;
      addrsize = sizeof(ipv4addr);
   }
   else
   {
      ipv6addr.sin6_family = AF_INET6;
      HostToNetwork(ipv6addr.sin6_addr.s6_addr16, ipv6addr.sin6_port);
      ipv6addr.sin6_flowinfo = 0;
      ipv6addr.sin6_scope_id = 0;
      addrinfo = (sockaddr*) &ipv6addr;
      addrsize = sizeof(ipv6addr);
   }

   char buff1[64];
   char buff2[64];

   FunctionGuard guard(Guard_MakePreemptable);

   auto err = getnameinfo(addrinfo, addrsize, buff1, 64, buff2, 64, 0);

   if(err == 0)
   {
      name = buff1;
      service = buff2;
      return true;
   }

   OutputNwLog(NetworkFunctionError, "getnameinfo", err);
   return false;
}

//------------------------------------------------------------------------------

void SysIpL3Addr::Patch(sel_t selector, void* arguments)
{
   SysIpL2Addr::Patch(selector, arguments);
}
}
#endif
