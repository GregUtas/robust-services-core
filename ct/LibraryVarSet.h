//==============================================================================
//
//  LibraryVarSet.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef LIBRARYVARSET_H_INCLUDED
#define LIBRARYVARSET_H_INCLUDED

#include "LibrarySet.h"
#include <string>
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Implments the library symbol $vars, which allows library variables to
//  be counted and listed.
//
class LibraryVarSet : public LibrarySet
{
public:
   //  Identifies SET with NAME.
   //
   explicit LibraryVarSet(const std::string& name);

   //  Updates RESULT with the number of library variables and returns 0.
   //
   NodeBase::word Count(std::string& result) const override;

   //  Returns the type of set.
   //
   LibSetType GetType() const override { return VAR_SET; }

   //  Returns a string for each variable in the set.
   //
   void to_str(stringVector& strings, bool verbose) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~LibraryVarSet();
};
}
#endif
