//==============================================================================
//
//  Formatters.h
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
#ifndef FORMATTERS_H_INCLUDED
#define FORMATTERS_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   class Base;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
   //> Precedes the output of a hex value.
   //
   extern fixed_string HexPrefixStr;

   //> Precedes each object when a Display function outputs a queue of objects,
   //  as an alternative to strIndex (see below) when no index can be associated
   //  with each object.
   //
   extern fixed_string ObjSeparatorStr;

   //  Returns a string of COUNT spaces.
   //
   std::string spaces(int count);

   //  Converts N to a string.
   //
   std::string strInt(int64_t n);

   //  Outputs N (as hex) in STREAM.  If PREFIX is true, HexPrefixStr ("0x")
   //  is added as a prefix.  WIDTH is interpreted as follows:
   //  o negative: N occupies only as much space as needed
   //  o zero: fixed width based on the size of N (e.g. uint32_t = 8)
   //  o positive: the width specified
   //
   std::string strHex(uint64_t n, int width = -1, bool prefix = true);
   std::string strHex(uint32_t n, int width = -1, bool prefix = true);
   std::string strHex(uint16_t n, int width = -1, bool prefix = true);
   std::string strHex(uint8_t n, int width = -1, bool prefix = true);

   //  Converts a pointer to a string.
   //
   std::string strPtr(const void* p);

   //  Returns the string "[N]", followed by a ": " if COLON is set.  If WIDTH
   //  is non-zero, it specifies the width of N, which is padded with blanks.
   //
   std::string strIndex(int n, int width = 0, bool colon = true);

   //  Returns a string containing either NAME (if not nullptr), else VALUE.
   //
   std::string strName(const char* name, int value);

   //  Returns S with all characters converted to lower case.
   //
   std::string strLower(const std::string& s);

   //  Returns -1, 0, or 1 if S1 is less than, equal to, or greater than S2.
   //  Case is ignored.
   //
   int strCompare(const std::string& s1, const std::string& s2);

   //  Returns a string of length BREADTH.  S is centered in the string and
   //  is surrounded by BLANKS spaces, divided between prefix and postfix
   //  positions.  If S contains more than (BREADTH - BLANKS) characters,
   //  it is truncated.
   //
   std::string strCenter(const std::string& s, int breadth, int blanks);

   //  Displays BYTES, which total COUNT, in STREAM, after PREFIX.  The bytes
   //  are separated by spaces, and their ASCII equivalents are shown to the
   //  right, similar to a standard debug dump format.
   //
   void strBytes(std::ostream& stream,
      const std::string& prefix, const byte_t* bytes, size_t count);

   //  Modifies NAME by replacing each occurrence of a "::" with a ".".
   //
   void ReplaceScopeOperators(std::string& name);

   //  Uses typeid to return OBJ's class name.  Removes any namespace qualifier
   //  if NS is false.  Returns "undefined" if OBJ is nullptr.
   //
   //  WARNING: If OBJ is a bad pointer, this will trap.  This must be taken
   //  =======  into account.  Display functions, for example, can use strObj
   //  on static objects such as singletons, but dynamic objects and their
   //  pointer members should be displayed as raw pointers.  The rationale
   //  behind this is that an object may be displayed while recovering from
   //  a trap.  If the object is corrupt, its Display function should not
   //  trap during error recovery.
   //
   std::string strClass(const Base* obj, bool ns = true);

   //  Returns a string containing OBJ's "this" pointer followed by its class
   //  name as returned by strClass.
   //
   std::string strObj(const Base* obj, bool ns = true);
}

#endif
