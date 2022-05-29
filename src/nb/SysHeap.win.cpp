//==============================================================================
//
//  SysHeap.win.cpp
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
#ifdef OS_WIN

#include "SysHeap.h"
#include <cstdint>
#include <ostream>
#include <string>
#include <Windows.h>
#include "Debug.h"
#include "Restart.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysHeap_ctor1 = "SysHeap.ctor";

SysHeap::SysHeap(MemoryType type, size_t size) : Heap(),
   heap_(nullptr),
   size_(size),
   type_(type)
{
   Debug::ft(SysHeap_ctor1);

   if(type == MemPermanent)
   {
      Debug::SwLog(SysHeap_ctor1, "use default constructor", type);
      return;
   }

   //  Linux doesn't support custom heaps, so all heaps other than
   //  the default heap now use BuddyHeap.  This code is therefore no
   //  longer used, but it has been kept in case Linux eventually
   //  supports custom heaps.
   //
   heap_ = HeapCreate(0, size, size);

   if(heap_ == nullptr)
   {
      Restart::Initiate(RestartWarm, HeapCreationFailed, type);
   }
}

//------------------------------------------------------------------------------

SysHeap::SysHeap() : Heap(),
   heap_(GetProcessHeap()),
   size_(0),
   type_(MemPermanent)
{
   Debug::ftnt("SysHeap.ctor(wrap)");
}

//------------------------------------------------------------------------------

fn_name SysHeap_dtor = "SysHeap.dtor";

SysHeap::~SysHeap()
{
   Debug::ftnt(SysHeap_dtor);

   //  If there's no actual heap, we're done.
   //
   if(heap_ == nullptr) return;

   //  Prevent an attempt to destroy the C++ default heap.
   //
   if(heap_ == GetProcessHeap())
   {
      Debug::SwLog(SysHeap_dtor, "tried to free default heap", 0);
      return;
   }

   if(!HeapDestroy(heap_))
   {
      Debug::SwLog(SysHeap_dtor, "heap not freed", GetLastError());
   }

   heap_ = nullptr;
}

//------------------------------------------------------------------------------

void* SysHeap::Addr() const
{
   return heap_;
}

//------------------------------------------------------------------------------

void* SysHeap::Alloc(size_t size)
{
   Debug::ft("SysHeap.Alloc");

   if(heap_ == nullptr) return nullptr;

   //  Windows doesn't provide a function that returns the actual
   //  size of a block, so we can only track requested sizes.
   //
   auto addr = HeapAlloc(heap_, 0, size);
   Requested(size, addr);
   return addr;
}

//------------------------------------------------------------------------------

size_t SysHeap::BlockToSize(const void* addr) const
{
   Debug::ft("SysHeap.BlockToSize");

   if(heap_ == nullptr) return 0;
   auto size = HeapSize(heap_, 0, addr);
   if(size == (SIZE_MAX - 1)) size = 0;
   return size;
}

//------------------------------------------------------------------------------

bool SysHeap::CanBeProtected() const { return false; }

//------------------------------------------------------------------------------

size_t SysHeap::CurrAvail() const
{
   Debug::ft("SysHeap.CurrAvail");

   return 0;
}

//------------------------------------------------------------------------------

void SysHeap::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Heap::Display(stream, prefix, options);

   stream << prefix << "heap : " << heap_ << CRLF;
   stream << prefix << "size : " << size_ << CRLF;
   stream << prefix << "type : " << type_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name SysHeap_Free = "SysHeap.Free";

void SysHeap::Free(void* addr)
{
   Debug::ft(SysHeap_Free);

   if(heap_ == nullptr) return;
   if(addr == nullptr) return;

   //  Windows doesn't provide a function that returns the actual
   //  size of a block, so we can only track requested sizes.
   //
   auto size = BlockToSize(addr);
   Freeing(addr, size);

   if(!HeapFree(heap_, 0, addr))
   {
      Debug::SwLog(SysHeap_Free, "HeapFree failed", GetLastError());
   }
}

//------------------------------------------------------------------------------

size_t SysHeap::Overhead() const { return 0; }

//------------------------------------------------------------------------------

void SysHeap::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SysHeap_SetPermissions = "SysHeap.SetPermissions";

int SysHeap::SetPermissions(MemoryProtection attrs)
{
   Debug::ft(SysHeap_SetPermissions);

   Debug::SwLog(SysHeap_SetPermissions, "not supported: use BuddyHeap", 0);
   return ERROR_NOT_SUPPORTED;
}

//------------------------------------------------------------------------------

bool SysHeap::Validate(const void* addr) const
{
   Debug::ft("SysHeap.Validate");

   if(heap_ == nullptr) return true;
   return HeapValidate(heap_, 0, addr);
}
}
#endif
