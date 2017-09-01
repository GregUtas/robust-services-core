//==============================================================================
//
//  SysTypes.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SysTypes.h"

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
fixed_string SPACE_STR = " ";
fixed_string ERROR_STR = "#ERR!";
fixed_string SCOPE_STR = "::";

//------------------------------------------------------------------------------

fixed_string LogLevelStrings[LogLevel_N + 1] =
{
   "INFO",
   "DEBUG",
   "WARNING",
   "ERROR",
   ERROR_STR
};

ostream& operator<<(ostream& stream, LogLevel level)
{
   if((level >= 0) && (level < LogLevel_N))
      stream << LogLevelStrings[level];
   else
      stream << LogLevelStrings[LogLevel_N];
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

const char* NodeBase::strRestartLevel(RestartLevel level)
{
   if((level >= 0) && (level < RestartLevel_N)) return RestartStrings[level];
   return RestartStrings[RestartLevel_N];
}
}