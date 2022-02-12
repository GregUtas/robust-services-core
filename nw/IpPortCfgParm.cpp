//==============================================================================
//
//  IpPortCfgParm.cpp
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
#include "IpPortCfgParm.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
IpPortCfgParm::IpPortCfgParm(c_string key, c_string def,
   c_string expl, const IpService* service) :
   CfgIntParm(key, def, FirstAppIpPort, LastAppIpPort, expl),
   service_(service)
{
   Debug::ft("IpPortCfgParm.ctor");
}

//------------------------------------------------------------------------------

IpPortCfgParm::~IpPortCfgParm()
{
   Debug::ftnt("IpPortCfgParm.dtor");
}

//------------------------------------------------------------------------------

void IpPortCfgParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgIntParm::Display(stream, prefix, options);

   stream << prefix << "service : " << strObj(service_) << CRLF;
}

//------------------------------------------------------------------------------

void IpPortCfgParm::Patch(sel_t selector, void* arguments)
{
   CfgParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool IpPortCfgParm::SetNextValue(word value)
{
   Debug::ft("IpPortCfgParm.SetNextValue");

   //  Check that the port is valid and available for the intended protocol.
   //
   if((value >= FirstAppIpPort) && (value <= LastAppIpPort))
   {
      auto reg = Singleton< IpPortRegistry >::Instance();

      if(reg->GetPort(value, service_->Protocol()) == nullptr)
      {
         return CfgIntParm::SetNextValue(value);
      }
   }

   return false;
}
}
