//==============================================================================
//
//  SysMemory.win.cpp
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
#ifdef OS_WIN
#include <Windows.h>
#include "Debug.h"
#include "SysMemory.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
const DWORD PAGE_INVALID = 0;

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

const fn_name NodeBase_GetMemoryProtection = "NodeBase.GetMemoryProtection";

DWORD GetMemoryProtection(MemoryProtection attrs)
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

const fn_name SysMemory_Alloc = "SysMemory.Alloc";

void* SysMemory::Alloc(void* addr, size_t size, MemoryProtection attrs)
{
   Debug::ft(SysMemory_Alloc);

   auto mode = GetMemoryProtection(attrs);
   if(mode == PAGE_INVALID) return nullptr;

   addr = VirtualAlloc(nullptr, size, MEM_COMMIT, attrs);
   if(addr != nullptr) return addr;
   
   auto err = GetLastError();
   Debug::SwLog(SysMemory_Alloc, "failed to allocate memory", err);
   return nullptr;
}

//------------------------------------------------------------------------------

const fn_name SysMemory_Free = "SysMemory.Free";

bool SysMemory::Free(void* addr, size_t size)
{
   Debug::ft(SysMemory_Free);

   if(VirtualFree(addr, size, MEM_RELEASE)) return true;
   
   auto err = GetLastError();
   Debug::SwLog(SysMemory_Free, "failed to free memory", err);
   return nullptr;
}

//------------------------------------------------------------------------------

const fn_name SysMemory_Lock = "SysMemory.Lock";

bool SysMemory::Lock(void* addr, size_t size)
{
   Debug::ft(SysMemory_Lock);

   if(VirtualLock(addr, size)) return true;
   
   auto err = GetLastError();
   Debug::SwLog(SysMemory_Lock, "failed to lock memory", err);
   return false;
}

//------------------------------------------------------------------------------

const fn_name SysMemory_Protect = "SysMemory.Protect";

bool SysMemory::Protect(void* addr, size_t size, MemoryProtection attrs)
{
   Debug::ft(SysMemory_Protect);

   auto newMode = GetMemoryProtection(attrs);
   if(newMode == PAGE_INVALID) return nullptr;

   DWORD oldMode = 0;
   if(VirtualProtect(addr, size, newMode, &oldMode)) return true;

   auto err = GetLastError();
   Debug::SwLog(SysMemory_Protect, "failed to alter permissions", err);
   return false;
}

//------------------------------------------------------------------------------

const fn_name SysMemory_Unlock = "SysMemory.Unlock";

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