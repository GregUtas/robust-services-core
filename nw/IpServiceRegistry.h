//==============================================================================
//
//  IpServiceRegistry.h
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
#ifndef IPSERVICEREGISTRY_H_INCLUDED
#define IPSERVICEREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <string>
#include <vector>
#include "NbTypes.h"
#include "Registry.h"

namespace NetworkBase
{
   class IpService;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Global registry for services that use IP protocols.
//
class IpServiceRegistry : public NodeBase::Immutable
{
   friend class NodeBase::Singleton< IpServiceRegistry >;
   friend class IpService;
public:
   //  Deleted to prohibit copying.
   //
   IpServiceRegistry(const IpServiceRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   IpServiceRegistry& operator=(const IpServiceRegistry& that) = delete;

   //  Returns the service(s) registered against NAME.
   //
   std::vector< IpService* > GetServices(const std::string& name) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   IpServiceRegistry();

   //  Private because this is a singleton.
   //
   ~IpServiceRegistry();

   //  Adds SERVICE to the registry.
   //
   bool BindService(IpService& service);

   //  Removes SERVICE from the registry.
   //
   void UnbindService(IpService& service);

   //  The global registry of IP services.
   //
   NodeBase::Registry< IpService > services_;
};
}
#endif
