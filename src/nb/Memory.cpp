//==============================================================================
//
//  Memory.cpp
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
#include "Memory.h"
#include "NbHeap.h"
#include <cstring>
#include <iomanip>
#include <ostream>
#include <vector>
#include "AllocationException.h"
#include "Debug.h"
#include "HeapCfg.h"
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
   friend class Singleton<ImmutableHeap>;

   ImmutableHeap();
   ~ImmutableHeap();
};

class ProtectedHeap : public NbHeap
{
   friend class Singleton<ProtectedHeap>;

   ProtectedHeap();
   ~ProtectedHeap();
};

class PersistentHeap : public NbHeap
{
   friend class Singleton<PersistentHeap>;

   PersistentHeap();
   ~PersistentHeap();
};

class DynamicHeap : public NbHeap
{
   friend class Singleton<DynamicHeap>;

   DynamicHeap();
   ~DynamicHeap();
};

class TemporaryHeap : public NbHeap
{
   friend class Singleton<TemporaryHeap>;

   TemporaryHeap();
   ~TemporaryHeap();
};

//------------------------------------------------------------------------------

ImmutableHeap::ImmutableHeap() : NbHeap(MemImmutable)
{
   Debug::ft("ImmutableHeap.ctor");

   //> The size of the immutable heap must be defined at compile time.
   //
   Create(HeapCfg::SizeOfImmutableHeap);
}

//------------------------------------------------------------------------------

ImmutableHeap::~ImmutableHeap()
{
   Debug::ftnt("ImmutableHeap.dtor");
}

//------------------------------------------------------------------------------

ProtectedHeap::ProtectedHeap() : NbHeap(MemProtected)
{
   Debug::ft("ProtectedHeap.ctor");

   Create();
}

//------------------------------------------------------------------------------

ProtectedHeap::~ProtectedHeap()
{
   Debug::ftnt("ProtectedHeap.dtor");
}

//------------------------------------------------------------------------------

PersistentHeap::PersistentHeap() : NbHeap(MemPersistent)
{
   Debug::ft("PersistentHeap.ctor");

   Create();
}

//------------------------------------------------------------------------------

PersistentHeap::~PersistentHeap()
{
   Debug::ftnt("PersistentHeap.dtor");
}

//------------------------------------------------------------------------------

DynamicHeap::DynamicHeap() : NbHeap(MemDynamic)
{
   Debug::ft("DynamicHeap.ctor");

   Create();
}

//------------------------------------------------------------------------------

DynamicHeap::~DynamicHeap()
{
   Debug::ftnt("DynamicHeap.dtor");
}

//------------------------------------------------------------------------------

TemporaryHeap::TemporaryHeap() : NbHeap(MemTemporary)
{
   Debug::ft("TemporaryHeap.ctor");

   Create();
}

//------------------------------------------------------------------------------

TemporaryHeap::~TemporaryHeap()
{
   Debug::ftnt("TemporaryHeap.dtor");
}

//------------------------------------------------------------------------------
//
//  Returns the heap for TYPE.  If it doesn't exist, it is created.
//
static Heap* EnsureHeap(MemoryType type)
{
   switch(type)
   {
   case MemTemporary: return Singleton<TemporaryHeap>::Instance();
   case MemDynamic: return Singleton<DynamicHeap>::Instance();
   case MemPersistent: return Singleton<PersistentHeap>::Instance();
   case MemProtected: return Singleton<ProtectedHeap>::Instance();
   case MemPermanent: return PermanentHeap::Instance();
   case MemImmutable: return Singleton<ImmutableHeap>::Instance();
   }

   return nullptr;
}

//==============================================================================

