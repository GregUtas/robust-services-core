//==============================================================================
//
//  Allocators.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ALLOCATORS_H_INCLUDED
#define ALLOCATORS_H_INCLUDED

#include <memory>
#include "AllocationException.h"
#include "Memory.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  The following allocators, which support the various memory types, are for
//  use with STL containers (e.g. std::string, std::vector).  An object should
//  only declare an STL container as a member if the object itself resides in
//  MemPerm (that is, derives from Permanent).
//
//  Say, for example, that an object in MemDyn (derived from Dynamic) declares
//  a std::string member.  Memory for the string's underlying array, however,
//  will be allocated from the default heap (MemPerm), because of the STL
//  allocator that std::string uses.
//
//  During a cold restart, the *heap* for dynamic objects is deleted.  Because
//  the string itself is not explicitly deleted, this will leak the MemPerm
//  allocated for the array.  Even if the dynamic object implements a Shutdown
//  function, it will be unable to free the MemPerm, because std::string does
//  not provide a function that will do this.  The only way that the dynamic
//  object can avoid leaking memory is to allocate the std::string member with
//  new.  Let's say that it declares a stringPtr (std::unique_ptr<std::string>)
//  member for this purpose.  It can then implement a Shutdown function that
//  invokes reset() on this member during cold and reload restarts.
//
//  The same holds for other objects not in MemPerm.  Note also that the string
//  for an object in MemProt (derived from Protected) would not actually be
//  write-protected, because it would still reside in MemPerm.
//
//  To avoid these drawbacks, an STL container in a non-MemPerm object must use
//  an allocator in this interface.  To support this, NbTypes.h defines string
//  types (e.g. DynString, ProtString) which use these allocators.  The problem,
//  at least for strings, is that the string types in NbTypes.h have a different
//  type than std::string.  In some cases, this requires the use of .c_str() to
//  interwork between string types.  However, this is far less problematic than
//  using stringPtr, having the string's array reside in MemPerm, and having to
//  implement a Shutdown function that will also slow down restarts.
//
//  Threads are not subject to the above restrictions.  If a thread exits during
//  a restart, its destructor is invoked, which safely deletes any STL members.
//
namespace NodeBase
{
template< typename T > struct DynAllocator
{
   typedef T value_type;

   DynAllocator() { }

   ~DynAllocator() { }

   template< typename U > DynAllocator
      (const DynAllocator< U >& that) noexcept { }

   template< typename U > DynAllocator& operator=
      (const DynAllocator< U >& that) noexcept { }

   template< typename U > bool operator==
      (const DynAllocator< U >& that) const noexcept
   {
      return true;
   }

   template<typename U > bool operator!=
      (const DynAllocator< U >& that) const noexcept
   {
      return false;
   }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemDyn, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemDyn);
      if(addr == nullptr) throw AllocationException(MemDyn, n);
      return static_cast< T* >(addr);
   }

   void deallocate(T* const addr, size_t n) const noexcept
   {
      Memory::Free(addr);
   }
};

//------------------------------------------------------------------------------

template< typename T > struct ImmAllocator
{
   typedef T value_type;

   ImmAllocator() { }

   ~ImmAllocator() { }

   template< typename U > ImmAllocator
      (const ImmAllocator< U >& that) noexcept { }

   template< typename U > ImmAllocator& operator=
      (const ImmAllocator< U >& that) noexcept { }

   template< typename U > bool operator==
      (const ImmAllocator< U >& that) const noexcept
   {
      return true;
   }

   template<typename U > bool operator!=
      (const ImmAllocator< U >& that) const noexcept
   {
      return false;
   }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemImm, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemImm);
      if(addr == nullptr) throw AllocationException(MemImm, n);
      return static_cast< T* >(addr);
   }

   void deallocate(T* const addr, size_t n) const noexcept
   {
      Memory::Free(addr);
   }
};

//------------------------------------------------------------------------------

template< typename T > struct PermAllocator
{
   typedef T value_type;

   PermAllocator() { }

   ~PermAllocator() { }

   template< typename U > PermAllocator
      (const PermAllocator< U >& that) noexcept { }

   template< typename U > PermAllocator& operator=
      (const PermAllocator< U >& that) noexcept { }

   template< typename U > bool operator==
      (const PermAllocator< U >& that) const noexcept
   {
      return true;
   }

   template<typename U > bool operator!=
      (const PermAllocator< U >& that) const noexcept
   {
      return false;
   }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemPerm, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemPerm);
      if(addr == nullptr) throw AllocationException(MemPerm, n);
      return static_cast< T* >(addr);
   }

   void deallocate(T* const addr, size_t n) const noexcept
   {
      Memory::Free(addr);
   }
};

//------------------------------------------------------------------------------

template< typename T > struct ProtAllocator
{
   typedef T value_type;

   ProtAllocator() { }

   ~ProtAllocator() { }

   template< typename U > ProtAllocator
      (const ProtAllocator< U >& that) noexcept { }

   template< typename U > ProtAllocator& operator=
      (const ProtAllocator< U >& that) noexcept { }

   template< typename U > bool operator==
      (const ProtAllocator< U >& that) const noexcept
   {
      return true;
   }

   template<typename U > bool operator!=
      (const ProtAllocator< U >& that) const noexcept
   {
      return false;
   }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemProt, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemProt);
      if(addr == nullptr) throw AllocationException(MemProt, n);
      return static_cast< T* >(addr);
   }

   void deallocate(T* const addr, size_t n) const noexcept
   {
      Memory::Free(addr);
   }
};

//------------------------------------------------------------------------------

template< typename T > struct TempAllocator
{
   typedef T value_type;

   TempAllocator() { }

   ~TempAllocator() { }

   template< typename U > TempAllocator
      (const TempAllocator< U >& that) noexcept { }

   template< typename U > TempAllocator& operator=
      (const TempAllocator< U >& that) noexcept { }

   template< typename U > bool operator==
      (const TempAllocator< U >& that) const noexcept
   {
      return true;
   }

   template<typename U > bool operator!=
      (const TempAllocator< U >& that) const noexcept
   {
      return false;
   }

   T* allocate(size_t n) const
   {
      if(n == 0) return nullptr;
      if(n > SIZE_MAX / sizeof(T)) throw AllocationException(MemTemp, n);
      auto addr = Memory::Alloc(n * sizeof(T), MemTemp);
      if(addr == nullptr) throw AllocationException(MemTemp, n);
      return static_cast< T* >(addr);
   }

   void deallocate(T* const addr, size_t n) const noexcept
   {
      Memory::Free(addr);
   }
};
}
#endif