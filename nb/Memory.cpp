//==============================================================================
//
//  Memory.cpp
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
#include "Memory.h"
#include "NbHeap.h"
#include "SysHeap.h"
#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>
#include "AllocationException.h"
#include "Debug.h"
#include "Formatters.h"
#include "MainArgs.h"
#include "MemoryTrace.h"
#include "PermanentHeap.h"
#include "Restart.h"
#include "Singleton.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
class ImmutableHeap : public NbHeap
{
   friend class Singleton< ImmutableHeap >;
private:
   ImmutableHeap();
   ~ImmutableHeap();

   //  The size of the immutable heap must be defined at compile
   //  time because it is created even before main() is entered.
   //
   static const size_t Size_ = 512 * kBs;
};

class ProtectedHeap : public NbHeap
{
   friend class Singleton< ProtectedHeap >;
private:
   ProtectedHeap();
   ~ProtectedHeap();

   //  The number of kBs in the protected heap may be defined by a
   //  command line parameter prefixed by "Prot_kBs=".  Its value
   //  may range from 1MB to 512Mb (32-bit CPU) or 8GB (64-bit CPU).
   //
   static size_t GetSize();
   const static size_t MinSize = 1 * MBs;
   const static size_t MaxSize = size_t(1) << (25 + BYTES_PER_WORD);
};

class PersistentHeap : public SysHeap
{
   friend class Singleton< PersistentHeap >;
private:
   PersistentHeap();
   ~PersistentHeap();
};

class DynamicHeap : public SysHeap
{
   friend class Singleton< DynamicHeap >;
private:
   DynamicHeap();
   ~DynamicHeap();
};

class TemporaryHeap : public SysHeap
{
   friend class Singleton< TemporaryHeap >;
private:
   TemporaryHeap();
   ~TemporaryHeap();
};

//------------------------------------------------------------------------------

fn_name ImmutableHeap_ctor = "ImmutableHeap.ctor";

ImmutableHeap::ImmutableHeap() : NbHeap(MemImmutable, Size_)
{
   Debug::ft(ImmutableHeap_ctor);
}

//------------------------------------------------------------------------------

fn_name ImmutableHeap_dtor = "ImmutableHeap.dtor";

ImmutableHeap::~ImmutableHeap()
{
   Debug::ft(ImmutableHeap_dtor);
}

//------------------------------------------------------------------------------

fn_name ProtectedHeap_ctor = "ProtectedHeap.ctor";

ProtectedHeap::ProtectedHeap() : NbHeap(MemProtected, GetSize())
{
   Debug::ft(ProtectedHeap_ctor);
}

//------------------------------------------------------------------------------

fn_name ProtectedHeap_dtor = "ProtectedHeap.dtor";

ProtectedHeap::~ProtectedHeap()
{
   Debug::ft(ProtectedHeap_dtor);
}

//------------------------------------------------------------------------------

fn_name ProtectedHeap_GetSize = "ProtectedHeap.GetSize";

size_t ProtectedHeap::GetSize()
{
   Debug::ft(ProtectedHeap_GetSize);

   size_t size;

   auto str = MainArgs::Find("Prot_kBs=");
   if(!strToSize(str, size)) return MinSize;

   size *= kBs;

   if(size < MinSize)
      size = MinSize;
   else if(size > MaxSize)
      size = MaxSize;

   return size;
}

//------------------------------------------------------------------------------

fn_name PersistentHeap_ctor = "PersistentHeap.ctor";

PersistentHeap::PersistentHeap() : SysHeap(MemPersistent, 0)
{
   Debug::ft(PersistentHeap_ctor);
}

//------------------------------------------------------------------------------

fn_name PersistentHeap_dtor = "PersistentHeap.dtor";

PersistentHeap::~PersistentHeap()
{
   Debug::ft(PersistentHeap_dtor);
}

//------------------------------------------------------------------------------

fn_name DynamicHeap_ctor = "DynamicHeap.ctor";

DynamicHeap::DynamicHeap() : SysHeap(MemDynamic, 0)
{
   Debug::ft(DynamicHeap_ctor);
}

//------------------------------------------------------------------------------

fn_name DynamicHeap_dtor = "DynamicHeap.dtor";

DynamicHeap::~DynamicHeap()
{
   Debug::ft(DynamicHeap_dtor);
}

//------------------------------------------------------------------------------

fn_name TemporaryHeap_ctor = "TemporaryHeap.ctor";

TemporaryHeap::TemporaryHeap() : SysHeap(MemTemporary, 0)
{
   Debug::ft(TemporaryHeap_ctor);
}

//------------------------------------------------------------------------------

