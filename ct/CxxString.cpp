//==============================================================================
//
//  CxxString.cpp
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
#include "CxxString.h"
#include <cstring>
#include "Cxx.h"
#include "Debug.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Forward declarations of local functions.
//
//  Starting at POS of NAME, returns the next left angle bracket at DEPTH
//  (the level of template nesting).  Returns string::npos if no such left
//  angle bracket exists.
//
size_t FindTemplateBegin(const string& name, size_t pos, size_t depth);

//  Starting at POS of NAME, which should be a left angle bracket, returns
//  the position of the matching right angle bracket.  Returns string::npos
//  if that right angle bracket is not found.
//
size_t FindTemplateEnd(const string& name, size_t pos);

//  Removes any spaces before or after an angle bracket or comma within NAME
//  and returns the result.
//
string RemoveTemplateSpaces(const string& name);

//  Returns the position of the last scope resolution operator between
//  BEGIN and END of NAME.  Ignores any operator that appears within a
//  template specification.
//
size_t RfindScopeOperator(const string& name, size_t begin, size_t end);

//  Finds substrings of NAME at DEPTH (the level of template nesting) and
//  deletes what precedes the last scope resolution operator in each one.
//  Returns false if no substring at DEPTH was found, which means that NAME
//  has been unqualified at all levels.
//
bool Unqualify(string& name, size_t depth);

//------------------------------------------------------------------------------

fn_name CodeTools_AdjustPtrs = "CodeTools.AdjustPtrs";

string& AdjustPtrs(string& type, TagCount ptrs)
{
   Debug::ft(CodeTools_AdjustPtrs);

   if(ptrs == 0) return type;
   if(type.empty()) return type;

   //  Back up to where TYPE's pointer tags, if any, are located.  Start by
   //  backing up over references and spaces.  There shouldn't be any spaces,
   //  but just in case...
   //
   auto constptr = false;
   auto pos = type.size() - 1;
   while(type[pos] == '&') --pos;
   while(type[pos] == SPACE) --pos;

   //  Back up over any const tag.  If one exists, we're currently at the 't'
   //  in "const", so TYPE would have to start with at least "X const" for a
   //  const tag to be present.
   //
   if(pos >= 6)
   {
      auto start = pos - 4;

      if(type.find(CONST_STR, start) == start)
      {
         //  Back up to any pointer tag.  Note that '@' can be a pointer tag
         //  when the indirection count is negative.  If a pointer tag is not
         //  found, the const tag must be for the type, not the pointer, so
         //  leave POS at the end of "const".  If a pointer tag *is* found,
         //  put POS one position before it.
         //
         constptr = true;
         pos = start - 1;
         while(type[pos] == SPACE) --pos;
      }
   }

   //  Back up over any pointers tags, and then step forward to the first one,
   //  if any.
   //
   while((type[pos] == '*') || (type[pos] == '@')) --pos;
   ++pos;

   if(ptrs > 0)
   {
      for(auto i = ptrs; i > 0; --i)
      {
         if(pos >= type.size())
         {
            type.push_back('*');
         }
         else
         {
            if(type[pos] == '@')
               type.erase(pos, 1);
            else
               type.insert(pos, 1, '*');
         }
      }
   }
   else
   {
      for(auto i = ptrs; i < 0; ++i)
      {
         if(pos >= type.size())
         {
            type.push_back('@');
         }
         else
         {
            if(type[pos] == '*')
               type.erase(pos, 1);
            else
               type.insert(pos, 1, '@');
         }
      }
   }

   //  If TYPE was a const pointer but no longer contains any pointers
   //  tags, remove the pointer's const tag.
   //
   if(constptr)
   {
      if(type[pos] != '*')
      {
         pos = type.find(" const", pos);
         if(pos != string::npos) type.erase(pos, 6);
      }
   }

   return type;
}

//------------------------------------------------------------------------------

fn_name CodeTools_Concatenate = "CodeTools.Concatenate";

void Concatenate(std::string& s)
{
   Debug::ft(CodeTools_Concatenate);

   std::vector< char > chars;
   size_t pos = 0;

   while(pos < s.size())
   {
      auto c = s[pos];

      switch(c)
      {
      case QUOTE:
         pos = s.find(QUOTE, pos + 1) + 1;
         break;
      case BACKSLASH:
         ++pos;
      }

      chars.push_back(s[pos]);
      ++pos;
   }

   s.clear();
   for(pos = 0; pos < chars.size(); ++pos) s.push_back(chars[pos]);
}

//------------------------------------------------------------------------------

bool FileExtensionIs(const std::string& file, const std::string& ext)
{
   auto s = '.' + ext;
   auto pos = file.rfind(s);
   if(pos == string::npos) return false;
   return (pos == file.size() - s.size());
}

//------------------------------------------------------------------------------

