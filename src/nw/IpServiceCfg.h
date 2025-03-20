//==============================================================================
//
//  IpServiceCfg.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef IPSERVICECFG_H_INCLUDED
#define IPSERVICECFG_H_INCLUDED

#include "CfgBoolParm.h"
#include "NwTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Configuration parameter for enabling or disabling an IpService.
//  If it is not enabled, no I/O thread is created for it.
//
class IpServiceCfg : public CfgBoolParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   IpServiceCfg(c_string key, c_string def, c_string expl, IpService* service);

   //  Virtual to allow subclassing.
   //
   virtual ~IpServiceCfg();
private:
   //  Overridden to indicate that a cold restart is required to disable a
   //  service.  Enabling a service does not require a restart.
   //
   RestartLevel RestartRequired() const override;

   //  Overridden to create the service's I/O thread when it is enabled.
   //
   void SetCurr() override;

   //  The service associated with the parameter.
   //
   IpService* const service_;
};
}
#endif
