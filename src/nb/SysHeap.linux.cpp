//==============================================================================
//
//  SysHeap.linux.cpp
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
#ifdef OS_LINUX

#include "SysHeap.h"
#include <cstdlib>
#include <errno.h>
#include <malloc.h>
#include <ostream>
#include <string>
#include "AllocationException.h"
#include "Debug.h"

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

   Debug::SwLog(SysHeap_ctor1, "not supported on Linux: use BuddyHeap", type);
   throw AllocationException(type, size);
}

//------------------------------------------------------------------------------

SysHeap::SysHeap() : Heap(),
   heap_(nullptr),
   size_(0),
   type_(MemPermanent)
{
   Debug::ftnt("SysHeap.ctor(wrap)");
}

//------------------------------------------------------------------------------

SysHeap::~SysHeap()
{
   Debug::ftnt("SysHeap.dtor");
}

//------------------------------------------------------------------------------

void* SysHeap::Alloc(size_t size)
{
   Debug::ft("SysHeap.Alloc");

   //  Linux doesn't provide a function for returning the size of block
   //  originally requested, so we can't distinguish between requested
   //  and actual sizes.  We therefore track actual sizes.
   //
   auto addr = malloc(size);
   if(addr != nullptr) size = malloc_usable_size(addr);
   Requested(size, addr);
   return addr;
}

//------------------------------------------------------------------------------

size_t SysHeap::BlockToSize(const void* addr) const
{
   Debug::ft("SysHeap.BlockToSize");

   //  Why isn't the parameter to malloc_usable_size a const void*?
   //
   return malloc_usable_size(const_cast<void*>(addr));
}

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

   //  Linux doesn't provide a function for returning the size of block
   //  originally requested, so we can't distinguish between requested
   //  and actual sizes.  We therefore track actual sizes.
   //
   if(addr == nullptr) return;

   auto size = malloc_usable_size(addr);
   if(size == 0)
   {
      Debug::SwLog(SysHeap_Free, "invalid address", uintptr_t(addr));
      return;
   }

   Freeing(addr, size);
   free(addr);
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

   static bool logged = false;

   if(!logged)
   {
      logged = true;
      Debug::SwLog(SysHeap_SetPermissions, "not supported: use BuddyHeap", 0);
   }

   return EPERM;
}

//------------------------------------------------------------------------------

bool SysHeap::Validate(const void* addr) const
{
   Debug::ft("SysHeap.Validate");

   //  UPDATE: The following discusses linking with -lmcheck.  However, this
   //  makes it unsafe to allocate memory from multiple threads without added
   //  synchronization.  Even if the correct synchronization--whatever it is--
   //  was added, being able to validate the heap seems unlikely to be worth
   //  the overhead that it would introduce.  For further information, see
   //  https://stackoverflow.com/questions/29894916
   //
   //  To validate the default help on Linux, mcheck() must have been called
   //  before malloc().  Because we allocate memory before entering main(),
   //  the only way to ensure this is to link using -lmcheck.  But doing so
   //  installs a nil handler.  This means that if mcheck_check_all() detects
   //  heap corruption, it invokes abort().  But we do catch SIGBART signals,
   //  so we should be OK.  Regardless, it probably won't be long until the
   //  system reboots--and maybe we should actually force one now, by setting
   //  a flag to let the signal handler know that, right now, a SIGABRT would
   //  signify heap corruption.
   //
   return true;
}
}
#endif