size_t FindIndex(const stringVector& sv, const string& s)
{
   for(size_t i = 0; i < sv.size(); ++i)
   {
      if(sv[i] == s) return i;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

fn_name CodeTools_FindSubstr = "CodeTools.FindSubstr";

size_t FindSubstr(const string& s, const string& targ)
{
   Debug::ft(CodeTools_FindSubstr);

   //  Look for TARG.  If it's found, check that it does not appear after
   //  a // comment or within a string literal.
   //
   auto pos = s.find(targ);

   if(pos != string::npos)
   {
      if(s.rfind(COMMENT_STR, pos) != string::npos) return string::npos;

      bool lit = false;

      for(size_t i = 0; i < pos; ++i)
      {
         if(s[i] == QUOTE) lit = !lit;
      }

      if(!lit) return pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t FindTemplateBegin(const string& name, size_t pos, size_t depth)
{
   size_t level = 1;

   for(NO_OP; pos < name.size(); ++pos)
   {
      switch(name[pos])
      {
      case '<':
         if(level == depth) return pos;
         ++level;
         break;
      case '>':
         --level;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t FindTemplateEnd(const string& name, size_t pos)
{
   size_t level = 1;

   for(NO_OP; pos < name.size(); ++pos)
   {
      switch(name[pos])
      {
      case '<':
         ++level;
         break;
      case '>':
         if(level == 1) return pos;
         --level;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

fn_name CodeTools_NameCouldReferTo = "CodeTools.NameCouldReferTo";

size_t NameCouldReferTo(const string& fqName, const string& name)
{
   Debug::ft(CodeTools_NameCouldReferTo);

   //  NAME must match a tail portion (or all of) fqName.  On a partial
   //  match, check that the match actually reached a scope operator.
   //
   auto pos = fqName.rfind(name);
   if(pos == string::npos) return string::npos;

   if(pos + name.size() == fqName.size())
   {
      if(pos == 0) return 0;
      if(fqName.compare(pos - 2, 2, SCOPE_STR) == 0) return pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

fn_name CodeTools_NameIsSuperscopeOf = "CodeTools.NameIsSuperscopeOf";

size_t NameIsSuperscopeOf(const string& fqName, const string& name)
{
   Debug::ft(CodeTools_NameIsSuperscopeOf);

   //  NAME must match a head portion (or all of) fqName.  On a partial
   //  match, check that the match actually reached a scope operator or
   //  template specification.
   //
   auto size = name.size();

   if(fqName.compare(0, size, name) == 0)
   {
      if(fqName.size() == size) return size;
      if(fqName.compare(size, 2, SCOPE_STR) == 0) return size;
      if(fqName[size] == '<') return size;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

string Normalize(const string& name)
{
   //  See if NAME contains any spaces.  If it does, it needs to be normalized.
   //
   size_t space = name.find(SPACE);

   if(space == string::npos)
   {
      //  Return NAME if it contains no ":".
      //
      size_t scope = name.rfind(SCOPE_STR);
      if(scope == string::npos) return name;

      //  If NAME has no "<", return the name after the last scope resolution
      //  operator.
      //
      size_t tmplt = name.find('<');
      if(tmplt == string::npos) return name.substr(scope + 2);
   }

   //  Remove spaces from NAME.  Then, at successive template depths, find the
   //  last scope resolution operator and delete any qualifiers that precede it.
   //
   auto result = RemoveTemplateSpaces(name);

   for(size_t depth = 0; true; ++depth)
   {
      if(!Unqualify(result, depth)) break;
   }

   return result;
}

//------------------------------------------------------------------------------

bool PathIncludes(const std::string& path, const std::string& dir)
{
   auto s = '/' + dir;
   auto pos = path.find(s);
   if(pos == string::npos) return false;
   if(pos == path.size() - s.size()) return true;
   return (path[pos + s.size()] == '/');
}

//------------------------------------------------------------------------------

string& Prefix(string& scope)
{
   if(scope.empty()) return scope;
   scope += SCOPE_STR;
   return scope;
}

//------------------------------------------------------------------------------

string& Prefix(string&& scope)
{
   if(scope.empty()) return scope;
   scope += SCOPE_STR;
   return scope;
}

//------------------------------------------------------------------------------

fn_name CodeTools_RemoveConsts = "CodeTools.RemoveConsts";

string RemoveConsts(const std::string& type)
{
   Debug::ft(CodeTools_RemoveConsts);

   //  Remove occurrences of "const " (a type or argument
   //  that is const) or " const" (a const pointer).
   //
   auto result = type;
   auto pos = type.find("const ");

   while(pos != string::npos)
   {
      result.erase(pos, 6);
      pos = result.find("const ", pos);
   }

   pos = result.find(" const");

   while(pos != string::npos)
   {
      result.erase(pos, 6);
      pos = result.find(" const", pos);
   }

   return result;
}

//------------------------------------------------------------------------------

fixed_string OperatorAmpersand = "operator&";

fn_name CodeTools_RemoveRefs = "CodeTools.RemoveRefs";

string& RemoveRefs(string& type)
{
   Debug::ft(CodeTools_RemoveRefs);

   auto pos = type.find('&');
   if(pos == string::npos) return type;

   auto opAmp = type.find(OperatorAmpersand);
   if(opAmp != string::npos) opAmp += strlen(OperatorAmpersand) - 1;

   do
   {
      if(pos != opAmp)
      {
         type.erase(pos, 1);
         pos = type.find('&', pos);
      }
      else
      {
         pos = type.find('&', pos + 1);
      }
   }
   while(pos != string::npos);

   return type;
}

//------------------------------------------------------------------------------

fn_name CodeTools_RemoveTags = "CodeTools.RemoveTags";

string& RemoveTags(string& type)
{
   Debug::ft(CodeTools_RemoveTags);

   if(type.find("const ") == 0) type.erase(0, 6);
   while(type.back() == '&') type.pop_back();
   while(type.back() == SPACE) type.pop_back();

   auto n = type.size();
   if(n >= 7)
   {
      n -= 5;
      if(type.rfind("const") == n)
      {
         type.erase(n);
         while(type.back() == SPACE) type.pop_back();
      }
   }

   while(type.back() == '*') type.pop_back();
   return type;
}

//------------------------------------------------------------------------------

fn_name CodeTools_RemoveTemplates = "CodeTools.RemoveTemplates";

string& RemoveTemplates(string&& type)
{
   Debug::ft(CodeTools_RemoveTemplates);

   while(true)
   {
      auto lpos = FindTemplateBegin(type, 0, 1);
      if(lpos == string::npos) break;
      auto rpos = FindTemplateEnd(type, lpos + 1);
      if(rpos == string::npos) break;
      type.erase(lpos, rpos - lpos + 1);
   }

   return type;
}

//------------------------------------------------------------------------------

string RemoveTemplateSpaces(const string& name)
{
   string result;

   //  It's easy if NAME contains no spaces.
   //
   auto pos = name.find(SPACE);

   if(pos == string::npos)
   {
      result = name;
      return result;
   }

   //  Step back to the character before the first space.
   //
   if(pos > 0)
   {
      result = name.substr(0, pos - 1);
      pos -= 1;
   }

   //  Erase any spaces before or after each angle bracket and comma.
   //
   while(pos < name.size())
   {
      auto c = name[pos];

      switch(c)
      {
      case '<':
      case '>':
      case ',':
         if(!name.empty()) while(result.back() == SPACE) result.pop_back();
         ++pos;
         if(pos < name.size()) pos = name.find_first_not_of(SPACE, pos);
         break;
      default:
         ++pos;
      }

      result.push_back(c);
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeTools_Replace = "CodeTools.Replace";

size_t Replace
   (string& code, const string& s1, const string& s2, size_t begin, size_t end)
{
   Debug::ft(CodeTools_Replace);

   auto size1 = s1.size();
   auto size2 = s2.size();

   for(auto pos = code.find(s1, begin); pos < end; pos = code.find(s1, pos))
   {
      auto prev = (pos > 0 ? code[pos - 1] : SPACE);
      auto next = (pos + size1 < code.size() ? code[pos + size1] : SPACE);

      //  Verify that S1 is preceded and followed by non-identifier characters.
      //  A destructor name begins with '~', which must therefore be allowed as
      //  the previous character.
      //
      if((!CxxChar::Attrs[prev].validFirst || (prev == '~')) &&
         !CxxChar::Attrs[next].validNext)
      {
         code.replace(pos, size1, s2);
         pos += size2;
         if(end != string::npos) end += (size2 - size1);
      }
      else
      {
         pos += size1;
      }
   }

   return end;
}

//------------------------------------------------------------------------------

size_t RfindScopeOperator(const string& name, size_t begin, size_t end)
{
   size_t level = 0;

   for(size_t pos = end; pos != begin - 1; --pos)
   {
      switch(name[pos])
      {
      case '<':
         --level;
         break;
      case '>':
         ++level;
         break;
      case ':':
         if(level == 0) return pos - 1;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Unqualify(string& name, size_t depth)
{
   if(depth == 0)
   {
      auto spos = RfindScopeOperator(name, 0, name.size() - 1);
      if(spos != string::npos) name.erase(0, spos + 2);
      return true;
   }

   auto found = false;

   for(size_t lpos = 0; lpos != string::npos; NO_OP)
   {
      lpos = FindTemplateBegin(name, lpos, depth);
      if(lpos == string::npos) break;
      auto rpos = FindTemplateEnd(name, lpos + 1);
      if(rpos == string::npos) break;

      found = true;
      auto spos = RfindScopeOperator(name, lpos + 1, rpos - 1);

      if(spos != string::npos)
      {
         auto count = spos - lpos + 1;
         name.erase(lpos + 1, count);
         rpos -= count;
      }

      lpos = rpos + 1;
   }

   return found;
}
}
