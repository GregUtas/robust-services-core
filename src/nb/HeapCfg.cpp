//==============================================================================
//
//  HeapCfg.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "HeapCfg.h"
#include "Debug.h"
#include "FunctionGuard.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
HeapCfg::HeapCfg() :
   minSize_{0},
   maxSize_{0},
   currSize_{0},
   targSize_{0}
{
   Debug::ft("HeapCfg.ctor");

   //> The size of the immutable heap is fixed at 512 kB.
   //
   minSize_[MemImmutable] = SizeOfImmutableHeap;
   maxSize_[MemImmutable] = SizeOfImmutableHeap;
   currSize_[MemImmutable] = SizeOfImmutableHeap;
   targSize_[MemImmutable] = SizeOfImmutableHeap;

   //> The minimum and maximum sizes of the protected heap:
   //    min = 2MB, init = 8MB, max = 1GB (32-bit), 16GB (64-bit)
   //
   minSize_[MemProtected] = 2 * MBs;
   maxSize_[MemProtected] = (BYTES_PER_WORD == 4 ? 1 * GBs : 16 * GBs);
   targSize_[MemProtected] = 10 * MBs;

   //> The minimum and maximum sizes of the persistent heap:
   //    min = 512kB, init = 2MB, max = 128MB (32-bit), 2GB (64-bit)
   //  Although MemPersistent is used together with MemProtected, it is
   //  typically used far less because it only stores data that changes
   //  too often to incur the overhead of write-protection.
   //
   minSize_[MemPersistent] = 512 * kBs;
   maxSize_[MemPersistent] = (BYTES_PER_WORD == 4 ? 128 * MBs : 2 * GBs);
   targSize_[MemPersistent] = 4 * MBs;

   //> The minimum and maximum sizes of the dynamic heap:
   //    min = 2MB, init = 16MB, max = 1GB (32-bit), 16GB (64-bit)
   //
   minSize_[MemDynamic] = 2 * MBs;
   maxSize_[MemDynamic] = (BYTES_PER_WORD == 4 ? 1 * GBs : 16 * GBs);
   targSize_[MemDynamic] = 20 * MBs;

   //> The minimum and maximum sizes of the temporary heap:
   //    min = 512kB, init = 1MB, max = 128MB (32-bit), 2GB (64-bit)
   //
   minSize_[MemTemporary] = 512 * kBs;
   maxSize_[MemTemporary] = (BYTES_PER_WORD == 4 ? 128 * MBs : 2 * GBs);
   targSize_[MemTemporary] = 1 * MBs;
}

//------------------------------------------------------------------------------

HeapCfg::~HeapCfg()
{
   Debug::ft("HeapCfg.dtor");
}

//------------------------------------------------------------------------------

void HeapCfg::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void HeapCfg::RevertSize(MemoryType type)
{
   Debug::ft("HeapCfg.RevertSize");

   FunctionGuard guard(Guard_ImmUnprotect);

   if(targSize_[type] > currSize_[type])
      targSize_[type] = currSize_[type];
   else
      targSize_[type] = (9 * targSize_[type]) / 10;
}

//------------------------------------------------------------------------------

bool HeapCfg::SetTargSize(MemoryType type, size_t size, string& expl)
{
   Debug::ft("HeapCfg.SetTargSize");

   expl.clear();

   if(size < minSize_[type])
   {
      expl = "That is less than the minimum allowed size.";
      return false;
   }

   if(size > maxSize_[type])
   {
      expl = "That is greater than the maximum allowed size.";
      return false;
   }

   FunctionGuard guard(Guard_ImmUnprotect);
   targSize_[type] = size;
   return true;
}

//------------------------------------------------------------------------------

void HeapCfg::UpdateSize(MemoryType type)
{
   Debug::ft("HeapCfg.UpdateSize");

   FunctionGuard guard(Guard_ImmUnprotect);
   currSize_[type] = targSize_[type];
}
}
