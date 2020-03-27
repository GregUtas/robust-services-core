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
#include "SysHeap.h"
#include <cstring>
#include "Algorithms.h"
#include "AllocationException.h"
#include "Debug.h"
#include "MemoryTrace.h"
#include "PermanentHeap.h"
#include "Singleton.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class ImmutableHeap : public SysHeap
{
   friend class Singleton< ImmutableHeap >;
private:
   ImmutableHeap();
   ~ImmutableHeap();
};

class ProtectedHeap : public SysHeap
{
   friend class Singleton< ProtectedHeap >;
private:
   ProtectedHeap();
   ~ProtectedHeap();
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

ImmutableHeap::ImmutableHeap() : SysHeap(MemImm, 0)
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

ProtectedHeap::ProtectedHeap() : SysHeap(MemProt, 0)
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

fn_name DynamicHeap_ctor = "DynamicHeap.ctor";

DynamicHeap::DynamicHeap() : SysHeap(MemDyn, 0)
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

TemporaryHeap::TemporaryHeap() : SysHeap(MemTemp, 0)
{
   Debug::ft(TemporaryHeap_ctor);
}

//------------------------------------------------------------------------------

fn_name TemporaryHeap_dtor = "TemporaryHeap.dtor";

TemporaryHeap::~TemporaryHeap()
{
   Debug::ft(TemporaryHeap_dtor);
}

//==============================================================================
//
//  Each memory segment allocated from a heap has the following header.
//
struct SegmentHeader
{
   size_t size;      // size of data (see below)
   MemoryType type;  // type of memory

