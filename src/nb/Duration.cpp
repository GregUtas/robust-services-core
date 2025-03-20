//==============================================================================
//
//  Duration.cpp
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
#include "Duration.h"

using std::string;

namespace NodeBase
{
string to_string(const msecs_t& msecs)
{
   return std::to_string(msecs.count()) + "ms";
}

//------------------------------------------------------------------------------

string to_string(const nsecs_t& nsecs)
{
   return std::to_string(nsecs.count()) + "ns";
}

//------------------------------------------------------------------------------

string to_string(const secs_t& secs)
{
   return std::to_string(secs.count()) + "s";
}

//------------------------------------------------------------------------------

string to_string(const usecs_t& usecs)
{
   return std::to_string(usecs.count()) + "us";
}
}
