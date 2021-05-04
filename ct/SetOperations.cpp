//==============================================================================
//
//  SetOperations.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "SetOperations.h"
#include <algorithm>
#include <iterator>
#include <set>
#include "Debug.h"
#include "Formatters.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
bool IsSortedByName(const LibraryItem* item1, const LibraryItem* item2)
{
   return (strCompare(item1->Name(), item2->Name()) < 0);
}

//------------------------------------------------------------------------------

void SetDifference
   (LibItemSet& lhs, const LibItemSet& rhs1, const LibItemSet& rhs2)
{
   Debug::ft("CodeTools.SetDifference(-)");

   lhs.clear();
   std::set_difference(rhs1.cbegin(), rhs1.cend(), rhs2.cbegin(),
      rhs2.cend(), std::inserter(lhs, lhs.begin()), IsSortedByName);
}

//------------------------------------------------------------------------------

void SetDifference(LibItemSet& lhs, const LibItemSet& rhs)
{
   Debug::ft("CodeTools.SetDifference(-=)");

   LibItemSet temp;
   std::set_difference(lhs.cbegin(), lhs.cend(), rhs.cbegin(),
      rhs.cend(), std::inserter(temp, temp.begin()), IsSortedByName);
   lhs.swap(temp);
}

//------------------------------------------------------------------------------

void SetIntersection
   (LibItemSet& lhs, const LibItemSet& rhs1, const LibItemSet& rhs2)
{
   Debug::ft("CodeTools.SetIntersection(&)");

   lhs.clear();
   std::set_intersection(rhs1.cbegin(), rhs1.cend(), rhs2.cbegin(),
      rhs2.cend(), std::inserter(lhs, lhs.begin()), IsSortedByName);
}

//------------------------------------------------------------------------------

void SetIntersection(LibItemSet& lhs, const LibItemSet& rhs)
{
   Debug::ft("CodeTools.SetIntersection(&=)");

   LibItemSet temp;
   std::set_intersection(lhs.cbegin(), lhs.cend(), rhs.cbegin(),
      rhs.cend(), std::inserter(temp, temp.begin()), IsSortedByName);
   lhs.swap(temp);
}

//------------------------------------------------------------------------------

void SetUnion
   (LibItemSet& lhs, const LibItemSet& rhs1, const LibItemSet& rhs2)
{
   Debug::ft("CodeTools.SetUnion(|)");

   lhs.clear();
   std::set_union(rhs1.cbegin(), rhs1.cend(), rhs2.cbegin(),
      rhs2.cend(), std::inserter(lhs, lhs.begin()), IsSortedByName);
}

//------------------------------------------------------------------------------

void SetUnion(LibItemSet& lhs, const LibItemSet& rhs)
{
   Debug::ft("CodeTools.SetUnion(|=)");

   LibItemSet temp;
   std::set_union(lhs.cbegin(), lhs.cend(), rhs.cbegin(),
      rhs.cend(), std::inserter(temp, temp.begin()), IsSortedByName);
   lhs.swap(temp);
}
}
