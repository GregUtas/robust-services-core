//==============================================================================
//
//  RegCell.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef REGCELL_H_INCLUDED
#define REGCELL_H_INCLUDED

#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   template< typename T > class Registry;
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
   template< typename T > friend class Registry;
public:
   //  Until an object is registered, it has a nil identifier and has not
   //  been bound to the registry.
   //
   RegCell();

   //  Before an object is destroyed, it should have been removed from the
   //  registry.
   //
   ~RegCell();

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
   //  Overridden to prohibit copying.
   //
   RegCell(const RegCell& that);
   void operator=(const RegCell& that);

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
