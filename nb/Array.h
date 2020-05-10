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

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include "Debug.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  A vector that supports memory types and that moves its last item
//  into the cell vacated by an erased item.
//
namespace NodeBase
{
template< typename T, class A = std::allocator<T>> class Array
{
public:
   //  Creates an empty array.
   //
   Array() : max_(0)
   {
      Debug::ft(Array_ctor());
   }

   //  Deletes the array.
   //
   ~Array()
   {
      Debug::ft(Array_dtor());
   }

   //  Deleted to prohibit copying.
   //
   Array(const Array& that) = delete;
   Array& operator=(const Array& that) = delete;

   //  Specifies that the array uses memory of type MEM and that it
   //  is limited to MAX elements.
   //
   void Init(size_t max)
   {
      Debug::ft(Array_Init());
      max_ = (max < 2 ? 2 : max);
   }

   //  Increases the size of the array to support CAPACITY elements.
   //
   bool Reserve(size_t capacity)
   {
      Debug::ft(Array_Reserve());
      if(capacity > max_) return false;
      vector_.reserve(capacity);
      return true;
   }

   //  Inserts ITEM at the end of the array.
   //
   bool PushBack(T& item)
   {
      Debug::Assert(&item != nullptr);
      if(vector_.size() >= max_) return false;
      vector_.push_back(item);
      return true;
   }

   //  Erases the item in the cell specified by INDEX and
   //  moves the last item into its cell.
   //
   void Erase(size_t index)
   {
      auto size = vector_.size();
      Debug::Assert(index < size);
      if((size > 1) && (index < (size - 1)))
      {
         std::swap(vector_[index], vector_.back());
      }
      vector_.pop_back();
   }

   //  Replaces the item in the cell specified by INDEX with ITEM.
   //
   void Replace(size_t index, T& item)
   {
      Debug::Assert(&item != nullptr);
      auto size = vector_.size();
      Debug::Assert(index < size);
      vector_[index].~T();
      vector_[index] = std::move(item);
   }

   //  Returns the number of items in the array.
   //
   size_t Size() const { return vector_.size(); }

   //  Returns true if the array is empty.
   //
   bool Empty() const { return vector_.empty(); }

   //  Returns the first item.
   //
   const T& Front() const
   {
      Debug::Assert(vector_.size() > 0);
      return vector_.front();
   }

   //  Returns the first item.
   //
   T& Front()
   {
      Debug::Assert(vector_.size() > 0);
      return vector_.front();
   }

   //  Returns the last item.
   //
   const T& Back() const
   {
      Debug::Assert(vector_.size() > 0);
      return vector_.back();
   }

   //  Returns the last item.
   //
   T& Back()
   {
      Debug::Assert(vector_.size() > 0);
      return vector_.back();
   }

   //  Returns the item at [index].
   //
   const T& At(size_t index) const
   {
      Debug::Assert(index < vector_.size());
      return vector_[index];
   }

   //  Returns the item at [index].
   //
   T& At(size_t index)
   {
      Debug::Assert(index < vector_.size());
      return vector_[index];
   }

   //  Returns the item at [index].
   //
   const T& operator[](size_t index) const
   {
      Debug::Assert(index < vector_.size());
      return vector_[index];
   }

   //  Returns the item at [index].
   //
   T& operator[](size_t index)
   {
      Debug::Assert(index < vector_.size());
      return vector_[index];
   }

   //  Returns a pointer to the entire array of items.
   //
   const T* Data() const
   {
      if(vector_.empty()) return nullptr;
      return vector_.data();
   }

   //  Returns a pointer to the entire array of items.
   //
   T* Data()
   {
      if(vector_.empty()) return nullptr;
      return vector_.data();
   }
private:
   //  See the comment in Singleton.h about fn_name's in a template header.
   //
   inline static fn_name Array_ctor()     { return "Array.ctor"; }
   inline static fn_name Array_dtor()     { return "Array.dtor"; }
   inline static fn_name Array_Init()     { return "Array.Init"; }
   inline static fn_name Array_Reserve()  { return "Array.Reserve"; }

   //  The maximum size allowed for the array.
   //
   size_t max_;

   //  The array of items.
   //
   std::vector< T, A > vector_;
};
}
#endif
