//==============================================================================
//
//  LibraryItem.h
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
#ifndef LIBRARYITEM_H_INCLUDED
#define LIBRARYITEM_H_INCLUDED

#include "Base.h"
#include <string>

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Base class for items in the code library (directories, files, variables).
//
class LibraryItem : public NodeBase::Base
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~LibraryItem();

   //  Returns the item's name.
   //
   const std::string& Name() const { return name_; }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
protected:
   //  Creates an item that will be referred to by NAME.  Protected because
   //  this class is virtual.
   //
   explicit LibraryItem(const std::string& name);

   //  Provides non-const access to the item's name.
   //
   std::string* AccessName() { return &name_; }
private:
   //  The item's name.
   //
   std::string name_;
};
}
#endif
