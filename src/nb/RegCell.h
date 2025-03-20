//==============================================================================
//
//  RegCell.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef REGCELL_H_INCLUDED
#define REGCELL_H_INCLUDED

#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   template<class T> class Registry;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Tracks the index at which an object was added to a registry's array.  An
//  object that resides in a registry usually includes this as a member and
//  implements a CellDiff function that returns the distance between the top
//  of the object and its RegCell member.  However, Registry also supports
//  registrants without RegCell members (see its documentation for details).
//
class RegCell
{
   template<class T> friend class Registry;
public:
   //  Until an object is registered, it has a nil identifier and has not
   //  been bound to the registry.
   //
   RegCell();

   //  Before an object is destroyed, it should have been removed from the
   //  registry.
   //
   ~RegCell();

   //  Deleted to prohibit copying.
   //
   RegCell(const RegCell& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   RegCell& operator=(const RegCell& that) = delete;

   //  Before an object is registered, this function allow its index within
   //  the registry (and therefore its identifier) to be specified.  This is
   //  important for an object whose identifier must be fixed (because it is
   //  included in an interprocessor protocol, for example).
   //
   void SetId(id_t cid);

   //  Returns the object's index (identifier) within the registry.
   //
   id_t GetId() const { return id; }

   //  Returns a string for displaying the cell.
   //
   std::string to_str() const;
private:
   //  The object's index (identifier) within the registry's array.
   //
   id_t id;

   //  Set when the object is added to the registry.  Cleared when the
   //  object is deregistered.
   //
   bool bound;
};
}
#endif
