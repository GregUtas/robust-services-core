//==============================================================================
//
//  Registry.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef REGISTRY_H_INCLUDED
#define REGISTRY_H_INCLUDED

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Base.h"
#include "Debug.h"
#include "Formatters.h"
#include "Memory.h"
#include "NbTypes.h"
#include "RegCell.h"
#include "Restart.h"
#include "SysTypes.h"
#include "ThisThread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A registry tracks objects derived from a common base class.  It uses
//  an array to save a pointer to each object that has been added to the
//  registry.  The array index at which an object is registered also acts
//  as an identifier for the object.  The first entry in the array is not
//  used and corresponds to NIL_ID (a nil object or nullptr).
//
template< class T > class Registry
{
public:
   //  Creates an empty registry.
   //
   Registry()
      : size_(0),
      capacity_(0),
      mem_(MemNull),
      max_(0),
      diff_(NilDiff),
      delete_(false),
      registry_(nullptr)
   {
      Debug::ft(Registry_ctor());
   }

   //  Deletes the objects in the registry (unless this action was not
   //  desired) and the deletes the registry's array.
   //
   ~Registry()
   {
      Debug::ftnt(Registry_dtor());
      if((delete_) && (capacity_ > 0)) Purge();
      Memory::Free(registry_, mem_);
      registry_ = nullptr;
   }

   //  Allocates memory of type MEM for the registry's array.  MAX is the
   //  maximum number of objects that can register.  All objects must derive
   //  from the same base class, with DIFF being the distance from the top of
   //  that base class to the RegCell member that tracks an object's location in
   //  the registry.  DEL is true if objects in the registry should be deleted
   //  when the registry is deleted.  This is typical but would be prevented,
   //  for example, if the objects were also added to another registry.  In
   //  such a case, the objects would require one RegCell member per registry.
   //  Alternatively, the second version of the Insert function could be used,
   //  as it does not require a RegCell member.
   //
   bool Init(id_t max, ptrdiff_t diff, MemoryType mem, bool del = true)
   {
      Debug::ft(Registry_Init());
      if(registry_ != nullptr)
      {
         Debug::SwLog(Registry_Init(), "already initialized", max_);
         return false;
      }
      if(diff < 0)
      {
         Debug::SwLog(Registry_Init(), "no cell offset", diff);
         return false;
      }
      max_ = (max == 0 ? 0 : max + 1);
      diff_ = diff;
      mem_ = mem;
      delete_ = del;
      if(max_ == 0) return true;
      auto size = sizeof(T*) * ((max >> 3) + 2);
      registry_ = (T**) Memory::Alloc(size, mem);
      capacity_ = (max >> 3) + 2;
      for(id_t i = 0; i < capacity_; ++i) registry_[i] = nullptr;
      return true;
   }

   //  Adds ITEM, which contains a RegCell member, to the registry.
   //
   bool Insert(T& item)
   {
      if(Restart::GetStage() == Running)
      {
         Debug::ft(Registry_Insert());
      }
      //  Ensure that ITEM has not already been registered.
      //
      auto cell = Cell(item);
      if(cell == nullptr) return false;
      if(cell->bound)
      {
         Debug::SwLog(Registry_Insert(), "already registered", cell->id);
         if((cell->id == NIL_ID) || (cell->id >= capacity_)) return false;
         return (registry_[cell->id] == &item);
      }
      //  If the item has a nil identifier, assign it to any available slot.
      //  If no slots remain, extend the size of the array.
      //
      if(cell->id == NIL_ID)
      {
         id_t start = 1;
         if(size_ + 1 >= capacity_)
         {
            start = capacity_;
            if(!Extend(capacity_)) return false;
         }
         for(auto i = start; i < capacity_; ++i)
         {
            if(registry_[i] == nullptr)
            {
               registry_[i] = &item;
               cell->id = i;
               cell->bound = true;
               ++size_;
               return true;
            }
         }
         return false;
      }
      //  If the item has a fixed identifier, assign it to that slot.  The
      //  array may first have to be extended.  If the slot is currently
      //  occupied, generate a log and delete the occupant unless this is
      //  a re-registration.
      //
      if(cell->id >= capacity_)
      {
         if(!Extend(cell->id)) return false;
      }
      if(registry_[cell->id] != &item)
      {
         if(registry_[cell->id] != nullptr)
         {
            if(delete_)
            {
               Debug::SwLog(Registry_Insert(), "identifier in use", cell->id);
               delete registry_[cell->id];
            }
            else
            {
               Erase(*registry_[cell->id]);
            }
         }
      }
      else
      {
         cell->bound = true;
         return true;
      }
      registry_[cell->id] = &item;
      cell->bound = true;
      ++size_;
      return true;
   }

   //  Adds ITEM to the registry in the slot specified by ID.  This function
   //  is used with ITEM does not contain a RegCell member.
   //
   bool Insert(T& item, id_t id)
   {
      if(Restart::GetStage() == Running)
      {
         Debug::ft(Registry_Insert());
      }
      //  Ensure that ITEM and ID are valid.
      //
      if(&item == nullptr)
      {
         Debug::SwLog(Registry_Insert(), "invalid item", 0);
         return false;
      }
      if(id > max_)
      {
         Debug::SwLog(Registry_Insert(), "invalid identifier", id);
         return false;
      }
      //  If ID is the nil identifier, assign ITEM to any available slot.
      //  If no slots remain, extend the size of the array.
      //
      if(id == NIL_ID)
      {
         id_t start = 1;
         if(size_ + 1 >= capacity_)
         {
            start = capacity_;
            if(!Extend(capacity_)) return false;
         }
         for(auto i = start; i < capacity_; ++i)
         {
            if(registry_[i] == nullptr)
            {
               registry_[i] = &item;
               ++size_;
               return true;
            }
         }
         return false;
      }
      //  If ID is a specific identifier, assign ITEM to that slot.  The
      //  array may first have to be extended.  If the slot is currently
      //  occupied, generate a log and delete the occupant unless this is
      //  a re-registration.
      //
      if(id >= capacity_)
      {
         if(!Extend(id)) return false;
      }
      if(registry_[id] != &item)
      {
         if(registry_[id] != nullptr)
         {
            if(delete_)
            {
               Debug::SwLog(Registry_Insert(), "identifier in use", id);
               delete registry_[id];
            }
            else
            {
               Erase(*registry_[id], id);
            }
         }
      }
      else
      {
         return true;
      }
      registry_[id] = &item;
      ++size_;
      return true;
   }

   //  Removes ITEM, which contains a RegCell member, from the registry.
   //
   bool Erase(T& item)
   {
      Debug::ftnt(Registry_Erase());
      //
      //  If ITEM has been registered, remove it from the registry.
      //
      auto cell = Cell(item);
      if(cell == nullptr) return false;
      if(cell->id == NIL_ID) return false;
      if(cell->id >= capacity_)
      {
         Debug::SwLog(Registry_Erase(), "invalid cell", cell->id);
         return false;
      }
      if(registry_[cell->id] != &item)
      {
         Debug::SwLog(Registry_Erase(), "incorrect item", cell->id);
         return false;
      }
      registry_[cell->id] = nullptr;
      cell->id = NIL_ID;
      cell->bound = false;
      --size_;
      return true;
   }

   //  Removes ITEM from the slot specified by ID.  This function is
   //  used when ITEM does not contain a RegCell member.
   //
   bool Erase(const T& item, id_t id)
   {
      Debug::ftnt(Registry_Erase());
      if(&item == nullptr)
      {
         Debug::SwLog(Registry_Erase(), "invalid item", 0);
         return false;
      }
      if((id == NIL_ID) || (id >= capacity_))
      {
         Debug::SwLog(Registry_Erase(), "invalid identifier", id);
         return false;
      }
      if(registry_[id] != &item)
      {
         Debug::SwLog(Registry_Erase(), "incorrect item", id);
         return false;
      }
      registry_[id] = nullptr;
      --size_;
      return true;
   }

   //  Returns the item registered against ID.
   //
   T* At(id_t id) const
   {
      if((id == NIL_ID) || (id >= capacity_)) return nullptr;
      return registry_[id];
   }

   //  Returns the first item in the registry.
   //
   T* First() const
   {
      for(id_t i = 1; i < capacity_; ++i)
      {
         if(registry_[i] != nullptr) return registry_[i];
      }
      return nullptr;
   }

   //  Returns the first item at ID or higher.
   //
   T* First(id_t& id) const
   {
      for(id_t i = id; i < capacity_; ++i)
      {
         if(registry_[i] != nullptr)
         {
            id = i;
            return registry_[i];
         }
      }
      id = NIL_ID;
      return nullptr;
   }

   //  Updates ITEM to the next item in the registry.
   //
   void Next(T*& item) const
   {
      auto cell = Cell(*item);
      item = nullptr;
      if(cell == nullptr) return;
      if((cell->id == NIL_ID) || (cell->id >= capacity_))
      {
         Debug::SwLog(Registry_Next(), "invalid cell", cell->id);
         return;
      }
      for(auto i = cell->id + 1; i < capacity_; ++i)
      {
         if(registry_[i] != nullptr)
         {
            item = registry_[i];
            return;
         }
      }
   }

   //  Returns the first item that follows ITEM.
   //
   T* Next(const T& item) const
   {
      auto cell = Cell(item);
      if(cell == nullptr) return nullptr;
      if((cell->id == NIL_ID) || (cell->id >= capacity_))
      {
         Debug::SwLog(Registry_Next(), "invalid cell", cell->id);
         return nullptr;
      }
      for(auto i = cell->id + 1; i < capacity_; ++i)
      {
         if(registry_[i] != nullptr) return registry_[i];
      }
      return nullptr;
   }

   //  Returns the first item that follows the slot identified by ID.
   //
   T* Next(id_t& id) const
   {
      if((id == NIL_ID) || (id >= capacity_))
      {
         Debug::SwLog(Registry_Next(), "invalid identifier", id);
         return nullptr;
      }
      for(auto i = id + 1; i < capacity_; ++i)
      {
         if(registry_[i] != nullptr)
         {
            id = i;
            return registry_[i];
         }
      }
      id = NIL_ID;
      return nullptr;
   }

   //  Returns the last item in the registry.
   //
   T* Last() const
   {
      if(capacity_ == 0) return nullptr;
      for(id_t i = capacity_ - 1; i > 0; --i)
      {
         if(registry_[i] != nullptr) return registry_[i];
      }
      return nullptr;
   }

   //  Updates ITEM to the previous item in the registry.
   //
   void Prev(T*& item) const
   {
      auto cell = Cell(*item);
      item = nullptr;
      if(cell == nullptr) return;
      if((cell->id == NIL_ID) || (cell->id >= capacity_))
      {
         Debug::SwLog(Registry_Prev(), "invalid cell", cell->id);
         return;
      }
      for(auto i = cell->id - 1; i > 0; --i)
      {
         if(registry_[i] != nullptr)
         {
            item = registry_[i];
            return;
         }
      }
   }

   //  Returns the first item that precedes ITEM.
   //
   T* Prev(const T& item) const
   {
      auto cell = Cell(item);
      if(cell == nullptr) return nullptr;
      if((cell->id == NIL_ID) || (cell->id >= capacity_))
      {
         Debug::SwLog(Registry_Prev(), "invalid cell", cell->id);
         return nullptr;
      }
      for(auto i = cell->id - 1; i > 0; --i)
      {
         if(registry_[i] != nullptr) return registry_[i];
      }
      return nullptr;
   }

   //  Returns the number of items in the registry.
   //
   id_t Size() const
   {
      Debug::ft(Registry_Size());
      return size_;
   }

   //  Returns true if the registry is empty.
   //
   bool Empty() const
   {
      Debug::ft(Registry_Empty());
      return (size_ == 0);
   }

   void Purge()
   {
      Debug::ftnt(Registry_Purge());
      for(id_t i = capacity_ - 1; i > 0; --i)
      {
         //  Clear the RegCell's bound flag so that its destructor won't
         //  generate a log.
         //
         auto& item = registry_[i];
         if(item != nullptr)
         {
            if(diff_ > 0)
            {
               auto cell = (RegCell*) getptr2(item, diff_);
               cell->bound = false;
            }
            delete item;
            item = nullptr;
            --size_;
         }
      }
   }

   //  Displays member variables.  T must be subclassed from Base.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const
   {
      Show(stream, prefix, options);

      if(!options.test(DispVerbose)) return;

      auto time = 5;

      for(id_t i = 0; i < capacity_; ++i)
      {
         auto lead1 = prefix + spaces(2);
         auto lead2 = prefix + spaces(4);

         if(registry_[i] != nullptr)
         {
            stream << lead1 << strIndex(i) << CRLF;
            registry_[i]->Display(stream, lead2, NoFlags);

            if(--time <= 0)
            {
               ThisThread::PauseOver(90);
               time = 5;
            }
         }
      }
   }
private:
   //  Deleted to prohibit copying.
   //
   Registry(const Registry& that) = delete;
   Registry& operator=(const Registry& that) = delete;

   //  Returns ELEM's cell location.
   //
   RegCell* Cell(const T& item) const
   {
      //  Ensure that the registry has been initialized with a RegCell offset.
      //
      if(diff_ <= 0)
      {
         Debug::SwLog(Registry_Cell(), "no cell offset", 0);
         return nullptr;
      }
      //  Ensure that ITEM is valid.
      //
      if(&item == nullptr)
      {
         Debug::SwLog(Registry_Cell(), "invalid item", 0);
         return nullptr;
      }
      return (RegCell*) getptr2(&item, diff_);
   }

   //  Increases the size of the registry's array, up to its limit, when more
   //  space is needed.  MIN is the minimum id_t to be supported.
   //
   bool Extend(id_t min)
   {
      Debug::ft(Registry_Extend());
      if(capacity_ >= max_) return false;
      if(min > max_) return false;
      auto count = std::min(2 * capacity_, max_);
      if(count <= min) count = min + 1;
      auto size = sizeof(T*) * count;
      auto table = (T**) Memory::Alloc(size, mem_);
      if(table == nullptr) return false;
      for(id_t i = 0; i < capacity_; ++i) table[i] = registry_[i];
      for(id_t i = capacity_; i < count; ++i) table[i] = nullptr;
      capacity_ = count;
      Memory::Free(registry_, mem_);
      registry_ = table;
      return true;
   }

   //  Displays member variables.
   //
   void Show(std::ostream& stream,
      const std::string& prefix, const Flags& options) const
   {
      stream << prefix << "size     : " << size_ << CRLF;
      stream << prefix << "capacity : " << capacity_ << CRLF;
      stream << prefix << "mem      : " << mem_ << CRLF;
      stream << prefix << "max      : " << max_ << CRLF;
      stream << prefix << "diff     : " << diff_ << CRLF;
      stream << prefix << "delete   : " << delete_ << CRLF;
      stream << prefix << "registry : " << registry_ << CRLF;

      if(options.test(DispVerbose)) return;

      auto time = 50;

      for(id_t i = 0; i < capacity_; ++i)
      {
         auto lead = prefix + spaces(2);

         if(registry_[i] != nullptr)
         {
            stream << lead << strIndex(i);
            stream << strObj(registry_[i]) << CRLF;

            if(--time <= 0)
            {
               ThisThread::PauseOver(90);
               time = 50;
            }
         }
      }
   }

   //  See the comment in Singleton.h about fn_name's in a template header.
   //
   inline static fn_name Registry_ctor()   { return "Registry.ctor"; }
   inline static fn_name Registry_dtor()   { return "Registry.dtor"; }
   inline static fn_name Registry_Init()   { return "Registry.Init"; }
   inline static fn_name Registry_Insert() { return "Registry.Insert"; }
   inline static fn_name Registry_Erase()  { return "Registry.Erase"; }
   inline static fn_name Registry_Next()   { return "Registry.Next"; }
   inline static fn_name Registry_Prev()   { return "Registry.Prev"; }
   inline static fn_name Registry_Size()   { return "Registry.Size"; }
   inline static fn_name Registry_Empty()  { return "Registry.Empty"; }
   inline static fn_name Registry_Purge()  { return "Registry.Purge"; }
   inline static fn_name Registry_Cell()   { return "Registry.Cell"; }
   inline static fn_name Registry_Extend() { return "Registry.Extend"; }

   //  For initializing diff_.
   //
   static const ptrdiff_t NilDiff = -1;

   //  The number of items currently in the registry.
   //
   id_t size_;

   //  The current size of the registry.
   //
   id_t capacity_;

   //  The type of memory used by the registry's array.
   //
   MemoryType mem_;

   //  The maximum size allowed for the registry.
   //
   id_t max_;

   //  The distance from a pointer to an item in the registry and its RegCell
   //  member.
   //
   ptrdiff_t diff_;

   //  Set if items in the registry should be deleted when overwritten or
   //  when the registry itself is deleted.
   //
   bool delete_;

   //  The registry, which is a dynamic array of pointers to registered items.
   //
   T** registry_;
};
}
#endif
