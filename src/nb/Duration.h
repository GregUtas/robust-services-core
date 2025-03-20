//==============================================================================
//
//  Duration.h
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
#ifndef DURATION_H_INCLUDED
#define DURATION_H_INCLUDED

#include <chrono>
#include <cstdint>
#include <ratio>
#include <string>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Aliases for durations of different magnitudes.
//
using secs_t = std::chrono::seconds;
using msecs_t = std::chrono::milliseconds;
using usecs_t = std::chrono::microseconds;
using nsecs_t = std::chrono::nanoseconds;

//  For converting a duration to a string.
//
std::string to_string(const secs_t& secs);
std::string to_string(const msecs_t& msecs);
std::string to_string(const usecs_t& usecs);
std::string to_string(const nsecs_t& nsecs);

//  Duration constants.
//
constexpr msecs_t TIMEOUT_IMMED = msecs_t(0);
constexpr msecs_t TIMEOUT_NEVER = msecs_t(UINT32_MAX);
constexpr msecs_t ZERO_SECS = msecs_t(0);
constexpr msecs_t ONE_SEC = msecs_t(1000);

//  Conversion constants.
//
constexpr uint32_t NS_TO_US = std::nano::den / std::micro::den;
constexpr uint32_t NS_TO_MS = std::nano::den / std::milli::den;
constexpr uint32_t NS_TO_SECS = std::nano::den;
constexpr uint32_t SECS_TO_MS = std::milli::den;
}
#endif
