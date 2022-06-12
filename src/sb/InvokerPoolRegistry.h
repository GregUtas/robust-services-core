//==============================================================================
//
//  InvokerPoolRegistry.h
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
#ifndef INVOKERPOOLREGISTRY_H_INCLUDED
#define INVOKERPOOLREGISTRY_H_INCLUDED

#include "Dynamic.h"
#include "NbTypes.h"
#include "Registry.h"

namespace SessionBase
{
   class InvokerPool;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for invoker pools.
//
class InvokerPoolRegistry : public NodeBase::Dynamic
{
   friend class NodeBase::Singleton<InvokerPoolRegistry>;
   friend class InvokerPool;
public:
   //  Deleted to prohibit copying.
   //
   InvokerPoolRegistry(const InvokerPoolRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   InvokerPoolRegistry& operator=(const InvokerPoolRegistry& that) = delete;

   //  Returns the pool registered against FACTION.
   //
   InvokerPool* Pool(NodeBase::Faction faction) const;

   //  Returns the registry of invoker pools.  Used for iteration.
   //
   const NodeBase::Registry<InvokerPool>& Pools() const { return pools_; }

   //  Overridden to mark the objects in each pool as being in use.
   //
   void ClaimBlocks() override;

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

   //  Overridden to display each pool.
   //
   size_t Summarize(std::ostream& stream, uint32_t selector) const override;
private:
   //  Private because this is a singleton.
   //
   InvokerPoolRegistry();

   //  Private because this is a singleton.
   //
   ~InvokerPoolRegistry();

   //  Adds POOL to the registry against its scheduler faction.
   //
   bool BindPool(InvokerPool& pool);

   //  Removes POOL from the registry.
   //
   void UnbindPool(InvokerPool& pool);

   //  The global registry of invoker pools.
   //
   NodeBase::Registry<InvokerPool> pools_;

   //  The statistics group for invoker pools.
   //
   NodeBase::StatisticsGroupPtr statsGroup_;

   //  The pool currently being audited (cast as a Faction, but declared
   //  as an int to simplify incrementing).
   //
   int poolToAudit_;
};
}
#endif
