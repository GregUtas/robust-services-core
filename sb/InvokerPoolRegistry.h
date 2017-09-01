//==============================================================================
//
//  InvokerPoolRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for invoker pools.
//
class InvokerPoolRegistry : public Dynamic
{
   friend class Singleton< InvokerPoolRegistry >;
public:
   //  Adds POOL to the registry against its scheduler faction.
   //
   bool BindPool(InvokerPool& pool);

   //  Removes POOL from the registry.
   //
   void UnbindPool(InvokerPool& pool);

   //  Returns the pool registered against FACTION.
   //
   InvokerPool* Pool(Faction faction) const;

   //  Returns the registry of invoker pools.  Used for iteration.
   //
   const Registry< InvokerPool >& Pools() const { return pools_; }

   //  Overridden to mark the objects in each pool as being in use.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

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
   InvokerPoolRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~InvokerPoolRegistry();

   //  The global registry of invoker pools.
   //
   Registry< InvokerPool > pools_;

   //  The statistics group for invoker pools.
   //
   StatisticsGroupPtr statsGroup_;

   //  The pool currently being audited (cast as a Faction, but declared
   //  as an int to simplify incrementing).
   //
   int poolToAudit_;
};
}
#endif
