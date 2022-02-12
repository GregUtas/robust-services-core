//==============================================================================
//
//  IpPortCfgParm.h
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
#ifndef IPPORTCFGPARM_H_INCLUDED
#define IPPORTCFGPARM_H_INCLUDED

#include "CfgIntParm.h"
#include "NwTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Configuration parameter for IP ports.
//
class IpPortCfgParm : public NodeBase::CfgIntParm
{
public:
   //  Creates a parameter with the specified attributes, which are described
   //  in the base class constructor.  SERVICE identifies what is running on
   //  the port.
   //
   IpPortCfgParm(NodeBase::c_string key, NodeBase::c_string def,
      NodeBase::c_string expl, const IpService* service);

   //  Virtual to allow subclassing.
   //
   virtual ~IpPortCfgParm();

   //  Returns the port on which the service is configured.
   //
   ipport_t GetPort() const { return ipport_t(CurrValue()); }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to check if VALUE (an IP port) is available.
   //
   bool SetNextValue(NodeBase::word value) override;
private:
   //  Overridden to indicate that a cold restart is required to move an IP
   //  service to a new port.
   //
   NodeBase::RestartLevel RestartRequired() const override
      { return NodeBase::RestartCold; }

   //  The service running on the port.
   //
   const IpService* const service_;
};
}
#endif
