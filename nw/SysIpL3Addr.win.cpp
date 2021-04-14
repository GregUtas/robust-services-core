//==============================================================================
//
//  SysIpL3Addr.win.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifdef OS_WIN

#include "SysIpL3Addr.h"
#include <cstring>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Debug.h"
#include "Log.h"
#include "NwLogs.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name SysIpL3Addr_ctor = "SysIpL3Addr.ctor(name, service)";

SysIpL3Addr::SysIpL3Addr(const string& name,
   const string& service, IpProtocol& proto) : SysIpL2Addr(INADDR_NONE),
   port_(NilIpPort),
   proto_(IpAny),
   socket_(nullptr)
{
   Debug::ft(SysIpL3Addr_ctor);

   addrinfo hints;
   addrinfo* info = nullptr;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;

   if(getaddrinfo(name.c_str(), service.c_str(), &hints, &info) == 0)
   {
      if(info->ai_family == AF_INET)
      {
         auto netaddr = (sockaddr_in*) info->ai_addr;
         SetIpV4Addr(ntohl(netaddr->sin_addr.s_addr));
         port_ = ntohs(netaddr->sin_port);

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
      }
      else
      {
         Debug::SwLog(SysIpL3Addr_ctor,
            "unsupported protocol family", info->ai_family);
      }

      freeaddrinfo(info);
   }
   else
   {
      auto log = Log::Create(NetworkLogGroup, NetworkFunctionError);

      if(log != nullptr)
      {
         *log << Log::Tab << "GetAddrInfo: errval=" << WSAGetLastError();
         Log::Submit(log);
      }
   }

   proto = proto_;
}

//------------------------------------------------------------------------------

bool SysIpL3Addr::AddrToName(string& name, string& service) const
{
   Debug::ft("SysIpL3Addr.AddrToName");

   sockaddr_in addr;
   char buff1[64];
   char buff2[64];

   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(GetIpV4Addr());
   addr.sin_port = htons(port_);

   if(getnameinfo
      ((sockaddr*) &addr, sizeof(addr), buff1, 64, buff2, 64, 0) == 0)
   {
      name = buff1;
      service = buff2;
      return true;
   }

   auto log = Log::Create(NetworkLogGroup, NetworkFunctionError);

   if(log != nullptr)
   {
      *log << Log::Tab << "GetNameInfo: errval=" << WSAGetLastError();
      Log::Submit(log);
   }
   return false;
}

//------------------------------------------------------------------------------

void SysIpL3Addr::Patch(sel_t selector, void* arguments)
{
   SysIpL2Addr::Patch(selector, arguments);
}
}
#endif