Heap* Memory::AccessHeap(MemoryType type)
{
   switch(type)
   {
   case MemTemporary: return Singleton<TemporaryHeap>::Extant();
   case MemDynamic: return Singleton<DynamicHeap>::Extant();
   case MemPersistent: return Singleton<PersistentHeap>::Extant();
   case MemProtected: return Singleton<ProtectedHeap>::Extant();
   case MemPermanent: return PermanentHeap::Instance();
   case MemImmutable: return Singleton<ImmutableHeap>::Extant();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

const size_t PowerOf2[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

size_t Memory::Align(size_t size, size_t log2align)
{
   return (((size + PowerOf2[log2align] - 1) >> log2align) << log2align);
}

//------------------------------------------------------------------------------

void* Memory::Alloc(size_t size, MemoryType type)
{
   Debug::ft("Memory.Alloc");

   if(size == 0) return nullptr;

   //  Access the heap that manages the type of memory being requested.
   //
   auto heap = EnsureHeap(type);
   if(heap == nullptr)
   {
      throw AllocationException(type, size);
   }

   //  Align the size of the segment to the system's word size and ask
   //  the heap to allocate it.
   //
   auto gross = Align(size);
   auto addr = heap->Alloc(gross);
   if(addr == nullptr)
   {
      throw AllocationException(type, gross);
   }

   //  Record the size of the segment and its memory type.
   //
   if(Debug::TraceOn())
   {
      auto buff = Singleton<TraceBuffer>::Extant();

      if((buff != nullptr) && buff->ToolIsOn(MemoryTracer))
      {
         auto rec = new MemoryTrace(MemoryTrace::Alloc, addr, type, gross);
         buff->Insert(rec);
      }
   }

   return addr;
}

//------------------------------------------------------------------------------

void* Memory::Alloc(size_t size, MemoryType type, const std::nothrow_t&)
{
   Debug::ft("Memory.Alloc(nothrow)");

   if(size == 0) return nullptr;

   //  Access the heap that manages the type of memory being requested.
   //
   auto heap = AccessHeap(type);
   if(heap == nullptr) return nullptr;

   //  Align the size of the segment to the system's word size and ask
   //  the heap to allocate it.
   //
   auto gross = Align(size);
   auto addr = heap->Alloc(gross);
   if(addr == nullptr) return nullptr;

   //  Record the size of the segment and its memory type.
   //
   if(Debug::TraceOn())
   {
      auto buff = Singleton<TraceBuffer>::Extant();

      if((buff != nullptr) && buff->ToolIsOn(MemoryTracer))
      {
         auto rec = new MemoryTrace(MemoryTrace::Alloc, addr, type, gross);
         buff->Insert(rec);
      }
   }

   return addr;
}

//------------------------------------------------------------------------------

void Memory::Copy(void* dest, const void* source, size_t size)
{
   Debug::ft("Memory.Copy");

   memcpy(dest, source, size);
}

//------------------------------------------------------------------------------

fixed_string HeapHeader =
   "MemoryType   Max kB  Curr kB  Targ kB  Used kB  Free kB  Address";

void Memory::DisplayHeaps(ostream& stream, const string& prefix)
{
   std::vector<const Heap*> heaps;
   std::ostringstream expl;

   for(int m = MemTemporary; m < MemoryType_N; ++m)
   {
      heaps.push_back(AccessHeap(MemoryType(m)));
   }

   stream << prefix << HeapHeader << CRLF;
   auto config = Singleton<HeapCfg>::Instance();

   for(auto h = heaps.cbegin(); h != heaps.cend(); ++h)
   {
      auto type = (*h)->Type();

      stream << prefix << setw(10) << type;
      auto size = config->GetMaxSize(type);
      stream << setw(9) << (size / kBs);

      if(size > 0)
      {
         stream << setw(9) << (config->GetCurrSize(type) / kBs);
         stream << setw(9) << (config->GetTargSize(type) / kBs);
         stream << setw(9) << ((*h)->CurrInUse() / kBs);
         stream << setw(9) << ((*h)->CurrAvail() / kBs);
      }
      else
      {
         stream << setw(9) << "n/a";
         stream << setw(9) << "n/a";
         stream << setw(9) << ((*h)->CurrInUse() / kBs);
         stream << setw(9) << "n/a";
      }

      stream << setw(NIBBLES_PER_POINTER + 2) << *h << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name Memory_Free = "Memory.Free";

void Memory::Free(void* addr, MemoryType type)
{
   Debug::ftnt(Memory_Free);

   //  ADDR is where the application's data begins.  Access the header that
   //  precedes it in order to find the heap that owns the memory segment.
   //
   if(addr == nullptr) return;

   auto heap = AccessHeap(type);

   if(heap == nullptr)
   {
      Debug::SwLog(Memory_Free, "heap not found", type);
      return;
   }

   //  Free the memory segment.
   //
   if(Debug::TraceOn())
   {
      auto buff = Singleton<TraceBuffer>::Extant();

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

void Memory::Set(void* dest, byte_t value, size_t size)
{
   Debug::ft("Memory.Set");

   memset(dest, value, size);
}

//------------------------------------------------------------------------------

void Memory::Shutdown()
{
   Debug::ft("Memory.Shutdown");

   if(Restart::ClearsMemory(MemTemporary))
   {
      Singleton<TemporaryHeap>::Destroy();
   }

   if(Restart::ClearsMemory(MemDynamic))
   {
      Singleton<DynamicHeap>::Destroy();
   }

   if(Restart::ClearsMemory(MemPersistent))
   {
      Singleton<PersistentHeap>::Destroy();
   }

   if(Restart::ClearsMemory(MemProtected))
   {
      Unprotect(MemProtected);
      Singleton<ProtectedHeap>::Destroy();
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

int Memory::Validate(MemoryType type, const void* addr)
{
   Debug::ft("Memory.Validate");

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
