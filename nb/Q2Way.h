//==============================================================================
//
//  Q2Way.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef Q2WAY_H_INCLUDED
#define Q2WAY_H_INCLUDED

#include <iosfwd>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "Q2Link.h"
#include "Restart.h"
#include "SysTypes.h"
#include "ThisThread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Two-way queue.  Recommended for long queues in which items are regularly
//  exqueued.
//  o no items: head_.next & head_.prev = head_ (empty circular queue)
//  o one item: head_.next & head_.prev = item, item.next & item.prev = head_
//  o two or more items: circular queue that includes head
//
template< typename T > class Q2Way
{
public:
   //  Initializes the queue header to default values.  Before the queue can
   //  be used, Init must be invoked.
   //
   Q2Way() : diff_(NilDiff) { }

   //  Cleans up the queue.
   //
   ~Q2Way()
   {
      if(head_.next == nullptr) return;  // Init never invoked
      if(head_.next != &head_)
      {
         Debug::ft(Q2Way_dtor());        // queue isn't empty
         Purge();
      }
      head_.prev = nullptr;              // expected by Q2Link destructor
      head_.next = nullptr;              // expected by Q2Link destructor
   }

   //  Initalizes the queue so that it is ready for use.
   //
   void Init(ptrdiff_t diff)
   {
      if(Restart::GetStatus() == Running)
      {
         Debug::ft(Q2Way_Init());
      }
      head_.next = &head_;  // head_ points to queue header
      head_.prev = &head_;  // tail points to queue header
      diff_ = diff;         // distance from each item's vptr to its Q2Link
   }

   //  Puts ELEM at the back of the queue.
   //
   bool Enq(T& elem)
   {
      if(Restart::GetStatus() == Running)
      {
         Debug::ft(Q2Way_Enq());
      }
      auto item = Item(elem);
      if(item == nullptr) return false;     // error
      if(item->next != nullptr) Exq(elem);  // if item is queued, exqueue it
      item->prev = head_.prev;              // item's prev is last item
      item->next = &head_;                  // item's next is queue head
      head_.prev->next = item;              // last item's next is now item
      head_.prev = item;                    // item is now last
      return true;
   }

   //  Puts ELEM at the front of the queue.
   //
   bool Henq(T& elem)
   {
      Debug::ft(Q2Way_Henq());
      auto item = Item(elem);
      if(item == nullptr) return false;     // error
      if(item->next != nullptr) Exq(elem);  // if item is queued, exqueue it
      item->prev = &head_;                  // item's prev is queue head
      item->next = head_.next;              // item's next is first item
      head_.next->prev = item;              // first item's prev is now item
      head_.next = item;                    // item is now first
      return true;
   }

   //  Takes the front item off the queue.
   //
   T* Deq()
   {
      Debug::ft(Q2Way_Deq());
      if(diff_ == NilDiff)
      {
         Debug::SwErr(Q2Way_Deq(), 0, 0);  // queue is not initialized
         return nullptr;
      }
      if(head_.next == &head_)             // if head points to itself
         return nullptr;                   // then queue is empty
      Q2Link* item = head_.next;           // access first item
      head_.next = item->next;             // new first is item's next
      item->next->prev = &head_;           // new first's prev is head
      item->next = nullptr;                // item has been removed
      item->prev = nullptr;                // item has been removed
      return (T*) getptr1(item, diff_);    // location of item's vptr
   }

   //  Removes ELEM from the queue.
   //
   bool Exq(T& elem)
   {
      Debug::ft(Q2Way_Exq());
      auto item = Item(elem);
      if(item == nullptr) return false;  // error
      if(item->next == nullptr)          // if item is not queued
         return true;                    // then return item immediately
      item->prev->next = item->next;     // prev item's next is item's next
      item->next->prev = item->prev;     // next item's prev is item's prev
      item->next = nullptr;              // item has been removed
      item->prev = nullptr;              // item has been removed
      return true;
   }

   //  Returns the first item in the queue.
   //
   T* First() const
   {
      if(diff_ == NilDiff) return nullptr;  // queue is not initialized
      Q2Link* item = head_.next;            // start at head
      if(item == &head_) return nullptr;    // check for empty queue
      return (T*) getptr1(item, diff_);     // location of item's vptr
   }

   //  Updates ELEM to the next item in the queue.
   //  Provides the first item if ELEM is nullptr.
   //
   bool Next(T*& elem) const
   {
      if(diff_ == NilDiff)
      {
         Debug::SwErr(Q2Way_Next(), 0, 0);  // queue is not initialized
         return false;
      }
      const Q2Link* item;                   // item will hold result
      if(elem == nullptr)                   // nullptr means
         item = &head_;                     // start at head
      else                                  // else
         item = (Q2Link*)
            getptr2(elem, diff_);           // start at current element
      item = item->next;                    // go to next element
      if(item == &head_)                    // if back at head
         elem = nullptr;                    // then no more items
      else                                  // else
         elem = (T*) getptr1(item, diff_);  // location of item's vptr
      return true;
   }

   T* Next(const T& elem) const
   {
      auto item = Item(elem);
      if(item == nullptr) return nullptr;     // error
      if(head_.prev == item) return nullptr;  // return if item was last
      item = item->next;                      // go to next element
      if(item == nullptr) return nullptr;     // return if item wasn't queued
      return (T*) getptr1(item, diff_);       // location of next item's vptr
   }

   //  Returns the last item in the queue.
   //
   T* Last() const
   {
      if(diff_ == NilDiff) return nullptr;  // queue is not initialized
      Q2Link* item = head_.prev;            // start at tail
      if(item == &head_) return nullptr;    // check for empty queue
      return (T*) getptr1(item, diff_);     // location of item's vptr
   }

   //  Updates ELEM to the previous item in the queue.
   //  Provides the last item if ELEM is nullptr.
   //
   bool Prev(T*& elem) const
   {
      if(diff_ == NilDiff)
      {
         Debug::SwErr(Q2Way_Prev(), 0, 0);  // queue is not initialized
         return false;
      }
      const Q2Link* item;                   // item will hold result
      if(elem == nullptr)                   // nullptr means
         item = &head_;                     // start at head
      else                                  // else
         item = (Q2Link*)
            getptr2(elem, diff_);           // start at current element
      item = item->prev;                    // go to previous element
      if(item == &head_)                    // if back at head
         elem = nullptr;                    // then no more items
      else                                  // else
         elem = (T*) getptr1(item, diff_);  // location of item's vptr
      return true;
   }

   T* Prev(const T& elem) const
   {
      auto item = Item(elem);
      if(item == nullptr) return nullptr;     // error
      if(head_.next == item) return nullptr;  // return if item was first
      item = item->prev;                      // go to previous element
      if(item == nullptr) return nullptr;     // return if item wasn't queued
      return (T*) getptr1(item, diff_);       // location of next item's vptr
   }

   //  Returns true if the queue is empty.
   //
   bool Empty() const
   {
      if(diff_ == NilDiff) return true;  // queue is not initialized
      return (head_.next == &head_);
   }

   //  Returns the number of items in the queue.
   //
   size_t Size() const
   {
      Debug::ft(Q2Way_Count());
      if(diff_ == NilDiff) return 0;  // queue is not initialized
      size_t count = 0;               // initialize count
      Q2Link* item = head_.next;      // start at first item
      while(item != &head_)           // queue eventually cycles to head
      {
         item = item->next;           // follow items back to head
         ++count;
      }
      return count;                   // report result
   }

   //  Deletes each item in the queue.
   //
   void Purge()
   {
      Debug::ft(Q2Way_Purge());
      if(diff_ == NilDiff)
      {
         Debug::SwErr(Q2Way_Purge(), 0, 0);  // queue is not initialized
         return;
      }
      while(head_.next != &head_)
      {
         auto item = Deq();
         delete item;
      }
   }

   //  Displays member variables.  Used if T is not subclassed from Base.
   //
   void Display(std::ostream& stream, const std::string& prefix) const
   {
      Show(stream, prefix, NoFlags);
   }

   //  Displays member variables.  Used if T is subclassed from Base.
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
private:
   //  Returns ELEM's link location.
   //
   Q2Link* Item(const T& elem) const
   {
      if(diff_ == NilDiff)
      {
         Debug::SwErr(Q2Way_Item(), 0, 0);  // queue is not initialized
         return nullptr;
      }
      if(&elem == nullptr)
      {
         Debug::SwErr(Q2Way_Item(), 0, 1);  // ELEM is invalid
         return nullptr;
      }

      return (Q2Link*) getptr2(&elem, diff_);
   }

   //  Displays member variables.
   //
   void Show(std::ostream& stream,
      const std::string& prefix, const Flags& options) const
   {
      stream << prefix << "head : " << CRLF;
      head_.Display(stream, prefix + spaces(2));
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

   //  Overridden to prohibit copying.
   //
   Q2Way(const Q2Way& that);
   void operator=(const Q2Way& that);

   //  See the comment in Singleton.h about fn_name's in a template header.
   //
   inline static fn_name Q2Way_ctor()  { return "Q2Way.ctor"; }
   inline static fn_name Q2Way_dtor()  { return "Q2Way.dtor"; }
   inline static fn_name Q2Way_Init()  { return "Q2Way.Init"; }
   inline static fn_name Q2Way_Enq()   { return "Q2Way.Enq"; }
   inline static fn_name Q2Way_Henq()  { return "Q2Way.Henq"; }
   inline static fn_name Q2Way_Deq()   { return "Q2Way.Deq"; }
   inline static fn_name Q2Way_Exq()   { return "Q2Way.Exq"; }
   inline static fn_name Q2Way_Next()  { return "Q2Way.Next"; }
   inline static fn_name Q2Way_Prev()  { return "Q2Way.Prev"; }
   inline static fn_name Q2Way_Count() { return "Q2Way.Count"; }
   inline static fn_name Q2Way_Purge() { return "Q2Way.Purge"; }
   inline static fn_name Q2Way_Item()  { return "Q2Way.Item"; }

   //  For initializing diff_.
   //
   static const ptrdiff_t NilDiff = -1;

   //  The queue head.  If the queue is empty, head_.next and head_.prev
   //  point to the head.
   //
   Q2Link head_;

   //  The distance from an item's vptr to its Q2Link.
   //
   ptrdiff_t diff_;
};
}
#endif
