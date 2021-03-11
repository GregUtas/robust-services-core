//==============================================================================
//
//  LibraryItem.h
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
#ifndef LIBRARYITEM_H_INCLUDED
#define LIBRARYITEM_H_INCLUDED

#include "Base.h"
#include <set>
#include <string>
#include "CxxFwd.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Base class for items in the code library.
//
class LibraryItem : public NodeBase::Base
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~LibraryItem();

   //  Returns the item's name.  The default implementation generates a
   //  log and returns an empty string.
   //
   virtual const std::string& Name() const;

   //  Updates ITEMS with code items declared within the item.  The
   //  default implementation generates a log.
   //
   virtual void GetDecls(std::set< CxxNamed* >& items);
protected:
   //  Protected because this class is virtual.
   //
   LibraryItem();
};
}
#endif
