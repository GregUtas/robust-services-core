//==============================================================================
//
//  FactoryRegistry.h
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
#ifndef FACTORYREGISTRY_H_INCLUDED
#define FACTORYREGISTRY_H_INCLUDED

#include "Immutable.h"
#include "NbTypes.h"
#include "Registry.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for factories.
//
class FactoryRegistry : public NodeBase::Immutable
{
   friend class NodeBase::Singleton< FactoryRegistry >;
   friend class Factory;
public:
   //  Deleted to prohibit copying.
   //
   FactoryRegistry(const FactoryRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   FactoryRegistry& operator=(const FactoryRegistry& that) = delete;

   //  Returns the factory registered against FID.
   //
   Factory* GetFactory(FactoryId fid) const;

   //  Returns the registry of factories.  Used for iteration.
   //
   const NodeBase::Registry< Factory >& Factories() const { return factories_; }

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
   FactoryRegistry();

   //  Private because this is a singleton.
   //
   ~FactoryRegistry();

   //  Registers FACTORY against its identifier.  Invoked by Factory's base
   //  class constructor.
   //
   bool BindFactory(Factory& factory);

   //  Removes FACTORY from the registry.  Invoked by Factory's base class
   //  destructor.
   //
   void UnbindFactory(Factory& factory);

   //  The global registry of factories.
   //
   NodeBase::Registry< Factory > factories_;

   //  The statistics group for factories.
   //
   NodeBase::StatisticsGroupPtr statsGroup_;
};
}
#endif
