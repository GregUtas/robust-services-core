//==============================================================================
//
//  CodeDirSet.h
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
#ifndef CODEDIRSET_H_INCLUDED
#define CODEDIRSET_H_INCLUDED

#include "CodeSet.h"
#include <string>
#include "LibraryItem.h"
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A set of code directories.
//
class CodeDirSet : public CodeSet
{
public:
   //  Identifies ITEMS with NAME.
   //
   CodeDirSet(const std::string& name, const LibItemSet* items);

   //  Override the operators supported by a set of directories.
   //
   LibrarySet* Directories() const override;
   LibrarySet* Files() const override;

   //  Returns the type of set.
   //
   LibSetType GetType() const override { return DIR_SET; }

   //  Returns a string for each directory in the set.
   //
   void to_str(stringVector& strings, bool verbose) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CodeDirSet();

   //  Overridden to create a set of directories.
   //
   LibrarySet* Create
      (const std::string& name, const LibItemSet* items) const override;
};
}
#endif
