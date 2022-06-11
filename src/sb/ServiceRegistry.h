//==============================================================================
//
//  ServiceRegistry.h
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
#ifndef SERVICEREGISTRY_H_INCLUDED
#define SERVICEREGISTRY_H_INCLUDED

#include "Immutable.h"
#include "NbTypes.h"
#include "Registry.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for services.
//
class ServiceRegistry : public NodeBase::Immutable
{
   friend class NodeBase::Singleton<ServiceRegistry>;
   friend class Service;
public:
   //  Deleted to prohibit copying.
   //
   ServiceRegistry(const ServiceRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ServiceRegistry& operator=(const ServiceRegistry& that) = delete;

   //  Returns the services in the registry.
   //
   const NodeBase::Registry<Service>& Services() const { return services_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display each service.
   //
   void Summarize(std::ostream& stream, uint8_t index) const override;
private:
   //  Private because this is a singleton.
   //
   ServiceRegistry();

   //  Private because this is a singleton.
   //
   ~ServiceRegistry();

   //  Registers SERVICE against its service identifier.
   //
   bool BindService(Service& service);

   //  Removes SERVICE from the registry.
   //
   void UnbindService(Service& service);

   //  The global registry of services.
   //
   NodeBase::Registry<Service> services_;
};
}
#endif
