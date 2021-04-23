//==============================================================================
//
//  Pooled.h
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
#ifndef POOLED_H_INCLUDED
#define POOLED_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include <cstdint>
#include "Q1Link.h"

namespace NodeBase
{
   class ObjectPool;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A pooled object is allocated from an ObjectPool created during system
//  initialization rather than from the heap.
//
class Pooled : public Object
{
   friend class ObjectPool;
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Pooled() = default;

   //  Deleted to prohibit copying.
   //
   Pooled(const Pooled& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Pooled& operator=(const Pooled& that) = delete;

   //  Returns true if the object is marked corrupt.
   //
   bool IsCorrupt() const { return corrupt_; }

   //  Returns true if the object is invalid.
   //
   bool IsInvalid() const { return !assigned_; }

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to claim blocks that this object owns.  If the object is
   //  marked corrupt, it simply returns; otherwise, it surrounds a call
   //  to Object::ClaimBlocks by setting and clearing the corrupt_ flag.
   //
   void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to return the type of memory used by subclasses.
   //
   MemoryType MemType() const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to return a block to its object pool.
   //
   static void operator delete(void* addr);
protected:
   //  Protected because this class is virtual.
   //
   Pooled();
private:
   //  Clears the object's orphaned_ field so that the object pool audit
   //  will not reclaim it.  May be overridden, but the base class version
   //  must be invoked.
   //
   void Claim() override;

   //  Link for queueing the object.
   //
   Q1Link link_;

   //  True if allocated for an object; false if on free queue.
   //
   bool assigned_;

   //  Zero for a block that is in use.  Incremented each time through the
   //  audit; if it reaches a threshold, the block is deemed to be orphaned
   //  and is recovered.
   //
   uint8_t orphaned_;

   //  Used by audits to avoid invoking functions on a corrupt block.  The
   //  audit sets this flag before it invokes any function on the object.
   //  If the object's function traps, the flag is still set when the audit
   //  resumes execution, so it knows that the block is corrupt and simply
   //  recovers it instead of invoking its function again.  If the function
   //  returns successfully, the audit immediately clears the flag.
   //
   bool corrupt_;

   //  Used by audits to avoid double logging.
   //
   bool logged_;
};
}
#endif
