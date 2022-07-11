//==============================================================================
//
//  SysTypes.h
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

constexpr size_t BITS_PER_BYTE_LOG2 = 3;
constexpr size_t BYTES_PER_WORD = sizeof(uintptr_t);
constexpr size_t BYTES_PER_WORD_LOG2 = (BYTES_PER_WORD == 8 ? 3 : 2);
constexpr size_t BITS_PER_WORD = BYTES_PER_WORD << BITS_PER_BYTE_LOG2;
constexpr size_t BYTES_PER_POINTER = sizeof(uintptr_t);
constexpr size_t NIBBLES_PER_POINTER = 2 * BYTES_PER_POINTER;

//  For memory sizes.
//
constexpr size_t kBs = 1024;
constexpr size_t MBs = 1024 * 1024;
constexpr size_t GBs = 1024 * 1024 * 1024;

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

//  Indicates that a function must *not* invoke Debug::ft.  The function
//  itself can be invoked via Debug::ft, so a stack overflow could occur
//  if it also invoked Debug::ft.
//
#define NO_FT

//  Causes a trap.  Its value must differ from nullptr.
//
extern const uintptr_t BAD_POINTER;

//  For wrapping dynamically allocated strings and streams.
//
typedef std::unique_ptr<std::string> stringPtr;
typedef std::unique_ptr<std::ostringstream> ostringstreamPtr;
typedef std::unique_ptr<std::istream> istreamPtr;
typedef std::unique_ptr<std::ostream> ostreamPtr;

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

//  The type for a POSIX signal.
//
typedef int signal_t;

//  For defining C strings and C string constants.  These are used unless
//  a char is mandated by something external (e.g. by exception::what).
//
typedef const char* c_string;
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
constexpr FlagId MaxFlagId = 31;
constexpr FlagId FLAGS_SIZE = MaxFlagId + 1;
typedef std::bitset<FLAGS_SIZE> Flags;
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
constexpr char NUL = '\0';
constexpr char QUOTE = '"';
constexpr char SPACE = ' ';
constexpr char TAB = '\t';
extern const char PATH_SEPARATOR;

//  String constants.
//
extern fixed_string EMPTY_STR;
extern fixed_string CRLF_STR;
extern fixed_string SPACE_STR;
extern fixed_string QUOTE_STR;
extern fixed_string ERROR_STR;
extern fixed_string SCOPE_STR;

//  Type for a debug error code.
//
typedef uint64_t debug64_t;

//  Types of memory.
//
enum MemoryType
{
   MemNull,        // nil value
   MemTemporary,   // does not survive restarts
   MemDynamic,     // survives warm restarts
   MemSlab,        // survives warm restarts (used by object pools)
   MemPersistent,  // survives warm and cold restarts
   MemProtected,   // survives warm and cold restarts; write-protected
   MemPermanent,   // survives all restarts (default process heap)
   MemImmutable,   // survives all restarts; write-protected
   MemoryType_N    // number of memory types
};

//  Inserts a string for TYPE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, MemoryType type);

//  Whether a memory segment can be read, written, and/or executed.
//
enum MemoryProtection
{
   MemInaccessible = 0,      // ---
   MemExecuteOnly = 1,       // --x
   MemReadOnly = 4,          // r--
   MemReadExecute = 5,       // r-x
   MemReadWrite = 6,         // rw-
   MemReadWriteExecute = 7,  // rwx
   MemoryProtection_N = 8    // range of enumerators
};

//  Inserts a string for ATTRS into STREAM.
//
std::ostream& operator<<(std::ostream& stream, MemoryProtection attrs);

//  Types of restarts.
//
enum RestartLevel
{
   RestartNone,    // in service (not restarting)
   RestartWarm,    // deleting MemTemporary and exiting threads
   RestartCold,    // warm + deleting MemDynamic & MemSlab (user sessions)
   RestartReload,  // cold + deleting MemPersistent & MemProtected (config data)
   RestartReboot,  // exiting and restarting executable
   RestartExit,    // exiting without restarting
   RestartLevel_N  // number of restart levels
};

//  Inserts a string for LEVEL into STREAM.
//
std::ostream& operator<<(std::ostream& stream, RestartLevel level);

//  Outcomes for a thread delay (timed sleep) function.
//
enum DelayRc
{
   DelayInterrupted,  // interrupted before sleep interval expired
   DelayCompleted     // sleep interval expired
};
}
#endif
