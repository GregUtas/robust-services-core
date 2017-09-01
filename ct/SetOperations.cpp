//==============================================================================
//
//  SetOperations.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SetOperations.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <set>
#include "Debug.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeTools_SetDifference1 = "CodeTools.SetDifference(-)";

void SetDifference(SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2)
{
   Debug::ft(CodeTools_SetDifference1);

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

   std::set_intersection(rhs1.cbegin(), rhs1.cend(),
      rhs2.cbegin(), rhs2.cend(), std::inserter(lhs, lhs.end()));
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

   std::set_union(rhs1.cbegin(), rhs1.cend(),
      rhs2.cbegin(), rhs2.cend(), std::inserter(lhs, lhs.end()));
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
