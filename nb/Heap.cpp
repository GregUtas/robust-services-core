//==============================================================================
//
//  Heap.cpp
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
#include "Heap.h"
#include <new>
#include <ostream>
#include <string>
#include "AllocationException.h"
#include "Debug.h"
#include "Restart.h"
#include "SysMemory.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Heap_ctor = "Heap.ctor";

Heap::Heap() :
   attrs_(MemReadWrite),
   inUse_(0),
   allocs_(0),
   fails_(0),
   frees_(0),
   maxInUse_(0),
   changes_(0)
{
   Debug::ft(Heap_ctor);
}

//------------------------------------------------------------------------------

fn_name Heap_dtor = "Heap.dtor";

Heap::~Heap()
{
   Debug::ftnt(Heap_dtor);
}

//------------------------------------------------------------------------------

fn_name Heap_Addr = "Heap.Addr";

void* Heap::Addr() const
{
   Debug::ft(Heap_Addr);

   Debug::SwLog(Heap_Addr, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Heap_Alloc= "Heap.Alloc";

void* Heap::Alloc(size_t size)
{
   Debug::ft(Heap_Alloc);

   Debug::SwLog(Heap_Alloc, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Heap_BlockToSize = "Heap.BlockToSize";

size_t Heap::BlockToSize(const void* addr) const
{
   Debug::ft(Heap_BlockToSize);

   Debug::SwLog(Heap_BlockToSize, strOver(this), 0);
   return 0;
}

//------------------------------------------------------------------------------

void Heap::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "attrs    : " << attrs_ << CRLF;
   stream << prefix << "inUse    : " << inUse_ << CRLF;
   stream << prefix << "allocs   : " << allocs_ << CRLF;
   stream << prefix << "fails    : " << fails_ << CRLF;
   stream << prefix << "frees    : " << frees_ << CRLF;
   stream << prefix << "maxInUse : " << maxInUse_ << CRLF;
   stream << prefix << "changes  : " << changes_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Heap_Free = "Heap.Free";

void Heap::Free(void* addr)
{
   Debug::ft(Heap_Free);

   Debug::SwLog(Heap_Free, strOver(this), 0);
}

//------------------------------------------------------------------------------

void Heap::Freed(size_t size)
{
   inUse_ -= size;
   ++frees_;
}

//------------------------------------------------------------------------------

bool Heap::IsFixedSize() const
{
   return (Size() != 0);
}

//------------------------------------------------------------------------------

fn_name Heap_delete = "Heap.operator delete";

void Heap::operator delete(void* addr)
{
   Debug::ftnt(Heap_delete);

   ::operator delete(addr);
}

//------------------------------------------------------------------------------

fn_name Heap_new = "Heap.operator new";

void* Heap::operator new(size_t size)
{
   Debug::ft(Heap_new);

   auto addr = ::operator new(size, std::nothrow);
   if(addr != nullptr) return addr;
   throw AllocationException(MemPermanent, size);
}

//------------------------------------------------------------------------------

void Heap::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Heap::Requested(size_t size, bool ok)
{
   if(ok)
   {
      inUse_ += size;
      if(inUse_ > maxInUse_) maxInUse_ = inUse_;
      ++allocs_;
   }
   else
   {
      ++fails_;
   }
}

//------------------------------------------------------------------------------

fn_name Heap_SetAttrs = "Heap.SetAttrs";

int Heap::SetAttrs(MemoryProtection attrs)
{
   Debug::ft(Heap_SetAttrs);

   if(attrs_ == attrs) return 0;
   attrs_ = attrs;
   ++changes_;
   return 0;
}

//------------------------------------------------------------------------------

fn_name Heap_SetPermissions = "Heap.SetPermissions";

int Heap::SetPermissions(MemoryProtection attrs)
{
   Debug::ft(Heap_SetPermissions);

   if(attrs_ == attrs) return 0;

   if(!IsFixedSize())
   {
      Debug::SwLog(Heap_SetPermissions, "heap size not fixed", 0);
      return 0x2000000;
   }

   auto err = SysMemory::Protect(Addr(), Size(), attrs);
   if(err == 0) return SetAttrs(attrs);

   Restart::Initiate(Restart::LevelToClear(Type()), HeapProtectionFailed, err);
   return err;
}

//------------------------------------------------------------------------------

fn_name Heap_Size= "Heap.Size";

size_t Heap::Size() const
{
   Debug::ft(Heap_Size);

   Debug::SwLog(Heap_Size, strOver(this), 0);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Heap_Type= "Heap.Type";

MemoryType Heap::Type() const
{
   Debug::ft(Heap_Type);

   Debug::SwLog(Heap_Type, strOver(this), 0);
   return MemNull;
}

//------------------------------------------------------------------------------

fn_name Heap_Validate = "Heap.Validate";

bool Heap::Validate(const void* addr) const
{
   Debug::ft(Heap_Validate);

   Debug::SwLog(Heap_Validate, strOver(this), 0);
   return false;
}
}
