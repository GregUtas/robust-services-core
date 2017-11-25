//==============================================================================
//
//  CodeDirSet.h
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
   //  Identifies SET with NAME.
   //
   CodeDirSet(const std::string& name, SetOfIds* set);

   //  Override the operators supported by a set of code directories.
   //
   virtual LibrarySet* Create
      (const std::string& name, SetOfIds* set) const override;
   virtual LibrarySet* Files() const override;

   //  Returns the type of set.
   //
   virtual LibSetType GetType() const override { return DIR_SET; }

   //  Displays the full directory paths in STREAM and returns 0.
   //
   virtual word List(std::ostream& stream, std::string& expl) const override;

   //  Displays the directory names in RESULT and returns 0.
   //
   virtual word Show(std::string& result) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CodeDirSet();
};
}
#endif
