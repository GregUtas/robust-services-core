//==============================================================================
//
//  SteadyTime.cpp
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
#include "SteadyTime.h"
#include <ratio>

//------------------------------------------------------------------------------

namespace NodeBase
{
static const SteadyTime::Point SteadyBootTime = SteadyTime::Now();

//------------------------------------------------------------------------------

SteadyTime::Point SteadyTime::GetInvalid()
{
   return Point::min();
}

//------------------------------------------------------------------------------

bool SteadyTime::IsValid(const Point& time)
{
   return (time != GetInvalid());
}

//------------------------------------------------------------------------------

SteadyTime::Point SteadyTime::Now()
{
   return std::chrono::steady_clock::now();
}

//------------------------------------------------------------------------------

SteadyTime::Point SteadyTime::TimeZero()
{
   return SteadyBootTime;
}
}
