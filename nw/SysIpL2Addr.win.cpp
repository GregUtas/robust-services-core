//==============================================================================
//
//  SysIpL2Addr.win.cpp
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
#ifdef OS_WIN
#include "SysIpL2Addr.h"
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
fn_name SysIpL2Addr_ctor3 = "SysIpL2Addr.ctor";

SysIpL2Addr::SysIpL2Addr() : v4Addr_(INADDR_NONE)
{
   Debug::ft(SysIpL2Addr_ctor3);
}

//------------------------------------------------------------------------------

fn_name SysIpL2Addr_ctor4 = "SysIpL2Addr.ctor(string)";

SysIpL2Addr::SysIpL2Addr(const string& text) : v4Addr_(INADDR_NONE)
{
   Debug::ft(SysIpL2Addr_ctor4);

   in_addr result;

   auto rc = inet_pton(AF_INET, text.c_str(), &result);

   switch(rc)
   {
   case 1:
      v4Addr_ = ntohl(result.s_addr);
      break;
   case 0:
      Debug::SwErr(SysIpL2Addr_ctor4, text, 0);
      break;
   default:
      Debug::SwErr(SysIpL2Addr_ctor4, WSAGetLastError(), 0);
   }
}

//------------------------------------------------------------------------------

fn_name SysIpL2Addr_HostName = "SysIpL2Addr.HostName";

bool SysIpL2Addr::HostName(string& name)
{
   Debug::ft(SysIpL2Addr_HostName);

   char buff[64];

   name.clear();

   if(gethostname(buff, 64) == SOCKET_ERROR)
   {
      auto log = Log::Create("IP GETHOSTNAME ERROR");

      if(log != nullptr)
      {
         *log << "errval=" << WSAGetLastError() << CRLF;
         Log::Spool(log);
      }
      return false;
   }

   name = buff;
   return true;
}

//------------------------------------------------------------------------------

fn_name SysIpL2Addr_IsValid = "SysIpL2Addr.IsValid";

bool SysIpL2Addr::IsValid() const
{
   Debug::ft(SysIpL2Addr_IsValid);

   return (v4Addr_ != INADDR_NONE);
}

//------------------------------------------------------------------------------

SysIpL2Addr SysIpL2Addr::LoopbackAddr()
{
   return SysIpL2Addr(INADDR_LOOPBACK);
}

//------------------------------------------------------------------------------

void SysIpL2Addr::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
#endif