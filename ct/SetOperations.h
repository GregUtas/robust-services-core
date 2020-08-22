//==============================================================================
//
//  SetOperations.h
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
   void SetUnion(SetOfIds& lhs, const SetOfIds& rhs1, const SetOfIds& rhs2);

   //  Implements lhs = lhs | rhs.
   //
   void SetUnion(SetOfIds& lhs, const SetOfIds& rhs);
}
#endif
