//==============================================================================
//
//  Q1Link.h
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
#ifndef Q1LINK_H_INCLUDED
#define Q1LINK_H_INCLUDED

#include <string>

namespace NodeBase
{
   template< typename T > class Q1Way;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Link for an item on a one-way queue.  An object that resides on a one-way
//  queue includes this as a member and implements a LinkDiff function that
//  returns the distance between the top of the object and its Q1Link member.
//
class Q1Link
{
   template< typename T > friend class Q1Way;
   friend class ObjectPool;
public:
   //  Public because an instance of this class is included in objects that
   //  can be queued.
   //
   Q1Link();

   //  Public because an instance of this class is included in objects that
   //  can be queued.
   //
   ~Q1Link();

   //  Returns true if the item is on a queue.
   //
   bool IsQueued() const { return next != nullptr; }

   //  Returns a string for displaying the link.
   //
   std::string to_str() const;
private:
   //  Overridden to prohibit copying.
   //
   Q1Link(const Q1Link& that);
   void operator=(const Q1Link& that);

   //  The next item in the queue.  Because Q1Way uses circular queues, a
   //  value of nullptr means that the item is not on a queue.
   //
   Q1Link* next;
};
}
#endif