fn_name TemporaryHeap_dtor = "TemporaryHeap.dtor";

TemporaryHeap::~TemporaryHeap()
{
   Debug::ft(TemporaryHeap_dtor);
}

//------------------------------------------------------------------------------
//
//  Returns the heap (if any) associated with TYPE.
//
Heap* AccessHeap(MemoryType type)
{
   switch(type)
   {
   case MemTemporary: return Singleton< TemporaryHeap >::Extant();
   case MemDynamic: return Singleton< DynamicHeap >::Extant();
   case MemPersistent: return Singleton< PersistentHeap >::Extant();
   case MemProtected: return Singleton< ProtectedHeap >::Extant();
   case MemPermanent: return PermanentHeap::Instance();
   case MemImmutable: return Singleton< ImmutableHeap >::Extant();
   }

   return nullptr;
}

//------------------------------------------------------------------------------
//
//  Returns the heap for TYPE.  If it doesn't exist, it is created.
//
Heap* EnsureHeap(MemoryType type)
{
   switch(type)
   {
   case MemTemporary: return Singleton< TemporaryHeap >::Instance();
   case MemDynamic: return Singleton< DynamicHeap >::Instance();
   case MemPersistent: return Singleton< PersistentHeap >::Instance();
   case MemProtected: return Singleton< ProtectedHeap >::Instance();
   case MemPermanent: return PermanentHeap::Instance();
   case MemImmutable: return Singleton< ImmutableHeap >::Instance();
   }

   return nullptr;
}

//==============================================================================

MemoryType Memory::AddrToType(const void* addr)
{
   for(auto m = 1; m < MemoryType_N; ++m)
   {
      auto type = MemoryType(m);
      auto heap = AccessHeap(type);
      if(heap == nullptr) continue;
      if(heap->Addr() == addr) return type;
   }

   return MemNull;
}

//------------------------------------------------------------------------------

const size_t PowerOf2[] = {1, 2, 4, 8, 16, 32, 64, 128};

size_t Memory::Align(size_t size, size_t log2align)
{
   return (((size + PowerOf2[log2align] - 1) >> log2align) << log2align);
}

//------------------------------------------------------------------------------

fn_name Memory_Alloc = "Memory.Alloc";

void* Memory::Alloc(size_t size, MemoryType type, bool ex)
{
   Debug::ft(Memory_Alloc);

   if(size == 0) return nullptr;

   //  Access the heap that manages the type of memory being requested.
   //
   auto heap = EnsureHeap(type);
   if(heap == nullptr)
   {
      if(!ex) return nullptr;
      throw AllocationException(type, size);
   }

   //  Align the size of the segment to the system's word size and ask
   //  the heap to allocate it.
   //
   auto gross = Align(size);
   auto addr = heap->Alloc(gross);
   if(addr == nullptr)
   {
      if(!ex) return nullptr;
      throw AllocationException(type, gross);
   }

   //  Success.  Record the size of the segment (excluding its header)
   //  and its memory type.
   //
   if(Debug::TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Extant();

      if((buff != nullptr) && buff->ToolIsOn(MemoryTracer))
      {
         auto rec = new MemoryTrace(MemoryTrace::Alloc, addr, type, gross);
         buff->Insert(rec);
      }
   }

   return addr;
}

//------------------------------------------------------------------------------

fn_name Memory_Copy = "Memory.Copy";

void Memory::Copy(void* dest, const void* source, size_t size)
{
   Debug::ft(Memory_Copy);

   memcpy(dest, source, size);
}

//------------------------------------------------------------------------------

