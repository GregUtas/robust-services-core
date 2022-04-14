//==============================================================================
//
//  SysIpL2Addr.linux.cpp
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

#include "SysIpL2Addr.h"
#include <cstring>
#include "Debug.h"
#include "NwLogs.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name SysIpL2Addr_LocalAddrs = "SysIpL2Addr.LocalAddrs";

std::vector< SysIpL2Addr > SysIpL2Addr::LocalAddrs()
{
   Debug::ft(SysIpL2Addr_LocalAddrs);

   std::vector< SysIpL2Addr > localAddrs;

/*L
   string self;

   if(LocalName(self))
   {
      addrinfo hints;
      addrinfo* info = nullptr;

      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_UNSPEC;

      if(getaddrinfo(self.c_str(), nullptr, &hints, &info) == 0)
      {
         for(auto curr = info; curr != nullptr; curr = curr->ai_next)
         {
            SysIpL2Addr localAddr;

            switch(curr->ai_family)
            {
            case AF_INET:
            {
               auto netaddr = (sockaddr_in*) curr->ai_addr;
               localAddr.NetworkToHost(netaddr->sin_addr.s_addr);
               localAddrs.push_back(localAddr);
               break;
            }

            case AF_INET6:
            {
               auto netaddr = (sockaddr_in6*) curr->ai_addr;
               if(netaddr->sin6_scope_id != 0) continue;
               localAddr.NetworkToHost(netaddr->sin6_addr.s6_words);
               localAddrs.push_back(localAddr);
               break;
            }

            default:
               Debug::SwLog(SysIpL2Addr_LocalAddrs,
                  "unsupported protocol family", curr->ai_family);
            }
         }

         freeaddrinfo(info);
      }
      else
      {
         OutputNwLog(NetworkFunctionError, "getaddrinfo", WSAGetLastError());
      }
   }
*/
   return localAddrs;
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::LocalName(string& name)
{
   Debug::ft("SysIpL2Addr.LocalName");

   return false;
/*L
   char buff[256];

   name.clear();

   if(gethostname(buff, 256) == SOCKET_ERROR)
   {
      OutputNwLog(NetworkFunctionError, "gethostname", WSAGetLastError());
      return false;
   }

   name = buff;
   return true;
*/
}

//------------------------------------------------------------------------------

void SysIpL2Addr::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::SupportsIPv6()
{
   return true;
}
}
#endif
