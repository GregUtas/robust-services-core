//==============================================================================
//
//  SysMemory.win.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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

#include "SysMemory.h"
#include <Windows.h>
#include "Debug.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
constexpr DWORD PAGE_INVALID = 0;

const DWORD PermissionToProtection[MemoryProtection_N] =
{
   PAGE_NOACCESS,          // MemInaccessible
   PAGE_EXECUTE,           // MemExecuteOnly
   PAGE_INVALID,           // hypothetical MemWriteOnly
   PAGE_INVALID,           // hypothetical MemWriteExecute
   PAGE_READONLY,          // MemReadOnly
   PAGE_EXECUTE_READ,      // MemReadExecute
   PAGE_READWRITE,         // MemReadWrite
   PAGE_EXECUTE_READWRITE  // MemReadWriteExecute
};

//------------------------------------------------------------------------------

fn_name NodeBase_GetMemoryProtection = "NodeBase.GetMemoryProtection";

static DWORD GetMemoryProtection(MemoryProtection attrs)
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

   auto mode = GetMemoryProtection(attrs);
   if(mode == PAGE_INVALID) return nullptr;

   addr = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE, mode);
   if(addr != nullptr) return addr;

   auto err = GetLastError();
   Debug::SwLog(SysMemory_Alloc, "failed to allocate memory", err);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Free = "SysMemory.Free";

bool SysMemory::Free(void* addr)
{
   Debug::ft(SysMemory_Free);

   if(VirtualFree(addr, 0, MEM_RELEASE)) return true;

   auto err = GetLastError();
   Debug::SwLog(SysMemory_Free, "failed to free memory", err);
   return false;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Lock = "SysMemory.Lock";

bool SysMemory::Lock(void* addr, size_t size)
{
   Debug::ft(SysMemory_Lock);

   if(VirtualLock(addr, size)) return true;

   auto err = GetLastError();
   Debug::SwLog(SysMemory_Lock, "failed to lock memory", err);
   return false;
}

//------------------------------------------------------------------------------

int SysMemory::Protect(void* addr, size_t size, MemoryProtection attrs)
{
   Debug::ft("SysMemory.Protect");

   auto newMode = GetMemoryProtection(attrs);
   if(newMode == PAGE_INVALID) return ERROR_INVALID_PARAMETER;

   DWORD oldMode = 0;
   if(VirtualProtect(addr, size, newMode, &oldMode)) return 0;

   auto err = GetLastError();
   return err;
}

//------------------------------------------------------------------------------

fn_name SysMemory_Unlock = "SysMemory.Unlock";

bool SysMemory::Unlock(void* addr, size_t size)
{
   Debug::ft(SysMemory_Unlock);

   if(VirtualUnlock(addr, size)) return true;

   auto err = GetLastError();
   Debug::SwLog(SysMemory_Unlock, "failed to unlock memory", err);
   return false;
}
}
#endif
