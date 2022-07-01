//==============================================================================
//
//  IpServiceCfg.cpp
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
#include "IpServiceCfg.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "Singleton.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
IpServiceCfg::IpServiceCfg(c_string key, c_string def,
   c_string expl, IpService* service) : CfgBoolParm(key, def, expl),
   service_(service)
{
   Debug::ft("IpServiceCfg.ctor");
}

//------------------------------------------------------------------------------

IpServiceCfg::~IpServiceCfg()
{
   Debug::ftnt("IpServiceCfg.dtor");
}

//------------------------------------------------------------------------------

RestartLevel IpServiceCfg::RestartRequired() const
{
   Debug::ftnt("IpServiceCfg.RestartRequired");

   //  A restart is required to disable, but not to enable, a service.
   //
   return (NextValue() ? RestartNone : RestartCold);
}

//------------------------------------------------------------------------------

void IpServiceCfg::SetCurr()
{
   Debug::ft("IpServiceCfg.SetCurr");

   FunctionGuard guard(Guard_MemUnprotect);
   CfgBoolParm::SetCurr();

   //  If the service was enabled, create its I/O thread.
   //
   if(CurrValue())
   {
      auto reg = Singleton<IpPortRegistry>::Instance();
      auto port = reg->GetPort(service_->Port(), service_->Protocol());

      if(port != nullptr)
      {
         port->CreateThread();
      }
   }
}
}
