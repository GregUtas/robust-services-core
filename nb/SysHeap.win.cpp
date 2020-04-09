//==============================================================================
//
//  SysHeap.win.cpp
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
#include "SysHeap.h"
#include <intsafe.h>
#include <iomanip>
#include <ostream>
#include <windows.h>
#include "Debug.h"

using std::ostream;
using std::setw;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysHeap_ctor = "SysHeap.ctor";

SysHeap::SysHeap(MemoryType type, size_t size) :
   type_(type),
   heap_(nullptr),
   size_(size),
   attrs_(MemReadWrite),
   inUse_(0),
   allocs_(0),
   fails_(0),
   frees_(0),
   maxInUse_(0)
{
   Debug::ft(SysHeap_ctor);

   //  If this is the default heap, wrap it, else create it.
   //
   if(type_ == MemPermanent)
      heap_ = GetProcessHeap();
   else
      heap_ = HeapCreate(0, size_, size_);

   if(heap_ == nullptr)
   {
      Debug::SwLog(SysHeap_ctor, 0, GetLastError(), SwError);
   }
}

//------------------------------------------------------------------------------

fn_name SysHeap_dtor = "SysHeap.dtor";

SysHeap::~SysHeap()
{
   Debug::ft(SysHeap_dtor);

   //  If there's no actual heap, we're done.
   //
   if(heap_ == nullptr) return;

   //  Prevent an attempt to destroy the default heap.
   //
   if(type_ == MemPermanent)
   {
      Debug::SwLog(SysHeap_dtor, debug64_t(heap_), 0);
      return;
   }

   if(!HeapDestroy(heap_))
   {
      Debug::SwLog(SysHeap_dtor, debug64_t(heap_), GetLastError());
   }

   heap_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name SysHeap_Alloc= "SysHeap.Alloc";

void* SysHeap::Alloc(size_t size)
{
   Debug::ft(SysHeap_Alloc);

   if(heap_ == nullptr) return nullptr;

   auto memory = HeapAlloc(heap_, 0, size);

   if(memory != nullptr)
   {
      inUse_ += size;
      if(inUse_ > maxInUse_) maxInUse_ = inUse_;
      ++allocs_;
   }
   else
   {
      ++fails_;
   }

   return memory;
}

//------------------------------------------------------------------------------

void SysHeap::DisplayHeaps(ostream& stream)
{
   DWORD NumberOfHeaps;
   DWORD HeapsIndex;
   DWORD HeapsLength;
   HANDLE DefaultProcessHeap;
   HRESULT Result;
   HANDLE* aHeaps;
   SIZE_T BytesToAllocate;

   //  Retrieve the number of active heaps for the current process
   //  so we can calculate the buffer size needed for the heap handles.
   //
   NumberOfHeaps = GetProcessHeaps(0, nullptr);

   if(NumberOfHeaps == 0)
   {
      stream << "Failed to get number of heaps: err=" << GetLastError() << CRLF;
      return;
   }

   //  Calculate the buffer size.
   //
   Result = SIZETMult(NumberOfHeaps, sizeof(*aHeaps), &BytesToAllocate);

   if(Result != S_OK)
   {
      stream << "SIZETMult failed: result=" << Result << CRLF;
      return;
   }

   //  Get a handle to the default process heap.
   //
   DefaultProcessHeap = GetProcessHeap();

   if(DefaultProcessHeap == nullptr)
   {
      stream << "Failed to get default heap: err=" << GetLastError() << CRLF;
      return;
   }

   //  Allocate a buffer from the default process heap.
   //
   aHeaps = (HANDLE*) HeapAlloc(DefaultProcessHeap, 0, BytesToAllocate);

   if(aHeaps == nullptr)
   {
      stream << "Failed to allocate " << BytesToAllocate << " bytes." << CRLF;
      return;
   }

   //  Save the original number of heaps, because we are going to compare
   //  it to the return value of the next GetProcessHeaps call.
   //
   HeapsLength = NumberOfHeaps;

   //  Retrieve handles to the process heaps and display them.
   //
   NumberOfHeaps = GetProcessHeaps(HeapsLength, aHeaps);

   if(NumberOfHeaps == 0)
   {
      stream << "Failed to get list of heaps: err=" << GetLastError() << CRLF;
      return;
   }

   if(NumberOfHeaps != HeapsLength)
   {
      //  Compare the latest number of heaps with the original number.  If
      //  the latest number is larger than the original, another component
      //  has created a new heap and the buffer is now too small.
      //
      stream << "The number of heaps has changed." << CRLF;
   }
   else
   {
      stream << "  Heap  Address" << CRLF;

      for(HeapsIndex = 0; HeapsIndex < HeapsLength; ++HeapsIndex)
      {
         stream << setw(6) << HeapsIndex + 1;
         stream << setw(NIBBLES_PER_POINTER + 2) << aHeaps[HeapsIndex] << CRLF;
      }

      stream << "The default heap is at address "
         << DefaultProcessHeap << '.' << CRLF;
   }

   //  Release the memory allocated from the default process heap.
   //
   if(!HeapFree(DefaultProcessHeap, 0, aHeaps))
   {
      stream << "Failed to free memory allocated from default heap." << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name SysHeap_Free = "SysHeap.Free";

void SysHeap::Free(void* addr, size_t size)
{
   Debug::ft(SysHeap_Free);

   if(heap_ == nullptr) return;

   if(HeapFree(heap_, 0, addr))
   {
      inUse_ -= size;
      ++frees_;
   }
   else
   {
      Debug::SwLog(SysHeap_Free, debug64_t(addr), GetLastError());
   }
}

//------------------------------------------------------------------------------

void SysHeap::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SysHeap_Validate = "SysHeap.Validate";

bool SysHeap::Validate(const void* addr)
{
   Debug::ft(SysHeap_Validate);

   if(heap_ == nullptr) return false;
   return HeapValidate(heap_, 0, addr);
}
}
#endif
