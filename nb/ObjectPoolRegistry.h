//==============================================================================
//
//  ObjectPoolRegistry.h
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
#ifndef OBJECTPOOLREGISTRY_H_INCLUDED
#define OBJECTPOOLREGISTRY_H_INCLUDED

#include "Protected.h"
#include "CfgBoolParm.h"
#include "NbTypes.h"
#include "Registry.h"

namespace NodeBase
{
   class ObjectPool;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for object pools.
//
class ObjectPoolRegistry : public Protected
{
   friend class Singleton< ObjectPoolRegistry >;
   friend class ObjectPool;
   friend class ObjectPoolAudit;
public:
   //  Deleted to prohibit copying.
   //
   ObjectPoolRegistry(const ObjectPoolRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ObjectPoolRegistry& operator=(const ObjectPoolRegistry& that) = delete;

   //  Returns the pool registered against PID.
   //
   ObjectPool* Pool(ObjectPoolId pid) const;

   //  Returns the registry of object pools.  Used for iteration.
   //
   const Registry< ObjectPool >& Pools() const { return pools_; }

   //  Returns true if full object nullification is enabled.
   //
   bool NullifyObjectData() const { return nullifyObjectDataCfg_->GetValue(); }

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   ObjectPoolRegistry();

   //  Private because this is a singleton.
   //
   ~ObjectPoolRegistry();

   //  Adds POOL to the registry.
   //
   bool BindPool(ObjectPool& pool);

   //  Removes POOL from the registry.
   //
   void UnbindPool(ObjectPool& pool);

   //  Performs an audit on each pool.
   //
   void AuditPools() const;

   //  The global registry of object pools.
   //
   Registry< ObjectPool > pools_;

   //  Configuration parameter for object nullification.
   //
   CfgBoolParmPtr nullifyObjectDataCfg_;

   //  The statistics group for object pools.
   //
   StatisticsGroupPtr statsGroup_;
};
}
#endif
