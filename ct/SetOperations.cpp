//==============================================================================
//
//  SetOperations.cpp
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
#include "SetOperations.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <set>
#include "Debug.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeTools_SetDifference1 = "CodeTools.SetDifference(-)";

void SetDifference(SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2)
{
   Debug::ft(CodeTools_SetDifference1);

   lhs.clear();
   std::set_difference(rhs1.cbegin(), rhs1.cend(),
      rhs2.cbegin(), rhs2.cend(), std::inserter(lhs, lhs.begin()));
}

//------------------------------------------------------------------------------

fn_name CodeTools_SetDifference2 = "CodeTools.SetDifference(-=)";

void SetDifference(SetOfIds& lhs, const SetOfIds& rhs)
{
   Debug::ft(CodeTools_SetDifference2);

   auto temp = std::unique_ptr< SetOfIds >(new SetOfIds);
   std::set_difference(lhs.cbegin(), lhs.cend(),
      rhs.cbegin(), rhs.cend(), std::inserter(*temp, temp->begin()));
   lhs.swap(*temp);
}

//------------------------------------------------------------------------------

fn_name CodeTools_SetIntersection1 = "CodeTools.SetIntersection(&)";

void SetIntersection(SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2)
{
   Debug::ft(CodeTools_SetIntersection1);

   lhs.clear();
   std::set_intersection(rhs1.cbegin(), rhs1.cend(),
      rhs2.cbegin(), rhs2.cend(), std::inserter(lhs, lhs.begin()));
}

//------------------------------------------------------------------------------

fn_name CodeTools_SetIntersection2 = "CodeTools.SetIntersection(&=)";

void SetIntersection(SetOfIds& lhs, const SetOfIds& rhs)
{
   Debug::ft(CodeTools_SetIntersection2);

   auto temp = std::unique_ptr< SetOfIds >(new SetOfIds);
   std::set_intersection(lhs.cbegin(), lhs.cend(),
      rhs.cbegin(), rhs.cend(), std::inserter(*temp, temp->begin()));
   lhs.swap(*temp);
}

//------------------------------------------------------------------------------

fn_name CodeTools_SetUnion1 = "CodeTools.SetUnion(|)";

void SetUnion(SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2)
{
   Debug::ft(CodeTools_SetUnion1);

   lhs.clear();
   std::set_union(rhs1.cbegin(), rhs1.cend(),
      rhs2.cbegin(), rhs2.cend(), std::inserter(lhs, lhs.begin()));
}

//------------------------------------------------------------------------------

fn_name CodeTools_SetUnion2 = "CodeTools.SetUnion(|=)";

void SetUnion(SetOfIds& lhs, const SetOfIds& rhs)
{
   Debug::ft(CodeTools_SetUnion2);

   auto temp = std::unique_ptr< SetOfIds >(new SetOfIds);
   std::set_union(lhs.cbegin(), lhs.cend(),
      rhs.cbegin(), rhs.cend(), std::inserter(*temp, temp->begin()));
   lhs.swap(*temp);
}
}
