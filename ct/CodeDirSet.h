//==============================================================================
//
//  CodeDirSet.h
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
#ifndef CODEDIRSET_H_INCLUDED
#define CODEDIRSET_H_INCLUDED

#include "CodeSet.h"
#include <string>
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

   //  Creates a set of directories (ITEMS) that is associated with NAME.
   //
   LibrarySet* Create
      (const std::string& name, const LibItemSet* items) const override;

   //  Returns the files in the directories.
   //
   LibrarySet* Files() const override;

   //  Returns the type of set.
   //
   LibSetType GetType() const override { return DIR_SET; }

   //  Displays the full directory paths in STREAM and returns 0.
   //
   NodeBase::word List(std::ostream& stream, std::string& expl) const override;

   //  Displays the directory names in RESULT and returns 0.
   //
   NodeBase::word Show(std::string& result) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CodeDirSet();
};
}
#endif
