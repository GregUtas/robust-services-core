//==============================================================================
//
//  CodeSet.h
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
#ifndef CODESET_H_INCLUDED
#define CODESET_H_INCLUDED

#include "LibrarySet.h"
#include <memory>
#include <string>
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of code items (code files or directories).
//
class CodeSet : public LibrarySet
{
public:
   //  Returns the set.
   //
   const SetOfIds& Set() const { return *set_; }

   SetOfIds& Set() { return *set_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Override the operators supported by both directories and files.
   //
   LibrarySet* Assign(LibrarySet* rhs) override;
   LibrarySet* Difference(const LibrarySet* rhs) const override;
   LibrarySet* Intersection(const LibrarySet* rhs) const override;
   LibrarySet* Union(const LibrarySet* rhs) const override;

   //  Updates STREAM with the number of files in the set and returns 0.
   //
   NodeBase::word Count(std::string& result) const override;
protected:
   //  Creates a set that is identified by NAME.  SET is the actual set, if
   //  known.  Protected because this class is virtual.
   //
   CodeSet(const std::string& name, SetOfIds* set);

   //  Protected to restrict deletion.
   //
   virtual ~CodeSet();
private:
   //  Overridden to allow assignment.
   //
   NodeBase::word PreAssign(std::string& expl) const override
      { return 0; }

   //  The set's contents.
   //
   std::unique_ptr< SetOfIds > set_;
};
}
#endif
