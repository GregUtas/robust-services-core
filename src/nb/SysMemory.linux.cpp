//==============================================================================
//
//  SysMemory.linux.cpp
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

#include "SysMemory.h"
#include <errno.h>
#include <sys/mman.h>
#include "Debug.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
constexpr int PAGE_INVALID = 0;

const int PermissionToProtection[MemoryProtection_N] =
{
   PROT_NONE,                          // MemInaccessible
   PROT_EXEC,                          // MemExecuteOnly
   PAGE_INVALID,                       // hypothetical MemWriteOnly
   PAGE_INVALID,                       // hypothetical MemWriteExecute
   PROT_READ,                          // MemReadOnly
   PROT_READ | PROT_EXEC,              // MemReadExecute
   PROT_READ | PROT_WRITE,             // MemReadWrite
   PROT_READ | PROT_WRITE | PROT_EXEC  // MemReadWriteExecute
};

//------------------------------------------------------------------------------

fn_name NodeBase_GetMemoryProtection = "NodeBase.GetMemoryProtection";

static int GetMemoryProtection(MemoryProtection attrs)
{
   if((attrs < 0) || (attrs >= MemoryProtection_N))
   {
      Debug::SwLog(NodeBase_GetMemoryProtection, "invalid permissions", attrs);
      return PAGE_INVALID;
   }

   auto mode = PermissionToProtection[attrs];

   if(mode == PAGE_INVALID)
   {
      Debug::SwLog(NodeBase_GetMemoryProtection, "invalid permissions", attrs);
   }

   return mode;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Alloc = "SysMemory.Alloc";

void* SysMemory::Alloc(void* addr, size_t size, MemoryProtection attrs)
{
   Debug::ft(SysMemory_Alloc);

   auto prot = GetMemoryProtection(attrs);
   if(prot == PAGE_INVALID) return nullptr;

   addr = mmap(nullptr, size, prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
   if(addr != nullptr) return addr;

   Debug::SwLog(SysMemory_Alloc, "failed to allocate memory", errno);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Free = "SysMemory.Free";

bool SysMemory::Free(void* addr, size_t size)
{
   Debug::ft(SysMemory_Free);

   if(munmap(addr, size) == 0) return true;

   Debug::SwLog(SysMemory_Free, "failed to free memory", errno);
   return false;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Lock = "SysMemory.Lock";

bool SysMemory::Lock(void* addr, size_t size)
{
   Debug::ft(SysMemory_Lock);

   if(mlock(addr, size) == 0) return true;

   Debug::SwLog(SysMemory_Lock, "failed to lock memory", errno);
   return false;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Protect = "SysMemory.Protect";

int SysMemory::Protect(void* addr, size_t size, MemoryProtection attrs)
{
   Debug::ft(SysMemory_Protect);

   auto prot = GetMemoryProtection(attrs);
   if(prot == PAGE_INVALID) return EINVAL;

   if(mprotect(addr, size, prot) == 0) return 0;

   Debug::SwLog(SysMemory_Protect, "failed to change permissions", errno);
   return errno;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Unlock = "SysMemory.Unlock";

bool SysMemory::Unlock(void* addr, size_t size)
{
   Debug::ft(SysMemory_Unlock);

   if(munlock(addr, size) == 0) return true;

   Debug::SwLog(SysMemory_Unlock, "failed to unlock memory", errno);
   return false;
}
}
#endif
