//==============================================================================
//
//  SysIpL2Addr.win.cpp
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

#include "SysIpL2Addr.h"
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
SysIpL2Addr::SysIpL2Addr() : v4Addr_(INADDR_NONE)
{
   Debug::ft("SysIpL2Addr.ctor");
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
      Debug::SwLog(SysIpL2Addr_ctor4, text, 0);
      break;
   default:
      Debug::SwLog(SysIpL2Addr_ctor4, "inet_pton failed", WSAGetLastError());
   }
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::HostName(string& name)
{
   Debug::ft("SysIpL2Addr.HostName");

   char buff[256];

   name.clear();

   if(gethostname(buff, 256) == SOCKET_ERROR)
   {
      auto log = Log::Create(NetworkLogGroup, NetworkFunctionError);

      if(log != nullptr)
      {
         *log << Log::Tab << "GetHostName: errval=" << WSAGetLastError();
         Log::Submit(log);
      }
      return false;
   }

   name = buff;
   return true;
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::IsValid() const
{
   Debug::ft("SysIpL2Addr.IsValid");

   return (v4Addr_ != INADDR_NONE);
}

//------------------------------------------------------------------------------

SysIpL2Addr SysIpL2Addr::LoopbackAddr()
{
   Debug::ft("SysIpL2Addr.LoopbackAddr");

   return SysIpL2Addr(INADDR_LOOPBACK);
}

//------------------------------------------------------------------------------

void SysIpL2Addr::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
#endif
