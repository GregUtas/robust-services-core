//==============================================================================
//
//  SteadyTime.h
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
#ifndef STEADYTIME_H_INCLUDED
#define STEADYTIME_H_INCLUDED

#include <chrono>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A point in time (timestamp) on the monotonic (steady) clock.
//
namespace SteadyTime
{
   //  The underlying type for a timestamp.
   //
   using Point = std::chrono::steady_clock::time_point;

   //  Returns the current timestamp.
   //
   Point Now();

   //  Returns the timestamp when the system booted.
   //
   Point TimeZero();

   //  Returns an invalid timestamp (the clock's epoch) for reinitialization
   //  purposes.  This is also the value set by the default constructor.
   //
   Point GetInvalid();

   //  Returns true if the timestamp is not the value assigned by Invalidate.
   //
   bool IsValid(const Point& time);
}
}
#endif
