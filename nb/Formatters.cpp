//==============================================================================
//
//  Formatters.cpp
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
#include "Formatters.h"
#include <cctype>
#include <iomanip>
#include <ios>
#include <sstream>
#include "Base.h"
#include "Debug.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string HexPrefixStr = "0x";
fixed_string ObjSeparatorStr = "[->]: ";

//------------------------------------------------------------------------------

void ReplaceScopeOperators(string& name)
{
   auto pos = name.find(SCOPE_STR);

   while(pos != string::npos)
   {
      name.replace(pos, 2, 1, '.');
      pos = name.find(SCOPE_STR);
   }
}

//------------------------------------------------------------------------------

std::string spaces(int count)
{
   if(count <= 0) count = 0;
   if(count > 511) count = 511;
   return string(count, SPACE);
}

//------------------------------------------------------------------------------

void strBytes(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count)
{
   if(count <= 0)
   {
      stream << CRLF;
      return;
   }

   for(size_t i = 0; i < count; ++i)
   {
      if((i & 0x000f) == 0)
      {
         stream << prefix << strHex(i >> 4, 2, false) << ": ";
      }

      stream << strHex(bytes[i], 2, false);
      stream << SPACE;

      if((i & 0x000f) == 7) stream << "- ";

      if((i & 0x000f) == 15)
      {
         stream << SPACE;

         for(auto j = 0; j < 16; ++j)
         {
            if(bytes[i-15+j] <= 32)
               stream << '.';
            else
               stream << bytes[i-15+j];
         }

         stream << CRLF;
      }
   }

   auto extra = count & 0xf;

   if(extra != 0)
   {
      for(auto j = extra; j < 16; ++j)
      {
         stream << spaces(3);
         if((j & 0x000f) == 7) stream << spaces(2);
      }

      stream << SPACE;

      for(size_t j = 0; j < extra; ++j)
      {
         if(bytes[count-extra+j] <= 32)
            stream << '.';
         else
            stream << bytes[count - extra + j];
      }

      stream << CRLF;
   }
}

//------------------------------------------------------------------------------

string strCenter(const string& s, int breadth, int blanks)
{
   auto width = breadth - blanks;
   if(width <= 0) return EMPTY_STR;
   int size = s.size();
   if(size < width) width = size;

   std::ostringstream stream;

   auto fills = breadth - width;
   if(fills > 1) stream << spaces(fills / 2);
   stream << (size <= width ? s : s.substr(0, width));
   if(fills > 0) stream << spaces((fills + 1) / 2);
   return stream.str();
}

//------------------------------------------------------------------------------

string strClass(const Base* obj, bool ns)
{
   if(obj == nullptr) return "nullptr";

   string name(obj->ClassName());

   if(name.find("class ") == 0) name.erase(0, 6);

   if(!ns)
   {
      auto pos = name.rfind(SCOPE_STR);
      if(pos != std::string::npos) name.erase(0, pos + 2);
   }

   ReplaceScopeOperators(name);
   return name;
}

//------------------------------------------------------------------------------

int strCompare(const string& s1, const string& s2)
{
   auto size1 = s1.size();
   auto size2 = s2.size();

   for(size_t i = 0; ((i < size1) && (i < size2)); ++i)
   {
      auto c1 = tolower(s1[i]);
      auto c2 = tolower(s2[i]);
      if(c1 < c2) return -1;
      if(c1 > c2) return 1;
   }

   if(size1 < size2) return -1;
   if(size1 > size2) return 1;
   return 0;
}

//------------------------------------------------------------------------------

fn_name NodeBase_strGet = "CodeTools.strGet";

string strGet(string& input)
{
   Debug::ft(NodeBase_strGet);

   if(input.empty()) return input;

   auto begin = input.find_first_not_of(SPACE);
   if(begin == string::npos)
   {
      input.clear();
      return EMPTY_STR;
   }

   auto end = input.find_first_of(SPACE, begin);
   if(end == string::npos) end = input.size();
   auto str = input.substr(begin, end - begin);
   input.erase(0, end);
   return str;
}

//------------------------------------------------------------------------------

string strHex(uint64_t n, int width, bool prefix)
{
   std::ostringstream stream;

   auto w = (width == 0 ? 16 : width);
   if(prefix) stream << HexPrefixStr;
   if(w > 0) stream << setw(w) << std::setfill('0');
   stream << std::nouppercase << std::hex << n;
   return stream.str();
}

//------------------------------------------------------------------------------

string strHex(uint32_t n, int width, bool prefix)
{
   auto w = (width == 0 ? 8 : width);
   return strHex(uint64_t(n), w, prefix);
}

//------------------------------------------------------------------------------

string strHex(uint16_t n, int width, bool prefix)
{
   auto w = (width == 0 ? 4 : width);
   return strHex(uint64_t(n), w, prefix);
}

//------------------------------------------------------------------------------

string strHex(uint8_t n, int width, bool prefix)
{
   auto w = (width == 0 ? 2 : width);
   return strHex(uint64_t(n), w, prefix);
}

//------------------------------------------------------------------------------

string strIndex(int n, int width, bool colon)
{
   std::ostringstream stream;

   stream << '[';
   if(width != 0) stream << setw(width);
   stream << n << ']';
   if(colon) stream << ": ";
   return stream.str();
}

//------------------------------------------------------------------------------

string strInt(int64_t n)
{
   return std::to_string(n);
}

//------------------------------------------------------------------------------

string strLower(const string& s)
{
   string output;
   for(size_t i = 0; i < s.size(); ++i) output += tolower(s[i]);
   return output;
}

//------------------------------------------------------------------------------

string strName(const char* name, int value)
{
   if(name == nullptr) return std::to_string(value);
   return name;
}

//------------------------------------------------------------------------------

std::string strObj(const Base* obj, bool ns)
{
   std::ostringstream stream;

   stream << obj << SPACE << strClass(obj, ns);
   return stream.str();
}

//------------------------------------------------------------------------------

string strPtr(const void* p)
{
   std::ostringstream stream;

   stream << p;
   return stream.str();
}
}
