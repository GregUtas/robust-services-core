//==============================================================================
//
//  Base.h
//
//  Copyright (C) 2017  Greg Utas
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
#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
#include "SysTypes.h"

namespace NodeBase
{
   class Class;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Each non-trivial class should ultimately derive from this, although most
//  base classes will derive from one of the following, which determines the
//  MemoryType that they use and, consequently, which restarts they survive:
//    o Temporary: does not survive any restart
//    o Dynamic:   survives warm restarts
//    o Pooled:    survives warm restarts; allocated from an ObjectPool
//    o Protected: survives warm and cold restarts; write-protected
//    o Permanent: survives all restarts; allocated from the default heap
//    o Immutable: survives all restarts; write-protected
//  The default C++ heap is the permanent heap, but the strategy of escalating
//  restarts means that few objects should derive from Permanent or Immutable.
//
class Base
{
   friend class Object;
   friend class Class;
   friend class ObjectPool;
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Base() = default;

   //  Displays the object in STREAM.  The default implementation displays
   //  the object's class name and its "this" pointer, using the typical
   //  form for each member:
   //    stream << prefix << "member : " << member_ << CRLF;
   //
   //  When a member is a class itself, it is usually displayed by
   //    stream << prefix << "member : " << CRLF;
   //    member_->Display(stream, prefix + spaces(2), options);
   //
   //  OPTIONS specifies display options.  Its interpretation is left to
   //  various class hierarchies.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const;

   //  Invokes Display(stream, spaces(indent), opts), setting DispVerbose
   //  in OPTS if VERBOSE is set.
   //
   void Output(std::ostream& stream, col_t indent, bool verbose) const;

   //  Adds the object to OBJECTS.  Used by ClaimBlocks and LogSubtended,
   //  and must therefore be overridden by subclasses of Pooled so that
   //  transitively owned pooled objects are also added to OBJECTS:
   //
   //  void MyObject::GetSubtended(std::vector< Base* >& objects)
   //  {
   //     MyBaseClass::GetSubtended(objects);
   //     for(each object X that I own) X.GetSubtended(objects);
   //  }
   //
   virtual void GetSubtended(std::vector< Base* >& objects) const;

   //  Logs this object and all subtended objects by invoking each of their
   //  Display functions with STREAM, PREFIX, and OPTIONS.
   //
   void LogSubtended(std::ostream& stream,
      const std::string& prefix, const Flags& options) const;

   //  Invokes Claim on the object and all of the blocks that it owns.
   //  The default version claims all objects returned by GetSubtended.
   //
   virtual void ClaimBlocks();

   //  Invoked during error recovery to perform a subset of the work done
   //  by the destructor.  What it MUST omit is deleting any Pooled object.
   //  Pooled objects are recovered separately, by the object pool audit.
   //    This function exists because it is risky to use operator delete
   //  during error recovery.  The error could have occurred because a
   //  pointer to an owned object was corrupt.  Deletion then causes an
   //  exception while handling an exception, which is extremely dangerous.
   //  A non-pooled object can be deleted to prevent a memory leak, given
   //  that the pointer to it will *usually* be valid.  But given that the
   //  audit will recover a Pooled object, it should not be deleted.
   //    A Cleanup function must not rely on anything but itself.  If it
   //  owns other objects, those objects might already have been cleaned
   //  up and recovered.  Similarly, the object's owner could also be gone.
   //    After a Cleanup function performs its work, it MUST invoke its
   //  base class version.
   //
   virtual void Cleanup() { }

   //  Allocates resources when exiting a restart at LEVEL.  This allows an
   //  object that survived a restart to reallocate resources that did not.
   //  Invokes Restart::Initiate on a failure that warrants an escalation of
   //  LEVEL.
   //
   virtual void Startup(RestartLevel level) { }

   //  Releases resources when entering a restart at LEVEL.  If the object
   //  will survive the restart but some of its resources will not, it may
   //  need to remove those resources from a registry or clear the pointers
   //  that refer to them.  The object's Startup function will subsequently
   //  reallocate these resources.
   //
   virtual void Shutdown(RestartLevel level) { }

   //  Returns the type of memory used by the object.
   //
   virtual MemoryType MemType() const { return MemPermanent; }

   //  Overridden to invoke Debug::ft before the global ::operator new.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);

   //  Placement new (the above versions hide the global placement new).
   //
   static void* operator new(size_t size, void* place);
   static void* operator new[](size_t size, void* place);
protected:
   //  Protected because this class is virtual.
   //
   Base() = default;
private:
   //  Marks an object as in-use so that an audit will not reclaim it.  The
   //  version here does nothing; the function is defined at this level so
   //  that GetSubtended can be used to create a list of blocks to claim.
   //
   virtual void Claim() { }

   //  Type for an object's pointer to its virtual function table.
   //
   typedef uintptr_t vptr_t;

   //  For manipulating an object at a low level.
   //
   struct ObjectStruct
   {
      vptr_t vptr;                 // resides at the top of each object
      uintptr_t data[UINT16_MAX];  // defined for nullification
   };

   //  Returns the object's vptr.  Given that this class defines virtual
   //  functions, the vptr should reside at the top of the object.
   //
   vptr_t Vptr() const;

   //  Nullifies the object by setting its vptr and N additional bytes to
   //  BAD_POINTER.
   //
   void Nullify(size_t n);
};
}
#endif
