//==============================================================================
//
//  CodeSet.h
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
#ifndef CODESET_H_INCLUDED
#define CODESET_H_INCLUDED

#include "LibrarySet.h"
#include <string>
#include "LibraryItem.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of code items (files or directories).
//
class CodeSet : public LibrarySet
{
public:
   //  Override the operators supported by both directories and files.
   //
   LibrarySet* Assign(LibrarySet* that) override;
   LibrarySet* Difference(const LibrarySet* that) const override;
   LibrarySet* Intersection(const LibrarySet* that) const override;
   LibrarySet* Union(const LibrarySet* that) const override;

   //  Updates STREAM with the number of files in the set and returns 0.
   //
   NodeBase::word Count(std::string& result) const override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
protected:
   //  Creates a set that is identified by NAME.  ITEMS is the actual set, if
   //  known.  Protected because this class is virtual.
   //
   CodeSet(const std::string& name, const LibItemSet* items);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~CodeSet();
private:
   //  Overridden to allow assignment.
   //
   NodeBase::word PreAssign(std::string& expl) const override { return 0; }
};
}
#endif
