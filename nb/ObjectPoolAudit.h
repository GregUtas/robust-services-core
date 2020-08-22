//==============================================================================
//
//  ObjectPoolAudit.h
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
#ifndef OBJECTPOOLAUDIT_H_INCLUDED
#define OBJECTPOOLAUDIT_H_INCLUDED

#include "Thread.h"
#include "Duration.h"
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
   void SetInterval(const Duration& interval);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
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
   c_string AbbrName() const override;

   //  Overridden to provide the audit's entry function.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  The time between audits.
   //
   Duration interval_;

   //  The work currently being performed by the audit.
   //
   Phase phase_;

   //  The pool currently being audited.
   //
   ObjectPoolId pid_;
};
}
#endif
