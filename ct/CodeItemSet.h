//==============================================================================
//
//  CodeItemSet.h
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
#ifndef CODEITEMSET_H_INCLUDED
#define CODEITEMSET_H_INCLUDED

#include "CodeSet.h"
#include <string>
#include "CxxFwd.h"
#include "LibraryTypes.h"

namespace CodeTools
{
   struct CxxUsageSets;
}

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of C++ code items.
//
class CodeItemSet : public CodeSet
{
public:
   //  Identifies SET with NAME.
   //
   CodeItemSet(const std::string& name, const LibItemSet* items);

   //  Copies the items in USAGES into the set.
   //
   void CopyUsages(const CxxUsageSets& usages);

   //  Override the operators supported by a set of code files.
   //
   LibrarySet* CodeDeclarers() const override;
   LibrarySet* CodeReferencers() const override;
   LibrarySet* DeclaredBy() const override;
   LibrarySet* Definitions() const override;
   LibrarySet* Directories() const override;
   LibrarySet* FileDeclarers() const override;
   LibrarySet* FileReferencers() const override;
   LibrarySet* Files() const override;
   LibrarySet* ReferencedBy() const override;

   //  Returns the type of set.
   //
   LibSetType GetType() const override { return ITEM_SET; }

   //  Returns a string for each item in the set.
   //
   void to_str(stringVector& strings, bool verbose) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CodeItemSet();

   //  Copies ITEMS into the set.
   //
   void CopyItems(const CxxNamedSet& items);

   //  Overridden to create a set of C++ items.
   //
   LibrarySet* Create
      (const std::string& name, const LibItemSet* items) const override;
};
}
#endif
