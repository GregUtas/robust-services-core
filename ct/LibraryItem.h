//==============================================================================
//
//  LibraryItem.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef LIBRARYITEM_H_INCLUDED
#define LIBRARYITEM_H_INCLUDED

#include "Temporary.h"
#include <string>

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Base class for items in the code library (directories, files, variables).
//
class LibraryItem : public Temporary
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
      const std::string& prefix, const Flags& options) const override;
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
