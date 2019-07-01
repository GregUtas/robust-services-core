//==============================================================================
//
//  SysTypes.h
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
#ifndef SYSTYPES_H_INCLUDED
#define SYSTYPES_H_INCLUDED

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Types defined here must be platform independent yet low level enough that
//  they conceivably apply to Sys* files, which implement the operating system
//  abstraction layer.
//
//  Word definitions.  These serve two purposes:
//  o alignment (to allocate raw memory using an array in which each
//    element aligns to the native word boundary)
//  o declaring an integer that is large enough to hold a pointer
//
typedef intptr_t word;
typedef uintptr_t uword;

constexpr word WORD_MAX = (sizeof(word) == 8 ? INT64_MAX: INT32_MAX);
constexpr word WORD_MIN = (sizeof(word) == 8 ? INT64_MIN: INT32_MIN);
constexpr uword UWORD_MAX = (sizeof(word) == 8 ? UINT64_MAX : UINT32_MAX);

constexpr size_t BYTES_PER_WORD = sizeof(uintptr_t);
constexpr size_t BYTES_PER_WORD_LOG2 = (BYTES_PER_WORD == 8 ? 3 : 2);
constexpr size_t BITS_PER_WORD = BYTES_PER_WORD << 3;
constexpr size_t BYTES_PER_POINTER = sizeof(uintptr_t);
constexpr size_t NIBBLES_PER_POINTER = 2 * BYTES_PER_POINTER;

//  Type for an identifier, most commonly used for items in a Registry.
//  Unsigned identifiers are preferred for compatibility with array
//  indices and size_t.  An identifier should be defined as 32 bits
//  unless it needs to be packed, either to conserve memory or because
//  it is passed in an interprocessor message.
//
typedef uint32_t id_t;

//  Nil identifier.
//
constexpr id_t NIL_ID = 0;

//  Indicates that an expression was not mistakenly omitted.  Most often
//  used instead of a bare semicolon in a for statement.
//
#define NO_OP

//  Causes a trap.  Its value must differ from nullptr.
//
extern uintptr_t const BAD_POINTER;

//  For wrapping dynamically allocated strings and streams.
//
typedef std::unique_ptr< std::string > stringPtr;
typedef std::unique_ptr< std::ostringstream > ostringstreamPtr;
typedef std::unique_ptr< std::istream > istreamPtr;
typedef std::unique_ptr< std::ostream > ostreamPtr;

//  Used when char* is for pointer arithmetic.
//
typedef char* ptr_t;
typedef const char* const_ptr_t;

//  Used for messages.
//
typedef uint8_t byte_t;

//  The type returned by main().
//
typedef int main_t;

//  The type for a Posix signal.
//
typedef int signal_t;

//  For defining a string constant.
//
typedef const char* const fixed_string;

//  Identifies a function by name.  The typedefs make it easier to track
//  usages or change the underlying type.
//
typedef const char* const fn_name;      // for defining a function name
typedef const char* const fn_name_arg;  // when fn_name is an argument

//  The depth of function call nesting on the stack.
//
typedef int16_t fn_depth;

//  Types for flags.
//
typedef uint8_t FlagId;
extern const FlagId MaxFlagId;
typedef std::bitset< BITS_PER_WORD > Flags;
extern const Flags NoFlags;

//  Maximum line length for formatted console output.
//
constexpr size_t COUT_LENGTH_MAX = 80;

//  Identifier for a column when writing to the console or a text file.
//
typedef uint8_t col_t;

//  Character constants.
//
constexpr char APOSTROPHE = '\'';
constexpr char BACKSLASH = '\\';
constexpr char CRLF = '\n';
constexpr char PATH_SEPARATOR = '/';
constexpr char QUOTE = '"';
constexpr char SPACE = ' ';
constexpr char TAB = '\t';

//  String constants.
//
extern fixed_string EMPTY_STR;
extern fixed_string CRLF_STR;
extern fixed_string ERROR_STR;
extern fixed_string SCOPE_STR;

//  Severity of software logs.  See Debug::SwLog.
//
enum SwLogLevel
{
   SwInfo,       // a basic debug log
   SwWarning,    // a log that includes a stack trace
   SwError,      // throws an exception (which includes a stack trace)
   SwLogLevel_N  // number of software log levels
};

//  Inserts a string for LEVEL into STREAM.
//
std::ostream& operator<<(std::ostream& stream, SwLogLevel level);

//  Types for debug error codes.
//
typedef uint32_t debug32_t;
typedef uint64_t debug64_t;

//  Types of memory.
//
enum MemoryType
{
   MemNull,      // nil value
   MemTemp,      // does not survive restarts
   MemDyn,       // survives warm restarts
   MemProt,      // survives warm and cold restarts; write-protected
   MemPerm,      // survives all restarts (default process heap)
   MemImm,       // survives all restarts; write-protected
   MemoryType_N  // number of memory types
};

//  Inserts a string for TYPE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, MemoryType type);

//  Types of restarts.
//
enum RestartLevel
{
   RestartNil,     // in service (not restarting)
   RestartWarm,    // deleting MemTemp and exiting threads
   RestartCold,    // warm plus deleting MemDyn (user sessions)
   RestartReload,  // cold plus deleting MemProt (configuration data)
   RestartReboot,  // exiting and restarting executable
   RestartExit,    // exiting without restarting
   RestartLevel_N  // number of restart levels
};

//  Returns a string that identifies LEVEL.  Returns ERROR_STR if
//  LEVEL is RestartNil or RestartExit.
//
const char* strRestartLevel(RestartLevel level);

//  The reason for a shutdown or restart.  See Restart.h for values.
//
typedef uint32_t reinit_t;

//  Outcomes for a thread delay (timed sleep) function.
//
enum DelayRc
{
   DelayError,        // failed to sleep
   DelayInterrupted,  // interrupted before sleep interval expired
   DelayCompleted     // sleep interval expired
};
}
#endif
