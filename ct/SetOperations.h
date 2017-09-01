//==============================================================================
//
//  SetOperations.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SETOPERATIONS_H_INCLUDED
#define SETOPERATIONS_H_INCLUDED

#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
   //  Implements lhs = rhs1 - rhs2.
   //
   void SetDifference
      (SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2);

   //  Implements lhs = lhs - rhs.
   //
   void SetDifference(SetOfIds& lhs, const SetOfIds& rhs);

   //  Implements lhs = rhs1 & rhs2.
   //
   void SetIntersection
      (SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2);

   //  Implements lhs = lhs & rhs.
   //
   void SetIntersection(SetOfIds& lhs, const SetOfIds& rhs);

   //  Implements lhs = rhs1 | rhs2.
   //
   void SetUnion
      (SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2);

   //  Implements lhs = lhs | rhs.
   //
   void SetUnion(SetOfIds& lhs, const SetOfIds& rhs);
}
#endif