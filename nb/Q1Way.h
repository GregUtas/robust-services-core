//==============================================================================
//
//  Q1Way.h
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
#ifndef Q1WAY_H_INCLUDED
#define Q1WAY_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Base.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "Q1Link.h"
#include "Restart.h"
#include "SysTypes.h"
#include "ThisThread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  One-way queue.  Recommended unless items are often exqueued, which
//  can be expensive.
//  o no items: tail_.next = nullptr
//  o one item: tail_.next = item, item.next = item (points to itself)
//  o two or more items: tail_.next = last item, last item.next = first
//    item, second last item.next = last item (circular queue)
//
template< class T > class Q1Way
{
   friend class ObjectPool;
public:
   //  Initializes the queue header to default values.  Before the queue
   //  can be used, Init must be invoked.
   //
   Q1Way() : diff_(NilDiff) { }

   //  Cleans up the queue.
   //
   ~Q1Way()
   {
      if(tail_.next == nullptr) return;
      Debug::ft(Q1Way_dtor());
      Purge();
   }

   //  Deleted to prohibit copying.
   //
   Q1Way(const Q1Way& that) = delete;
   Q1Way& operator=(const Q1Way& that) = delete;

   //  Initializes the queue so that it can be used.
   //
   void Init(ptrdiff_t diff)
   {
      if(Restart::GetStage() == Running)
      {
         Debug::ft(Q1Way_Init());
      }
      tail_.next = nullptr;  // queue is empty
      diff_ = diff;          // distance from each item's vptr to its Q1Link
   }

   //  Puts ELEM at the back of the queue.
   //
   bool Enq(T& elem)
   {
      if(Restart::GetStage() == Running)
      {
         Debug::ft(Q1Way_Enq());
      }
      auto item = Item(elem);
      if(item == nullptr) return false;        // error
      if(item->next != nullptr) return false;  // if item is queued, do nothing
      if(tail_.next != nullptr)                // if queue isn't empty
      {                                        // then
         item->next = tail_.next->next;        // item points to first element
         tail_.next->next = item;              // last element points to item
      }
      else item->next = item;                  // else item points to itself
      tail_.next = item;                       // tail points to item
      return true;
   }

   //  Puts ELEM at the front of the queue.
   //
   bool Henq(T& elem)
   {
      Debug::ft(Q1Way_Henq());
      auto item = Item(elem);
      if(item == nullptr) return false;        // error
      if(item->next != nullptr) return false;  // if item is queued, do nothing
      if(tail_.next != nullptr)                // if queue isn't empty
      {
         item->next = tail_.next->next;        // item points to first element
         tail_.next->next = item;              // last element points to item
      }                                        // tail isn't changed, so item is
      else                                     // after last (and is thus first)
      {
         item->next = item;                    // item points to itself
         tail_.next = item;                    // tail points to item
      }
      return true;
   }

   //  Puts ELEM immediately after PREV.  If PREV is nullptr,
   //  ELEM goes at the front of the queue.
   //
   bool Insert(T* prev, T& elem)
   {
      Debug::ft(Q1Way_Insert());
      if(prev == nullptr)                        // if nothing is previous
         return Henq(elem);                      // then put item at head
      auto item = Item(elem);
      if(item == nullptr) return false;          // error
      auto ante = (Q1Link*)
         getptr2(prev, diff_);                   // put item after previous
      if(item->next != nullptr) return false;    // item must not be queued
      if(ante->next == nullptr) return false;    // prev must be queued
      item->next = ante->next;
      ante->next = item;
      if(tail_.next == ante) tail_.next = item;  // update tail if item is last
      return true;
   }

   //  Takes the front item off the queue.
   //
   T* Deq()
   {
      Debug::ft(Q1Way_Deq());
      if(tail_.next == nullptr)          // if tail points to nothing
         return nullptr;                 // then queue is empty
      Q1Link* item = tail_.next->next;   // set item to first element
      if(tail_.next != item)             // if item wasn't alone
         tail_.next->next = item->next;  // then last now points to second
      else
         tail_.next = nullptr;           // else queue is now empty
      item->next = nullptr;              // item has been removed
      return (T*) getptr1(item, diff_);  // location of item's vptr
   }

   //  Removes ELEM from anywhere on the queue.
   //
   bool Exq(T& elem)
   {
      Debug::ft(Q1Way_Exq());
      auto item = Item(elem);
      if(item == nullptr) return false;    // error
      if(item->next == nullptr)            // if the item isn't queued
         return true;                      // then return immediately
      if(tail_.next == nullptr)            // if the queue is empty
         return false;                     // then the item can't be there
      if(item->next == item)               // if the item points to itself
      {
         if(tail_.next == item)            // and if the item is also last
         {
            item->next = nullptr;          // then remove the item
            tail_.next = nullptr;          // and the queue is now empty
            return true;
         }
         return false;                     // the item is on another queue
      }
      auto curr = tail_.next;              // starting at the last element,
      while(curr->next != item)            // advance until the item is
      {
         curr = curr->next;                // the next element--but
         if(curr == tail_.next)            // stop after searching the
            return false;                  // entire entire queue
      }
      curr->next = item->next;             // curr's next becomes item's next
      if(tail_.next == item)               // if the item was the tail
         tail_.next = curr;                // then the tail has to back up
      item->next = nullptr;                // the item has been removed
      return true;
   }

   //  Returns the first item in the queue.
   //
   T* First() const
   {
      if(diff_ == NilDiff) return nullptr;  // queue is not initialized
      Q1Link* item = tail_.next;            // set item to first element
      if(item == nullptr) return nullptr;   // queue is empty
      item = item->next;                    // advance to head
      return (T*) getptr1(item, diff_);     // location of item's vptr
   }

   //  Updates ELEM to the next item in the queue.  If ELEM is nullptr,
   //  provides the first item.  Returns true if there was a next item.
   //
   bool Next(T*& elem) const
   {
      if(diff_ == NilDiff)
      {
         Debug::SwLog(Q1Way_Next(), 0, 0);  // queue is not initialized
         return false;
      }
      Q1Link* item;                         // item will hold result
      if(elem == nullptr)                   // nullptr means
      {
         item = tail_.next;                 // start at last element
         if(item == nullptr) return false;  // return if the queue is empty
      }
      else
      {
         item = (Q1Link*)
            getptr2(elem, diff_);           // start at the current item
         if(tail_.next == item)             // if that is the last element
         {
            elem = nullptr;                 // then there are no more
            return false;
         }
      }
      item = item->next;                    // advance to next element
      if(item == nullptr)                   // make sure the item was queued
      {
         elem = nullptr;
         return false;
      }
      elem = (T*) getptr1(item, diff_);     // location of item's vptr
      return true;
   }

   //  Returns the item that follows ELEM.
   //
   T* Next(const T& elem) const
   {
      auto item = Item(elem);
      if(item == nullptr) return nullptr;     // error
      if(tail_.next == item) return nullptr;  // return if item was last
      item = item->next;                      // advance to the next element
      if(item == nullptr) return nullptr;     // return if item wasn't queued
      return (T*) getptr1(item, diff_);       // location of next item's vptr
   }

   //  Returns true if the queue is empty.
   //
   bool Empty() const
   {
      return (tail_.next == nullptr);
   }

   //  Returns the number of items in the queue.
   //
   size_t Size() const
   {
      Debug::ft(Q1Way_Size());
      if(diff_ == NilDiff) return 0;  // queue is not initialized
      Q1Link* item = tail_.next;      // start at the last item
      if(item == nullptr) return 0;   // check for an empty queue
      size_t count = 1;               // there is at least one element
      item = item->next;              // advance to the first element
      while(item != tail_.next)       // stop when we reach the tail
      {
         item = item->next;
         ++count;
      }
      return count;                   // report the result
   }

   //  Deletes each item in the queue.
   //
   void Purge()
   {
      Debug::ft(Q1Way_Purge());
      while(tail_.next != nullptr)
      {
         auto item = Deq();
         delete item;
      }
   }

   //  Displays member variables.  T must be subclassed from Base.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const
   {
      Show(stream, prefix, options);

      if(!options.test(DispVerbose)) return;

      auto lead = prefix + spaces(2);
      auto time = 5;

      for(auto t = First(); t != nullptr; Next(t))
      {
         stream << prefix << ObjSeparatorStr << CRLF;
         t->Display(stream, lead, NoFlags);

         if(--time <= 0)
         {
            ThisThread::PauseOver(90);
            time = 5;
         }
      }
   }

   //  Corrupts ELEM's next pointer for testing purposes.  If ELEM is
   //  nullptr, the queue's tail pointer is corrupted instead.
   //
   void Corrupt(T* elem)
   {
      if(elem == nullptr)                     // if no ELEM provided
      {
         tail_.next = (Q1Link*) BAD_POINTER;  // corrupt queue header
         return;
      }
      if(diff_ == NilDiff)
      {
         Debug::SwLog(Q1Way_Next(), 0, 0);    // queue is not initialized
         return;
      }
      auto item = (Q1Link*)                   // start at the current item
         getptr2(elem, diff_);
      item->next = (Q1Link*) BAD_POINTER;     // corrupt ELEM's next pointer
   }
private:
   //  Returns ELEM's link location.
   //
   Q1Link* Item(const T& elem) const
   {
      if(diff_ == NilDiff)
      {
         Debug::SwLog(Q1Way_Item(), 0, 0);  // queue is not initialized
         return nullptr;
      }
      if(&elem == nullptr)
      {
         Debug::SwLog(Q1Way_Item(), 0, 1);  // ELEM is invalid
         return nullptr;
      }

      return (Q1Link*) getptr2(&elem, diff_);
   }

   //  Displays member variables.
   //
   void Show(std::ostream& stream,
      const std::string& prefix, const Flags& options) const
   {
      stream << prefix << "tail : " << tail_.to_str() << CRLF;
      stream << prefix << "diff : " << diff_ << CRLF;

      if(options.test(DispVerbose)) return;

      auto time = 50;

      for(auto t = First(); t != nullptr; Next(t))
      {
         stream << prefix << ObjSeparatorStr << strObj(t) << CRLF;

         if(--time <= 0)
         {
            ThisThread::PauseOver(90);
            time = 50;
         }
      }
   }

   //  See the comment in Singleton.h about fn_name's in a template header.
   //
   inline static fn_name Q1Way_dtor()   { return "Q1Way.dtor"; }
   inline static fn_name Q1Way_Init()   { return "Q1Way.Init"; }
   inline static fn_name Q1Way_Enq()    { return "Q1Way.Enq"; }
   inline static fn_name Q1Way_Henq()   { return "Q1Way.Henq"; }
   inline static fn_name Q1Way_Insert() { return "Q1Way.Insert"; }
   inline static fn_name Q1Way_Deq()    { return "Q1Way.Deq"; }
   inline static fn_name Q1Way_Exq()    { return "Q1Way.Exq"; }
   inline static fn_name Q1Way_Next()   { return "Q1Way.Next"; }
   inline static fn_name Q1Way_Size()   { return "Q1Way.Size"; }
   inline static fn_name Q1Way_Purge()  { return "Q1Way.Purge"; }
   inline static fn_name Q1Way_Item()   { return "Q1Way.Item"; }

   //  For initializing diff_.
   //
   static const ptrdiff_t NilDiff = -1;

   //  The queue head, which actually points to the tail item.
   //  If the queue is empty, tail_.next is nullptr.
   //
   Q1Link tail_;

   //  The distance from an item's vptr to its Q1Link.
   //
   ptrdiff_t diff_;
};
}
#endif
