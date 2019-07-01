//==============================================================================
//
//  SysTypes.cpp
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
#include "SysTypes.h"
#include <ostream>

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
uintptr_t BadPointer()
{
   uintptr_t value = 0;
   auto bytes = reinterpret_cast< uint8_t* >(&value);
   for(size_t i = 0; i < BYTES_PER_POINTER; ++i) bytes[i] = 0xfd;
   return value;
}

const uintptr_t BAD_POINTER = BadPointer();

//------------------------------------------------------------------------------

const FlagId MaxFlagId = BITS_PER_WORD - 1;
const Flags NoFlags = Flags();
fixed_string EMPTY_STR = "";
fixed_string CRLF_STR = "\n";
fixed_string ERROR_STR = "#ERR!";
fixed_string SCOPE_STR = "::";

//------------------------------------------------------------------------------

fixed_string SwLogLevelStrings[SwLogLevel_N + 1] =
{
   "INFO",
   "WARNING",
   "ERROR",
   ERROR_STR
};

ostream& operator<<(ostream& stream, SwLogLevel level)
{
   if((level >= 0) && (level < SwLogLevel_N))
      stream << SwLogLevelStrings[level];
   else
      stream << SwLogLevelStrings[SwLogLevel_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string MemoryTypeStrings[MemoryType_N + 1] =
{
   "null",
   "temporary",
   "dynamic",
   "protected",
   "permanent",
   "immutable",
   ERROR_STR
};

ostream& operator<<(ostream& stream, MemoryType type)
{
   if((type >= 0) && (type < MemoryType_N))
      stream << MemoryTypeStrings[type];
   else
      stream << MemoryTypeStrings[MemoryType_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string RestartStrings[RestartLevel_N + 1] =
{
   ERROR_STR,
   "warm",
   "cold",
   "reload",
   "reboot",
   ERROR_STR,
   ERROR_STR
};

const char* strRestartLevel(RestartLevel level)
{
   if((level >= 0) && (level < RestartLevel_N)) return RestartStrings[level];
   return RestartStrings[RestartLevel_N];
}
}
