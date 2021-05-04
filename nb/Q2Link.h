//==============================================================================
//
//  Q2Link.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef Q2LINK_H_INCLUDED
#define Q2LINK_H_INCLUDED

#include <iosfwd>
#include <string>

namespace NodeBase
{
   template< class T > class Q2Way;
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
   template< class T > friend class Q2Way;
   friend class NbHeap;
public:
   //  Public because an instance of this class is included in objects that
   //  can be queued.
   //
   Q2Link();

   //  Public because an instance of this class is included in objects that
   //  can be queued.
   //
   ~Q2Link();

   //  Deleted to prohibit copying.
   //
   Q2Link(const Q2Link& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Q2Link& operator=(const Q2Link& that) = delete;

   //  Returns true if the item is on a queue.
   //
   bool IsQueued() const { return next != nullptr; }

   //  Displays member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;
private:
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
