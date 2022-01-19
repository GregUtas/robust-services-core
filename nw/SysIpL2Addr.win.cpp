//==============================================================================
//
//  SysIpL2Addr.win.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "Debug.h"
#include "Log.h"
#include "NwLogs.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
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
         *log << Log::Tab << "gethostname: errval=" << WSAGetLastError();
         Log::Submit(log);
      }
      return false;
   }

   name = buff;
   return true;
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