   static size_t Size();
};

size_t SegmentHeader::Size()
{
   static size_t size = Memory::Align(sizeof(SegmentHeader));
   return size;
}

//  This struct references a memory segment's header and the location
//  where the client's data begins.
//
struct Segment
{
   SegmentHeader header;  // memory management information
   uword data;            // start of application data
};

//------------------------------------------------------------------------------

SysHeap* Memory::AccessHeap(MemoryType type)
{
   switch(type)
   {
   case MemTemp: return Singleton< TemporaryHeap >::Extant();
   case MemDyn: return Singleton< DynamicHeap >::Extant();
   case MemProt: return Singleton< ProtectedHeap >::Extant();
   case MemPerm: return PermanentHeap::Instance();
   case MemImm: return Singleton< ImmutableHeap >::Extant();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

const size_t PowerOf2[] = {1, 2, 4, 8, 16, 32, 64, 128};

size_t Memory::Align(size_t size, size_t log2align)
{
   return (((size + PowerOf2[log2align] - 1) >> log2align) << log2align);
}

//------------------------------------------------------------------------------

fn_name Memory_Alloc = "Memory.Alloc";

void* Memory::Alloc(size_t nBytes, MemoryType type, bool ex)
{
   Debug::ft(Memory_Alloc);

   if(nBytes == 0) return nullptr;

   //  Access the heap that manages the type of memory being requested.
   //
   auto heap = EnsureHeap(type);
   if(heap == nullptr)
   {
      if(!ex) return nullptr;
      throw AllocationException(type, nBytes);
   }

   //  Align the size of the segment to the system's word size and ask
   //  the heap to allocate it.
   //
   auto size = Align(nBytes);
   auto addr = heap->Alloc(SegmentHeader::Size() + size);
   if(addr == nullptr)
   {
      if(!ex) return nullptr;
      throw AllocationException(type, SegmentHeader::Size() + size);
   }

   //  Success.  Record the size of the segment (excluding its header)
   //  and its memory type.
   //
   auto seg = (Segment*) addr;
   seg->header.size = size;
   seg->header.type = type;

   if(Debug::TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Extant();

      if((buff != nullptr) && buff->ToolIsOn(MemoryTracer))
      {
         auto rec = new MemoryTrace(MemoryTrace::Alloc, &seg->data, type, size);
         buff->Insert(rec);
      }
   }

   return &seg->data;
}

//------------------------------------------------------------------------------

fn_name Memory_Copy = "Memory.Copy";

void Memory::Copy(void* dest, const void* source, size_t nBytes)
{
   Debug::ft(Memory_Copy);

   memcpy(dest, source, nBytes);
}

//------------------------------------------------------------------------------

SysHeap* Memory::EnsureHeap(MemoryType type)
{
   switch(type)
   {
   case MemTemp: return Singleton< TemporaryHeap >::Instance();
   case MemDyn: return Singleton< DynamicHeap >::Instance();
   case MemProt: return Singleton< ProtectedHeap >::Instance();
   case MemPerm: return PermanentHeap::Instance();
   case MemImm: return Singleton< ImmutableHeap >::Instance();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Memory_Free = "Memory.Free";

void Memory::Free(const void* addr)
{
   Debug::ft(Memory_Free);

   //  ADDR is where the application's data begins.  Access the header that
   //  precedes it in order to find the heap that owns the memory segment.
   //
   if(addr == nullptr) return;

   auto segment = (Segment*) getptr1(addr, SegmentHeader::Size());
   auto header = &segment->header;
   auto heap = AccessHeap(header->type);

   if(heap == nullptr)
   {
      Debug::SwLog(Memory_Free, header->size, header->type);
      return;
   }

   //  Free the memory segment.  Its actual size includes the header.
   //
   if(Debug::TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Extant();

      if((buff != nullptr) && buff->ToolIsOn(MemoryTracer))
      {
         auto rec = new MemoryTrace(MemoryTrace::Free,
            addr, header->type, header->size);
         buff->Insert(rec);
      }
   }

   heap->Free(segment, SegmentHeader::Size() + header->size);
}

//------------------------------------------------------------------------------

const SysHeap* Memory::Heap(MemoryType type)
{
   return AccessHeap(type);
}

//------------------------------------------------------------------------------

fn_name Memory_Realloc = "Memory.Realloc";

void* Memory::Realloc(void* addr, size_t nBytes)
{
   Debug::ft(Memory_Realloc);

   //  ADDR is where the application's data begins.  Access the header that
   //  precedes it.  Return the current segment if its size is sufficient.
   //  Otherwise, allocate a new segment, copy the data to it, and free the
   //  original segment.
   //
   if(addr == nullptr)
   {
      Debug::SwLog(Memory_Realloc, "null address", nBytes);
      return nullptr;
   }

   auto source = (Segment*) getptr1(addr, SegmentHeader::Size());
   if(source->header.size >= Align(nBytes)) return addr;

   auto dest = Alloc(nBytes, source->header.type);
   if(dest == nullptr) return nullptr;
   Copy(dest, &source->data, source->header.size);
   Free(addr);
   return dest;
}

//------------------------------------------------------------------------------

fn_name Memory_Set = "Memory.Set";

void Memory::Set(void* dest, byte_t value, size_t nBytes)
{
   Debug::ft(Memory_Set);

   memset(dest, value, nBytes);
}

//------------------------------------------------------------------------------

fn_name Memory_Shutdown = "Memory.Shutdown";

void Memory::Shutdown(RestartLevel level)
{
   Debug::ft(Memory_Shutdown);

   if(level < RestartWarm) return;
   Singleton< TemporaryHeap >::Destroy();
   if(level < RestartCold) return;
   Singleton< DynamicHeap >::Destroy();
   if(level < RestartReload) return;
   Singleton< ProtectedHeap >::Destroy();
}

//------------------------------------------------------------------------------

fn_name Memory_Type = "Memory.Type";

MemoryType Memory::Type(const void* addr)
{
   Debug::ft(Memory_Type);

   if(addr == nullptr)
   {
      Debug::SwLog(Memory_Type, "null address", 0);
      return MemNull;
   }

   auto source = (Segment*) getptr1(addr, SegmentHeader::Size());
   return source->header.type;
}

//------------------------------------------------------------------------------

fn_name Memory_Verify = "Memory.Verify";

bool Memory::Verify(MemoryType type, const void* addr)
{
   Debug::ft(Memory_Verify);

   auto heap = AccessHeap(type);
   if(heap == nullptr) return true;
   return heap->Validate(addr);
}

//------------------------------------------------------------------------------

size_t Memory::Words(size_t nBytes)
{
   return ((nBytes + BYTES_PER_WORD - 1) >> BYTES_PER_WORD_LOG2);
}
}