void Memory::DisplayHeaps(ostream& stream, const string& prefix)
{
   std::set< void* > heaps;
   std::ostringstream expl;

   SysHeap::ListHeaps(heaps, expl);
   const Heap* heap = PermanentHeap::Instance();
   if(heap != nullptr) heaps.insert(heap->Addr());
   heap = Singleton< ImmutableHeap >::Extant();
   if(heap != nullptr) heaps.insert(heap->Addr());
   heap = Singleton< ProtectedHeap >::Extant();
   if(heap != nullptr) heaps.insert(heap->Addr());
   heap = Singleton< PersistentHeap >::Extant();
   if(heap != nullptr) heaps.insert(heap->Addr());
   heap = Singleton< DynamicHeap >::Extant();
   if(heap != nullptr) heaps.insert(heap->Addr());
   heap = Singleton< TemporaryHeap >::Extant();
   if(heap != nullptr) heaps.insert(heap->Addr());

   stream << prefix << "Heap  MemoryType  Address" << CRLF;
   size_t index = 1;

   for(auto h = heaps.cbegin(); h != heaps.cend(); ++h)
   {
      auto type = Memory::AddrToType(*h);

      stream << prefix << setw(4) << index++;

      if(type != MemNull)
         stream << setw(12) << type;
      else
         stream << setw(12) << "unknown";

      stream << setw(NIBBLES_PER_POINTER + 2) << *h << CRLF;
   }

   if(!expl.str().empty())
   {
      stream << CRLF;
      stream << prefix << "Problem while querying system heaps: " << CRLF;
      stream << prefix << spaces(2) << expl.str() << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name Memory_Free = "Memory.Free";

void Memory::Free(void* addr, MemoryType type)
{
   Debug::ft(Memory_Free);

   //  ADDR is where the application's data begins.  Access the header that
   //  precedes it in order to find the heap that owns the memory segment.
   //
   if(addr == nullptr) return;

   auto heap = AccessHeap(type);

   if(heap == nullptr)
   {
      Debug::SwLog(Memory_Free, 0, type);
      return;
   }

   //  Free the memory segment.  Its actual size includes the header.
   //
   if(Debug::TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Extant();

      if((buff != nullptr) && buff->ToolIsOn(MemoryTracer))
      {
         auto size = heap->BlockToSize(addr);
         auto rec = new MemoryTrace(MemoryTrace::Free, addr, type, size);
         buff->Insert(rec);
      }
   }

   heap->Free(addr);
}

//------------------------------------------------------------------------------

const Heap* Memory::GetHeap(MemoryType type)
{
   return AccessHeap(type);
}

//------------------------------------------------------------------------------

fn_name Memory_Protect = "Memory.Protect";

bool Memory::Protect(MemoryType type)
{
   switch(type)
   {
   case MemProtected:
   case MemImmutable:
      break;
   default:
      Debug::SwLog(Memory_Protect, "invalid memory type", type);
      return false;
   }

   auto heap = AccessHeap(type);
   if(heap == nullptr) return false;
   if(!heap->CanBeProtected()) return true;
   return (heap->SetPermissions(MemReadOnly) == 0);
}

//------------------------------------------------------------------------------

fn_name Memory_Realloc = "Memory.Realloc";

void* Memory::Realloc(void* addr, size_t size, MemoryType type)
{
   Debug::ft(Memory_Realloc);

   //  ADDR is where the application's data begins.  Allocate a new
   //  segment, copy the data to it, and free the original segment.
   //
   if(addr == nullptr)
   {
      Debug::SwLog(Memory_Realloc, "null address", size);
      return nullptr;
   }

   auto dest = Alloc(size, type);
   if(dest == nullptr) return nullptr;
   Copy(dest, addr, size);
   Free(addr, type);
   return dest;
}

//------------------------------------------------------------------------------

fn_name Memory_Set = "Memory.Set";

void Memory::Set(void* dest, byte_t value, size_t size)
{
   Debug::ft(Memory_Set);

   memset(dest, value, size);
}

//------------------------------------------------------------------------------

fn_name Memory_Shutdown = "Memory.Shutdown";

void Memory::Shutdown()
{
   Debug::ft(Memory_Shutdown);

   if(Restart::ClearsMemory(MemTemporary))
   {
      Singleton< TemporaryHeap >::Destroy();
   }

   if(Restart::ClearsMemory(MemDynamic))
   {
      Singleton< DynamicHeap >::Destroy();
   }

   if(Restart::ClearsMemory(MemPersistent))
   {
      Singleton< PersistentHeap >::Destroy();
   }

   if(Restart::ClearsMemory(MemProtected))
   {
      Unprotect(MemProtected);
      Singleton< ProtectedHeap >::Destroy();
   }
}

//------------------------------------------------------------------------------

bool Memory::Unprotect(MemoryType type)
{
   switch(type)
   {
   case MemProtected:
   case MemImmutable:
      break;
   default:
      Debug::SwLog(Memory_Protect, "invalid memory type", type);
      return true;
   }

   auto heap = AccessHeap(type);
   if(heap == nullptr) return false;
   if(!heap->CanBeProtected()) return true;
   return (heap->SetPermissions(MemReadWrite) == 0);
}

//------------------------------------------------------------------------------

fn_name Memory_Validate = "Memory.Validate";

int Memory::Validate(MemoryType type, const void* addr)
{
   Debug::ft(Memory_Validate);

   auto heap = AccessHeap(type);
   if(heap == nullptr) return -1;
   return (heap->Validate(addr) ? 1 : 0);
}

//------------------------------------------------------------------------------

size_t Memory::Words(size_t size)
{
   return ((size + BYTES_PER_WORD - 1) >> BYTES_PER_WORD_LOG2);
}
}
