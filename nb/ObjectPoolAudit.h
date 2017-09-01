//==============================================================================
//
//  ObjectPoolAudit.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef OBJECTPOOLAUDIT_H_INCLUDED
#define OBJECTPOOLAUDIT_H_INCLUDED

#include "Thread.h"
#include "Clock.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The object pool audit acts as a background garbage collector by recovering
//  orphaned PooledObjects.
//
class ObjectPoolAudit : public Thread
{
   friend class Singleton< ObjectPoolAudit >;
   friend class ObjectPoolRegistry;
public:
   //  Sets the audit interval.
   //
   void SetInterval(msecs_t interval);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Steps in the object pool audit.
   //
   enum Phase
   {
      CheckingFreeq,    // marking blocks and checking free queue
      ClaimingBlocks,   // application claiming in-use blocks
      RecoveringBlocks  // recovering unclaimed blocks
   };

   //  Private because this singleton is not subclassed.
   //
   ObjectPoolAudit();

   //  Private because this singleton is not subclassed.
   //
   ~ObjectPoolAudit();

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to provide the audit's entry function.
   //
   virtual void Enter() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;

   //  The time between audits.
   //
   msecs_t interval_;

   //  The work currently being performed by the audit.
   //
   Phase phase_;

   //  The pool currently being audited.
   //
   ObjectPoolId pid_;
};
}
#endif
