//==============================================================================
//
//  ObjectPoolRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef OBJECTPOOLREGISTRY_H_INCLUDED
#define OBJECTPOOLREGISTRY_H_INCLUDED

#include "Protected.h"
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
   //  Returns the pool registered against PID.
   //
   ObjectPool* Pool(ObjectPoolId pid) const;

   //  Returns the registry of object pools.  Used for iteration.
   //
   const Registry< ObjectPool >& Pools() const { return pools_; }

   //  Returns true if full object nullification is enabled.
   //
   static bool NullifyObjectData() { return NullifyObjectData_; }

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ObjectPoolRegistry();

   //  Private because this singleton is not subclassed.
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
   CfgBoolParmPtr nullifyObjectData_;

   //  Causes an object's data to be nullified after its vptr.
   //
   static bool NullifyObjectData_;

   //  The statistics group for object pools.
   //
   StatisticsGroupPtr statsGroup_;
};
}
#endif
