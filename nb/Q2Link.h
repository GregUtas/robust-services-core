//==============================================================================
//
//  Q2Link.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef Q2LINK_H_INCLUDED
#define Q2LINK_H_INCLUDED

#include <iosfwd>
#include <string>

namespace NodeBase
{
   template< typename T > class Q2Way;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Link for an item on a two-way queue.  An object that resides on a two-way
//  queue includes this as a member and implements a LinkDiff function that
//  returns the distance between the top of the object and its Q2Link member.
//
class Q2Link
{
   template< typename T > friend class Q2Way;
public:
   //  Public because an instance of this class is included in objects that
   //  can be queued.
   //
   Q2Link();

   //  Public because an instance of this class is included in objects that
   //  can be queued.
   //
   ~Q2Link();

   //  Returns true if the item is on a queue.
   //
   bool IsQueued() const { return next != nullptr; }

   //  Displays member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;
private:
   //  Overridden to prohibit copying.
   //
   Q2Link(const Q2Link& that);
   void operator=(const Q2Link& that);

   //  The next item in the queue.  Because Q2Way uses circular queues,
   //  a value of nullptr means that this item is not on a queue.
   //
   Q2Link* next;

   //  The previous item in the queue.  Because Q2Way uses circular queues,
   //  a value of nullptr means that this item is not on a queue.
   //
   Q2Link* prev;
};
}
#endif
