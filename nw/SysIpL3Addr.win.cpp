//==============================================================================
//
//  SysIpL3Addr.win.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifdef OS_WIN
#include "SysIpL3Addr.h"
#include <cstring>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Debug.h"
#include "Log.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
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
            Debug::SwErr(SysIpL3Addr_ctor, info->ai_protocol, 1);
         }
      }
      else
      {
         Debug::SwErr(SysIpL3Addr_ctor, info->ai_family, 1);
      }

      freeaddrinfo(info);
   }
   else
   {
      auto log = Log::Create("IP GETADDRINFO ERROR");

      if(log != nullptr)
      {
         *log << "errval=" << WSAGetLastError() << CRLF;
         Log::Spool(log);
      }
   }

   proto = proto_;
}

//------------------------------------------------------------------------------

fn_name SysIpL3Addr_AddrToName = "SysIpL3Addr.AddrToName";

bool SysIpL3Addr::AddrToName(string& name, string& service) const
{
   Debug::ft(SysIpL3Addr_AddrToName);

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

   auto log = Log::Create("IP GETNAMEINFO ERROR");

   if(log != nullptr)
   {
      *log << "errval=" << WSAGetLastError() << CRLF;
      Log::Spool(log);
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
