//==============================================================================
//
//  OrderedList.h
//
//  Copyright (C) 2012-2015 Greg Utas.  All rights reserved.
//
#ifndef ORDEREDLIST_H_INCLUDED
#define ORDEREDLIST_H_INCLUDED

#include <algorithm>
#include <iosfwd>
#include <string>
#include "Algorithms.h"
#include "AllocationException.h"
#include "Debug.h"
#include "Formatters.h"
#include "Memory.h"
#include "SysDefs.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
   const std::string OrderedList_ctor1      = "OrderedList.ctor";
   const std::string OrderedList_dtor       = "OrderedList.dtor";
   const std::string OrderedList_Init       = "OrderedList.Init";
   const std::string OrderedList_Insert     = "OrderedList.Insert";
   const std::string OrderedList_Remove     = "OrderedList.Remove";
// const std::string OrderedList_Contains   = "OrderedList.Contains";
// const std::string OrderedList_First      = "OrderedList.First";
// const std::string OrderedList_Next       = "OrderedList.Next";
// const std::string OrderedList_Count      = "OrderedList.Count";
// const std::string OrderedList_Empty      = "OrderedList.Empty";
   const std::string OrderedList_Clear      = "OrderedList.Clear";
// const std::string OrderedList_Swap       = "OrderedList.Swap";
   const std::string OrderedList_ctor2      = "OrderedList.ctor(copy)";
   const std::string OrderedList_ctor3      = "OrderedList.ctor(move)";
   const std::string OrderedList_assign     = "OrderedList.operator=";
   const std::string OrderedList_and_assign = "OrderedList.operator&=";
   const std::string OrderedList_sub_assign = "OrderedList.operator-=";
   const std::string OrderedList_or_assign  = "OrderedList.operator|=";
// const std::string OrderedList_IndexOf    = "OrderedList.IndexOf";
// const std::string OrderedList_IndexFor   = "OrderedList.IndexFor";
// const std::string OrderedList_ShiftUp    = "OrderedList.ShiftUp";
// const std::string OrderedList_ShiftDown  = "OrderedList.ShiftDown";
   const std::string OrderedList_Append     = "OrderedList.Append";
   const std::string OrderedList_Extend     = "OrderedList.Extend";
   const std::string OrderedList_and        = "OrderedList.operator&";
   const std::string OrderedList_sub        = "OrderedList.operator-";
   const std::string OrderedList_or         = "OrderedList.operator|";

