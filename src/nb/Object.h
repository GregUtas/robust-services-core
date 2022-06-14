//==============================================================================
//
//  Object.h
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
#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include "Base.h"
#include <cstddef>
#include <cstdint>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  This extends Base to provide support for
//  o patching (Patch and patchArea_)
//  o association with a Class (most everything else in this header)
//
//  NOTE: Objects allocated on the heap must derive from Pooled, Temporary,
//  ====  Dynamic, Persistent, Protected, Permanent, or Immutable.
//
class Object : public Base
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Object() = default;

   //  Copy constructor.
   //
   Object(const Object& that) = default;

   //  Copy operator.
   //
   Object& operator=(const Object& that) = default;

   //  Selector for the Patch function.
   //
   typedef uint8_t sel_t;

   //  Allows a new function to be "added" without changing the interface.
   //  For lab use and delivery of bug fixes only.  A non-trivial subclass
   //  must override this by invoking the same function on its base class.
   //  SELECTOR is used in a switch statement to select the "function" being
   //  invoked.  ARGUMENTS is cast as whatever struct supplies arguments
   //  and/or returned values for the specific SELECTOR being used.
   //
   virtual void Patch(sel_t selector, void* arguments) { }

   ////////////////////////////////////////////////////////////////////////////
   //
   //  The rest of the public interface is only used by objects that are
   //  supported by a subclass of Class (see Class.h).

   typedef id_t ClassId;         // identifies a Class
   typedef uint32_t InstanceId;  // identifies an object within a Class
   typedef uint32_t ObjectId;    // ClassId (12 bits) + InstanceId (20 bits)

   static const ClassId MaxClassId = (1 << 12) - 1;

   static const size_t MaxInstanceIdLog2 = 20;
   static const InstanceId MaxInstanceId = (1 << MaxInstanceIdLog2) - 1;

   //  Returns the Class singleton associated with this object.  Must be
   //  overridden by subclasses that have an associated Class.  The default
   //  version returns nullptr.
   //
   virtual Class* GetClass() const;

   //  Called to initialize member data when using the Object Template
   //  technique.  The default version does nothing and is overridden by
   //  classes that use Object Template but that also have data members
   //  whose initialization depends on information that is only available
   //  at run time.
   //
   virtual void PostInitialize() { }

   //  Returns the class to which the object belongs.  Returns NIL_ID
   //  if the object is not associated with a Class.
   //
   ClassId GetClassId() const;

   //  Returns the object's instance identifier within its Class.  Must be
   //  overridden by subclasses that support instance identifiers.  The
   //  default version returns NIL_ID.
   //
   virtual InstanceId GetInstanceId() const;

   //  Returns the object's identifier, which is a combination of its class
   //  and instance identifiers.  Returns NIL_ID if the object is not
   //  associated with a Class or does not support instance identifiers.
   //
   ObjectId GetObjectId() const;

   //  Given object identifier OID, returns object's instance identifier in
   //  ID and its associated Class in CLS.  Returns false if the object has
   //  no instance identifier or Class.
   //
   static bool GetClassInstanceId(ObjectId oid, Class*& cls, InstanceId& iid);

   //  Deleted to prevent heap allocation for objects directly derived from
   //  this class.  Such objects must derive from Pooled, Temporary, Dynamic,
   //  Persistent, Protected, Permanent, or Immutable.
   //
   static void* operator new(size_t size) = delete;
   static void* operator new[](size_t size) = delete;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   Object() : patchArea_(0) { }

   //  Morphs the object to the class associated with TARGET by changing
   //  its vptr.
   //
   void MorphTo(const Class& target);
private:
   //  If a Patch function requires additional memory, this provides enough
   //  space for a pointer to dynamically allocated memory.
   //
   uintptr_t patchArea_;
};
}
#endif
