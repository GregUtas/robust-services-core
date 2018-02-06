//==============================================================================
//
//  Array.h
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
#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <iosfwd>
#include <utility>
#include "Debug.h"
#include "Memory.h"
#include "SysDecls.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  A vector that supports memory types and that moves its last item
//  into the cell vacated by an erased item.
//
namespace NodeBase
{
template< typename T > class Array
{
public:
   //  Creates an empty array.
   //
   Array()
      : size_(0),
      cap_(0),
      max_(0),
      mem_(MemNull),
      array_(nullptr)
   {
      Debug::ft(Array_ctor());
   }

   //  Deletes the array.
   //
   ~Array()
   {
      Debug::ft(Array_dtor());
      Memory::Free(array_);
      array_ = nullptr;
   }

   //  Specifies that the array uses memory of type MEM and that it
   //  is limited to MAX elements.
   //
   bool Init(size_t max, MemoryType mem)
   {
      Debug::ft(Array_Init());
      if(array_ != nullptr)
      {
         Debug::SwErr(Array_Init(), cap_, max_);
         return false;
      }
      mem_ = mem;
      max_ = (max < 2 ? 2 : max);
      return true;
   }

   //  Increases the size of the array to support CAPACITY elements.
   //
   bool Reserve(size_t capacity)
   {
      Debug::ft(Array_Reserve());
      if(capacity > max_) return false;
      if(capacity <= cap_) return true;
      return Extend(capacity);
   }

   //  Inserts ITEM at the end of the array.
   //
   bool PushBack(const T& item)
   {
      if(&item == nullptr)
      {
         Debug::SwErr(Array_PushBack(), 0, 0);
         return false;
      }
      if(size_ >= cap_)
      {
         if(!Extend(size_ + 1)) return false;
      }
      array_[size_] = item;
      ++size_;
      return true;
   }

   //  Erases the item in the cell specified by INDEX, and
   //  moves the last item into its cell.
   //
   void Erase(size_t index)
   {
      if(index >= size_)
      {
         Debug::SwErr(Array_Erase(), index, size_);
         return;
      }
      if(--size_ == 0) return;
      if(index == size_) return;
      array_[index] = std::move(array_[size_]);
   }

   //  Replaces the item in the cell specified by INDEX with ITEM.
   //
   bool Replace(size_t index, const T& item)
   {
      if(&item == nullptr)
      {
         Debug::SwErr(Array_Replace(), 0, 0);
         return false;
      }
      if(index >= size_)
      {
         Debug::SwErr(Array_Replace(), index, size_);
         return false;
      }
      array_[index] = item;
      return true;
   }

   //  Returns the number of items in the array.
   //
   size_t Size() const { return size_; }

   //  Returns true if the registry is empty.
   //
   bool Empty() const { return (size_ == 0); }

   //  Returns the first item.
   //
   const T& Front() const
   {
      Debug::Assert(size_ > 0);
      return array_[0];
   }

   //  Returns the first item.
   //
   T& Front()
   {
      Debug::Assert(size_ > 0);
      return array_[0];
   }

   //  Returns the last item.
   //
   const T& Back() const
   {
      Debug::Assert(size_ > 0);
      return array_[size_ - 1];
   }

   //  Returns the last item.
   //
   T& Back()
   {
      Debug::Assert(size_ > 0);
      return array_[size_ - 1];
   }

   //  Returns the item at [index].
   //
   const T& At(size_t index) const
   {
      Debug::Assert(index < size_);
      return array_[index];
   }

   //  Returns the item at [index].
   //
   T& At(size_t index)
   {
      Debug::Assert(index < size_);
      return array_[index];
   }

   //  Returns the item at [index].
   //
   const T& operator[](size_t index) const
   {
      Debug::Assert(index < size_);
      return array_[index];
   }

   //  Returns the item at [index].
   //
   T& operator[](size_t index)
   {
      Debug::Assert(index < size_);
      return array_[index];
   }

   //  Returns a pointer to the entire array of items.
   //
   const T* Items() const { return array_; }

   //  Returns a pointer to the entire array of items.
   //
   T* Items() { return array_; }
private:
   //  Deleted to prohibit copying.
   //
   Array(const Array& that) = delete;
   Array& operator=(const Array& that) = delete;

   //  Increases the size of the registry's array, up to its maximum, when
   //  more space is needed.  MIN is the minimum number of elements to be
   //  supported.
   //
   bool Extend(size_t min)
   {
      Debug::ft(Array_Extend());
      if(cap_ >= max_) return false;
      if(min >= max_) return false;
      auto count = cap_ << 1;
      if(count == 0)
         count = 2;
      else if(count < min)
         count = min;
      else if(count > max_)
         count = max_;
      auto bytes = sizeof(T) * count;
      auto table = (T*) Memory::Alloc(bytes, mem_, false);
      if(table == nullptr) return false;
      for(size_t i = 0; i < size_; ++i) table[i] = std::move(array_[i]);
      cap_ = count;
      Memory::Free(array_);
      array_ = table;
      return true;
   }

   //  See the comment in Singleton.h about fn_name's in a template header.
   //
   inline static fn_name Array_ctor()     { return "Array.ctor"; }
   inline static fn_name Array_dtor()     { return "Array.dtor"; }
   inline static fn_name Array_Init()     { return "Array.Init"; }
   inline static fn_name Array_Reserve()  { return "Array.Reserve"; }
   inline static fn_name Array_PushBack() { return "Array.PushBack"; }
   inline static fn_name Array_Erase()    { return "Array.Erase"; }
   inline static fn_name Array_Replace()  { return "Array.Replace"; }
   inline static fn_name Array_Extend()   { return "Array.Extend"; }

   //  The number of items currently in the array.
   //
   size_t size_;

   //  The current size of the array.
   //
   size_t cap_;

   //  The maximum size allowed for the array.
   //
   size_t max_;

   //  The type of memory used by the array.
   //
   MemoryType mem_;

   //  The array of items.
   //
   T* array_;
};
}
#endif