//------------------------------------------------------------------------------
//
//  This class makes set operations (inclusion, union, intersection, difference)
//  efficient by storing elements in sorted order and preventing duplicates.
//  Elements must therefore support comparison operators.  Currently, integers
//  are the only elements, so this isn't a problem.  To support non-POD types,
//  a Traits function (key_compare) would need to be registered.
//    This class could be replaced by STL's set by providing an allocator for
//  the desired MemoryType and by using the set operations in <algorithm>.
//
template< typename T > class OrderedList
{
public:
   //  Creates an empty list.
   //
   OrderedList()
      : count_(0),
      size_(0),
      mem_(MEM_NULL),
      elements_(nullptr)
   {
      Debug::ft(&OrderedList_ctor1);
   }

   //  Copy constructor.
   //
   OrderedList(const OrderedList& that)
      : count_(0),
      size_(0),
      mem_(that.mem_),
      elements_(nullptr)
   {
      Debug::ft(&OrderedList_ctor2);
      if(!Extend(that.count_))
      {
         throw AllocationException(mem_, that.count_);
      }
      for(size_t i = 0; i < that.count_; ++i)
      {
         elements_[i] = that.elements_[i];
      }
      count_ = that.count_;
   }

   //  Move constructor.
   //
   OrderedList(OrderedList&& that)
      : count_(0),
      size_(0),
      mem_(MEM_NULL),
      elements_(nullptr)
   {
      Debug::ft(&OrderedList_ctor3);
      Swap(*this, that);
   }

   //  Deletes the objects in the list (unless this action was not
   //  desired) and the deletes the list's array.
   //
   ~OrderedList()
   {
      Debug::ft(&OrderedList_dtor);
      Memory::Free(elements_);
      elements_ = nullptr;
   }

   //  Assignment operator.
   //
   OrderedList& operator=(OrderedList that)
   {
      Debug::ft(&OrderedList_assign);
      Swap(*this, that);
      return *this;
   }

   //  Allocates memory of type MEM for the list's array.
   //  Space is initially allocated for MIN elements.
   //
   bool Init(MemoryType mem, size_t min = 16)
   {
      Debug::ft(&OrderedList_Init);
      if(elements_ != nullptr)
      {
         Debug::SwErr(&OrderedList_Init, size_, mem);
         return false;
      }
      mem_ = mem;
      auto size = min * sizeof(T);
      elements_ = (T*) Memory::Alloc(size, mem);
      size_ = min;
      return true;
   }

   //  Adds ITEM to the list if it is not already present.
   //
   bool Insert(T item)
   {
      //  Ensure that the list has been initialized.
      //
      if(size_ == 0)
      {
         Debug::SwErr(&OrderedList_Insert, 0, 0);
         return false;
      }
      auto slot = IndexFor(item);
      if(slot == WORD_MAX) return true;
      if((count_ + 1 >= size_) && !Extend(count_ + 1)) return false;
      ShiftDown(slot);
      elements_[slot] = item;
      ++count_;
      return true;
   }

   //  Removes ITEM from the list, keeping the list contiguous.
   //
   bool Erase(T item)
   {
      auto i = IndexOf(item);
      if(i == WORD_MAX)
      {
         Debug::SwErr(&OrderedList_Remove, 0, 0);
         return false;
      }
      ShiftUp(i);
      --count_;
      return true;
   }

   //  Returns true if ITEM is in the list.
   //
   bool Contains(T item) const
   {
      return (IndexOf(item) != WORD_MAX);
   }

   //  Updates ITEM to the first item in the list and returns 1.
   //  Returns 0 if the list is empty.
   //
   uword First(T& item) const
   {
      if(count_ > 0)
      {
         item = elements_[0];
         return 1;
      }
      return 0;
   }

   //  If INDEX is between 0 and the number of elements, sets ITEM to
   //  the next element in the list and increments INDEX.  If ITEM is
   //  the last item or INDEX is out of range, sets INDEX to 0.
   //
   void Next(uword& index, T& item) const
   {
      if(index >= count_)
      {
         index = 0;
         return;
      }
      item = elements_[index++];
   }

   //  Returns the number of items in the list.
   //
   size_t Count() const
   {
      return count_;
   }

   //  Returns true if the list is empty.
   //
   bool Empty() const
   {
      return (count_ == 0);
   }

   //  Removes all elements.
   //
   void Clear()
   {
      Debug::ft(&OrderedList_Clear);
      count_ = 0;
   }

   //  Set intersection assignment operator.
   //
   OrderedList& operator&=(const OrderedList& that)
   {
      Debug::ft(&OrderedList_and_assign);
      auto result = new OrderedList;
      auto min = minOf(count_, that.count_);
      result->Init(mem_, min);

      size_t i = 0;
      size_t j = 0;

      while((i < count_) && (j < that.count_))
      {
         if(elements_[i] == that.elements_[j])
         {
            result->Append(elements_[i++]);
            ++j;
         }
         else
         {
            if(elements_[i] < that.elements_[j])
               ++i;
            else
               ++j;
         }
      }
      Swap(*this, *result);
      delete result;
      return *this;
   }

   //  Set difference assignment operator.
   //
   OrderedList& operator-=(const OrderedList& that)
   {
      Debug::ft(&OrderedList_sub_assign);
      auto result = new OrderedList;
      result->Init(mem_, count_);

      size_t i = 0;
      size_t j = 0;

      while((i < count_) && (j < that.count_))
      {
         while(that.elements_[j] < elements_[i]) ++j;
         if(elements_[i] != that.elements_[j])
         {
            result->Append(elements_[i++]);
         }
         else
         {
            ++i;
            ++j;
         }
      }
      while(i < count_) result->Append(elements_[i++]);
      Swap(*this, *result);
      delete result;
      return *this;
   }

   //  Set union assignment operator.
   //
   OrderedList& operator|=(const OrderedList& that)
   {
      Debug::ft(&OrderedList_or_assign);
      auto result = new OrderedList;
      auto min = count_ + that.count_;
      result->Init(mem_, min);

      size_t i = 0;
      size_t j = 0;

      while((i < count_) && (j < that.count_))
      {
         if(elements_[i] == that.elements_[j])
         {
            result->Append(elements_[i++]);
            ++j;
         }
         else
         {
            if(elements_[i] < that.elements_[j])
               result->Append(elements_[i++]);
            else
               result->Append(that.elements_[j++]);
         }
      }
      while(i < count_) result->Append(elements_[i++]);
      while(j < that.count_) result->Append(that.elements_[j++]);
      Swap(*this, *result);
      delete result;
      return *this;
   }

   //  Displays member variables.
   //
   void Display(std::ostream& stream, col_t indent, bool verbose) const
   {
      stream << spaces(indent) << "count    : " << count_ << std::endl;
      stream << spaces(indent) << "size     : " << size_ << std::endl;
      stream << spaces(indent) << "mem      : " << mem_ << std::endl;
      stream << spaces(indent) << "elements : " << elements_ << std::endl;

      if(!verbose) return;

      for(size_t i = 0; i < count_; ++i)
      {
         stream << spaces(indent+2) << strIndex(i) << elements_[i] << std::endl;
      }
   }
private:
   //  The maximum size of the set.
   //
   static const size_t MAX_SIZE = UINT16_MAX;

   //  Swaps two lists.
   //
   friend void Swap(OrderedList& left, OrderedList& right)
   {
      std::swap(left.count_, right.count_);
      std::swap(left.size_, right.size_);
      std::swap(left.mem_, right.mem_);
      std::swap(left.elements_, right.elements_);
   }

   //  Returns ITEM's index.  Returns WORD_MAX if it is not present.
   //  The list is sorted, so this uses a binary search that narrows
   //  the search to one element and then checks it.
   //
   size_t IndexOf(T item) const
   {
      word min = 0;
      word max = count_ - 1;

      while(min < max)
      {
         auto mid = (min + max) >> 1;

         if(elements_[mid] < item)
            min = mid + 1;
         else
            max = mid;
      }

      if(elements_[min] == item) return min;
      return WORD_MAX;
   }

   //  Returns the index where ITEM should be inserted.  The list is
   //  sorted, so this uses a binary search that narrows the search
   //  to one slot before deciding what to do.
   //
   size_t IndexFor(T item) const
   {
      if(count_ == 0) return 0;
      word min = 0;
      word max = count_ - 1;

      while(min < max)
      {
         auto mid = (min + max) >> 1;

         if(elements_[mid] < item)
            min = mid + 1;
         else
            max = mid;
      }

      if(elements_[min] < item) return min + 1;
      if(elements_[min] == item) return WORD_MAX;
      return min;
   }

   //  Shifts entries starting at SLOT up one cell.
   //
   void ShiftUp(size_t slot)
   {
      for(size_t i = slot; i < count_; ++i)
      {
         elements_[i] = elements_[i + 1];
      }
   }

   //  Shifts entries starting at SLOT down one cell.
   //
   void ShiftDown(size_t slot)
   {
      for(size_t i = count_; i > slot; --i)
      {
         elements_[i] = elements_[i - 1];
      }
   }

   //  Adds ITEM to the end of the list when it is known to be larger
   //  than all other elements.
   //
   void Append(T item)
   {
      if((count_ + 1 >= size_) && !Extend(count_ + 1))
      {
         throw AllocationException(mem_, 1);
      }
      if((count_ > 0) && (elements_[count_ - 1] >= item))
      {
         Debug::SwErr(&OrderedList_Append, elements_[count_ - 1], item);
      }
      elements_[count_++] = item;
   }

   //  Increases the size of the list's array, up to its limit, when more
   //  space is needed.  MIN is the minimum index to be supported.
   //
   bool Extend(size_t min)
   {
      Debug::ft(&OrderedList_Extend);
      if(min < size_) return true;
      if(size_ > MAX_SIZE) return false;
      if(min > MAX_SIZE) return false;
      size_t count = minOf(size_ << 3, MAX_SIZE);
      if(count <= min) count = min;
      auto size = count* sizeof(T);
      auto table = (T*) Memory::Alloc(size, mem_, false);
      if(table == nullptr) return false;
      for(size_t i = 0; i < size_; ++i) table[i] = elements_[i];
      size_ = count;
      Memory::Free(elements_);
      elements_ = table;
      return true;
   }

   //  The number of items currently in the list.
   //
   size_t count_;

   //  The current size of the list.
   //
   size_t size_;

   //  The type of memory used by the list's array.
   //
   MemoryType mem_;

   //  The elements in the list.
   //
   T* elements_;
};

//  Implements set intersection.
//
template< typename T > OrderedList< T > operator&
   (OrderedList< T > lhs, const OrderedList< T >& rhs)
{
   Debug::ft(&OrderedList_and);
   lhs &= rhs;
   return lhs;
}

//  Implements set difference.
//
template< typename T > OrderedList< T > operator-
   (OrderedList< T > lhs, const OrderedList< T >& rhs)
{
   Debug::ft(&OrderedList_sub);
   lhs -= rhs;
   return lhs;
}

//  Implements set union.
//
template< typename T > OrderedList< T > operator|
   (OrderedList< T > lhs, const OrderedList< T >& rhs)
{
   Debug::ft(&OrderedList_or);
   lhs |= rhs;
   return lhs;
}
}
#endif
