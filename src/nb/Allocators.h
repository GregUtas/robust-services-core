//==============================================================================
//
//  Allocators.h
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
#ifndef ALLOCATORS_H_INCLUDED
#define ALLOCATORS_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include "AllocationException.h"
#include "Memory.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  The following allocators, which support the various memory types, are for
//  use with STL containers (e.g. std::string, std::vector).  An object should
//  only declare an STL container as a member if the object itself resides in
//  MemPermanent (that is, derives from Permanent).
//
//  Say, for example, that an object in MemDynamic (derived from Dynamic) has
//  a std::string member.  Memory for the string's underlying array, however,
//  will be allocated from the default heap (MemPermanent), because of the STL
//  allocator that std::string uses.
//
//  During a cold restart, the *heap* for dynamic objects is deleted.  Because
//  the string itself is not explicitly deleted, this will leak the MemPermanent
//  allocated for the array.  Even if the dynamic object implements a Shutdown
//  function, it will be unable to free the MemPermanent, because std::string
//  doesn't provide a function that will do this.  The only way that the dynamic
//  object can avoid leaking memory is to allocate the std::string member with
//  new.  Let's say that it declares a stringPtr (std::unique_ptr<std::string>)
//  member for this purpose.  It can then implement a Shutdown function that
//  invokes reset() on this member during cold and reload restarts.
//
//  The same holds for other objects not in MemPermanent.  Note also that the
//  string for an object in MemProtected (derived from Protected) would not
//  actually be write-protected, because it would still reside in MemPermanent.
//
//  To avoid these drawbacks, an STL container in a non-MemPermanent object must
//  use an allocator in this interface.  To this end, NbTypes.h defines string
//  types (e.g. DynamicStr, ProtectedStr) that use these allocators.  The issue,
//  at least for strings, is that the string types in NbTypes.h have a different
//  type than std::string.  In some cases, this requires the use of .c_str() to
//  interwork between string types.  However, this is far less problematic than
//  using stringPtr, having the string's array reside in MemPermanent, and
//  having to implement a Shutdown function that will also slow down restarts.
//
//  Threads are not subject to the above restrictions.  If a thread exits during
//  a restart, its destructor is invoked, which safely deletes any STL members.
//
namespace NodeBase
{
template<typename T> struct DynamicAllocator
{
   typedef T value_type;

   DynamicAllocator() = default;

   ~DynamicAllocator() = default;

   DynamicAllocator(const DynamicAllocator<T>& that) noexcept { }

   DynamicAllocator& operator=(const DynamicAllocator& that) = default;

   template<typename U> DynamicAllocator
      (const DynamicAllocator<U>& that) noexcept { }

   template<typename U> bool operator==
      (const DynamicAllocator<U>& that) const noexcept { return true; }

   template<typename U> bool operator!=
      (const DynamicAllocator<U>& that) const noexcept { return false; }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemDynamic, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemDynamic);
      return static_cast<T*>(addr);
   }

   void deallocate(T* const addr, size_t n) const
   {
      Memory::Free(addr, MemDynamic);
   }
};

//------------------------------------------------------------------------------

template<typename T> struct ImmutableAllocator
{
   typedef T value_type;

   ImmutableAllocator() = default;

   ~ImmutableAllocator() = default;

   ImmutableAllocator(const ImmutableAllocator<T>& that) noexcept { }

   ImmutableAllocator& operator=(const ImmutableAllocator& that) = default;

   template<typename U> ImmutableAllocator
      (const ImmutableAllocator<U>& that) noexcept { }

   template<typename U> bool operator==
      (const ImmutableAllocator<U>& that) const noexcept { return true; }

   template<typename U> bool operator!=
      (const ImmutableAllocator<U>& that) const noexcept { return false; }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemImmutable, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemImmutable);
      return static_cast<T*>(addr);
   }

   void deallocate(T* const addr, size_t n) const
   {
      Memory::Free(addr, MemImmutable);
   }
};

//------------------------------------------------------------------------------

template<typename T> struct PermanentAllocator
{
   typedef T value_type;

   PermanentAllocator() = default;

   ~PermanentAllocator() = default;

   PermanentAllocator(const PermanentAllocator<T>& that) noexcept { }

   PermanentAllocator& operator=(const PermanentAllocator& that) = default;

   template<typename U> PermanentAllocator
      (const PermanentAllocator<U>& that) noexcept { }

   template<typename U> bool operator==
      (const PermanentAllocator<U>& that) const noexcept { return true; }

   template<typename U> bool operator!=
      (const PermanentAllocator<U>& that) const noexcept { return false; }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemPermanent, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemPermanent);
      return static_cast<T*>(addr);
   }

   void deallocate(T* const addr, size_t n) const
   {
      Memory::Free(addr, MemPermanent);
   }
};

//------------------------------------------------------------------------------

template<typename T> struct PersistentAllocator
{
   typedef T value_type;

   PersistentAllocator() = default;

   ~PersistentAllocator() = default;

   PersistentAllocator(const PersistentAllocator<T>& that) noexcept { }

   PersistentAllocator& operator=(const PersistentAllocator& that) = default;

   template<typename U> PersistentAllocator
      (const PersistentAllocator<U>& that) noexcept { }

   template<typename U> bool operator==
      (const PersistentAllocator<U>& that) const noexcept { return true; }

   template<typename U> bool operator!=
      (const PersistentAllocator<U>& that) const noexcept { return false; }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemPersistent, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemPersistent);
      return static_cast<T*>(addr);
   }

   void deallocate(T* const addr, size_t n) const
   {
      Memory::Free(addr, MemPersistent);
   }
};

//------------------------------------------------------------------------------

template<typename T> struct ProtectedAllocator
{
   typedef T value_type;

   ProtectedAllocator() = default;

   ~ProtectedAllocator() = default;

   ProtectedAllocator(const ProtectedAllocator<T>& that) noexcept { }

   ProtectedAllocator& operator=(const ProtectedAllocator& that) = default;

   template<typename U> ProtectedAllocator
      (const ProtectedAllocator<U>& that) noexcept { }

   template<typename U> bool operator==
      (const ProtectedAllocator<U>& that) const noexcept { return true; }

   template<typename U> bool operator!=
      (const ProtectedAllocator<U>& that) const noexcept { return false; }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemProtected, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemProtected);
      return static_cast<T*>(addr);
   }

   void deallocate(T* const addr, size_t n) const
   {
      Memory::Free(addr, MemProtected);
   }
};

//------------------------------------------------------------------------------

template<typename T> struct TemporaryAllocator
{
   typedef T value_type;

   TemporaryAllocator() = default;

   ~TemporaryAllocator() = default;

   TemporaryAllocator(const TemporaryAllocator<T>& that) noexcept { }

   TemporaryAllocator& operator=(const TemporaryAllocator& that) = default;

   template<typename U> TemporaryAllocator
      (const TemporaryAllocator<U>& that) noexcept { }

   template<typename U> bool operator==
      (const TemporaryAllocator<U>& that) const noexcept { return true; }

   template<typename U> bool operator!=
      (const TemporaryAllocator<U>& that) const noexcept { return false; }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemTemporary, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemTemporary);
      return static_cast<T*>(addr);
   }

   void deallocate(T* const addr, size_t n) const
   {
      Memory::Free(addr, MemTemporary);
   }
};
}
#endif
