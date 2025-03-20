//==============================================================================
//
//  HeapCfg.h
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
#ifndef HEAPCFG_H_INCLUDED
#define HEAPCFG_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   template<class T> class Singleton;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For configuring the size of heaps.
//
class HeapCfg : public Immutable
{
   friend class Singleton<HeapCfg>;
public:
   //  Deleted to prohibit copying.
   //
   HeapCfg(const HeapCfg& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   HeapCfg& operator=(const HeapCfg& that) = delete;

   //  For accessing the various sizes of the heap that supports TYPE.
   //
   size_t GetMinSize(MemoryType type) const { return minSize_[type]; }
   size_t GetMaxSize(MemoryType type) const { return maxSize_[type]; }
   size_t GetCurrSize(MemoryType type) const { return currSize_[type]; }
   size_t GetTargSize(MemoryType type) const { return targSize_[type]; }

   //  Sets the target size of the heap that supports TYPE.  Returns true
   //  on success.  On failure, returns false and updates EXPL with an
   //  explanation.
   //
   bool SetTargSize(MemoryType type, size_t size, std::string& expl);

   //  Changes the current size to the target size when the target size
   //  was successfully allocated.
   //
   void UpdateSize(MemoryType type);

   //  Changes the target size to the current size when the target size
   //  could not be allocated.
   //
   void RevertSize(MemoryType type);

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  The size of the immutable heap must be defined at compile time.
   //
   static const size_t SizeOfImmutableHeap = 512 * kBs;
private:
   //  Private because this is a singleton.
   //
   HeapCfg();

   //  Private because this is a singleton.
   //
   ~HeapCfg();

   //  The minimum size of each heap.
   //
   size_t minSize_[MemoryType_N];

   //  The maximum size of each heap.
   //
   size_t maxSize_[MemoryType_N];

   //  The current size of each heap.
   //
   size_t currSize_[MemoryType_N];

   //  The target size for each heap.
   //
   size_t targSize_[MemoryType_N];
};
}
#endif
