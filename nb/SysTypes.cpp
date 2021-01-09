//==============================================================================
//
//  SysTypes.cpp
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

const Flags NoFlags = Flags();
fixed_string EMPTY_STR = "";
fixed_string CRLF_STR = "\n";
fixed_string QUOTE_STR = "\"";
fixed_string ERROR_STR = "#ERR!";
fixed_string SCOPE_STR = "::";

//------------------------------------------------------------------------------

fixed_string MemoryProtectionStrings[MemoryProtection_N + 1] =
{
   "---",
   "--x",
   ERROR_STR,
   ERROR_STR,
   "r--",
   "r-x",
   "rw-",
   "rwx",
   ERROR_STR
};

ostream& operator<<(ostream& stream, MemoryProtection attrs)
{
   if((attrs >= 0) && (attrs < MemoryProtection_N))
      stream << MemoryProtectionStrings[attrs];
   else
      stream << MemoryProtectionStrings[MemoryType_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string MemoryTypeStrings[MemoryType_N + 1] =
{
   "null",
   "temporary",
   "dynamic",
   "persistent",
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
   "none",
   "warm",
   "cold",
   "reload",
   "reboot",
   "exit",
   ERROR_STR
};

ostream& operator<<(ostream& stream, RestartLevel level)
{
   if((level >= 0) && (level < RestartLevel_N))
      stream << RestartStrings[level];
   else
      stream << RestartStrings[RestartLevel_N];
   return stream;
}
}
