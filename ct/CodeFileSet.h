//==============================================================================
//
//  CodeFileSet.h
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
#ifndef CODEFILESET_H_INCLUDED
#define CODEFILESET_H_INCLUDED

#include "CodeSet.h"
#include <string>
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of library items (code files or directories).
//
class CodeFileSet : public CodeSet
{
public:
   //  Identifies SET with NAME.
   //
   CodeFileSet(const std::string& name, SetOfIds* set);

   //  Override the operators supported by a set of code files.
   //
   virtual LibrarySet* AffectedBy() const override;
   virtual LibrarySet* Affecters() const override;
   virtual LibrarySet* CommonAffecters() const override;
   virtual LibrarySet* Create
      (const std::string& name, SetOfIds* set) const override;
   virtual LibrarySet* Directories() const override;
   virtual LibrarySet* FileName(const LibrarySet* that) const override;
   virtual LibrarySet* FileType(const LibrarySet* that) const override;
   virtual LibrarySet* FoundIn(const LibrarySet* that) const override;
   virtual LibrarySet* Implements() const override;
   virtual LibrarySet* MatchString(const LibrarySet* that) const override;
   virtual LibrarySet* NeededBy() const override;
   virtual LibrarySet* Needers() const override;
   virtual LibrarySet* UsedBy(bool self) const override;
   virtual LibrarySet* Users(bool self) const override;

   //  Checks the code files in the set.
   //
   virtual word Check(std::ostream& stream, std::string& expl) const override;

   //  Updates RESULT with the number of lines of code in the set.
   //
   virtual word Countlines(std::string& result) const override;

   //  Formats the code files in the set.
   //
   virtual word Format(std::string& expl) const override;

   //  Returns the type of set.
   //
   virtual LibSetType GetType() const override { return FILE_SET; }

   //  Displays the full filenames in STREAM.
   //
   virtual word List(std::ostream& stream, std::string& expl) const override;

   //  Parses the code files in the set.
   //
   virtual word Parse
      (std::string& expl, const std::string& opts) const override;

   //  Displays, in STREAM, lines from the code files that match PATTERN.
   //
   virtual word Scan(std::ostream& stream,
      const std::string& pattern, std::string& expl) const override;

   //  Displays the build order in STREAM.
   //
   virtual word Sort(std::ostream& stream, std::string& expl) const override;

   //  Displays the filenames in RESULT.
   //
   virtual word Show(std::string& result) const override;

   //  Trims the code files in the set.
   //
   virtual word Trim(std::ostream& stream, std::string& expl) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CodeFileSet();

   //  Returns the build order of the set.
   //
   BuildOrderPtr SortInBuildOrder() const;
};
}
#endif
