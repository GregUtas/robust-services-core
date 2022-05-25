//==============================================================================
//
//  HeapCfg.cpp
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
   //    min = 2MB  max = 512MB (32-bit CPU), 8GB (64-bit CPU)
   //
   minSize_[MemProtected] = 2 * MBs;
   maxSize_[MemProtected] = size_t(1) << (25 + BYTES_PER_WORD);
   targSize_[MemProtected] = 4 * MBs;

   //> The minimum and maximum sizes of the persistent heap:
   //    min = 512kB  max = 128MB (32-bit CPU), 2GB (64-bit CPU)
   //  Although MemPersistent is used together with MemProtected, it is
   //  typically used far less because it only stores data that changes
   //  too often to incur the overhead of write-protection.
   //
   minSize_[MemPersistent] = 512 * kBs;
   maxSize_[MemPersistent] = size_t(1) << (23 + BYTES_PER_WORD);
   targSize_[MemPersistent] = 2 * MBs;

   //> The minimum and maximum sizes of the dynamic heap:
   //    min = 4MB  max = 1GB (32-bit CPU), 16GB (64-bit CPU)
   //  MemDynamic is the most used memory type.
   //
   minSize_[MemDynamic] = 4 * MBs;
   maxSize_[MemDynamic] = size_t(1) << (26 + BYTES_PER_WORD);
   targSize_[MemDynamic] = 168 * MBs;

   //> The minimum and maximum sizes of the temporaru heap:
   //    min = 512kB  max = 128MB (32-bit CPU), 2GB (64-bit CPU)
   //  MemTemporary is often the least used memory type.
   //
   minSize_[MemTemporary] = 512 * kBs;
   maxSize_[MemTemporary] = size_t(1) << (23 + BYTES_PER_WORD);
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
