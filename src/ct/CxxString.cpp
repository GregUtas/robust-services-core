//==============================================================================
//
//  CxxString.cpp
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
#include "CxxString.h"
#include <cstring>
#include "Cxx.h"
#include "Debug.h"

using namespace NodeBase;
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
static size_t FindTemplateBegin(const string& name, size_t pos, size_t depth);

//  Starting at POS of NAME, which should be a left angle bracket, returns
//  the position of the matching right angle bracket.  Returns string::npos
//  if that right angle bracket is not found.
//
static size_t FindTemplateEnd(const string& name, size_t pos);

//------------------------------------------------------------------------------

void AdjustPtrs(string& type, TagCount ptrs) //c reassess for >1 const pointer
{
   Debug::ft("CodeTools.AdjustPtrs");

   if(ptrs == 0) return;
   if(type.empty()) return;

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
            type.push_back('*');
         else if(type[pos] == '@')
            type.erase(pos, 1);
         else
            type.insert(pos, 1, '*');
      }
   }
   else
   {
      for(auto i = ptrs; i < 0; ++i)
      {
         if(pos >= type.size())
            type.push_back('@');
         else if(type[pos] == '*')
            type.erase(pos, 1);
         else
            type.insert(pos, 1, '@');
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
}

//------------------------------------------------------------------------------

size_t CompareScopes(const string& fqSub, const string& fqSuper, bool tmplt)
{
   Debug::ft("CodeTools.CompareScopes");

   //  fqSuper is a superscope of fqSub if it matches all, or a front portion,
   //  of fqSub.  On a partial match, check that the match actually reached a
   //  scope operator or, if TMPLT is set, template arguments in fqSub.
   //
   if(fqSub.rfind(fqSuper, 0) == 0)
   {
      auto size = fqSuper.size();
      if(size == fqSub.size()) return size;
      if(fqSub.compare(size, 2, SCOPE_STR) == 0) return size;
      if(tmplt && (fqSub[size] == '<')) return size;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

string Compress(const string& s)
{
   //  Copy S to T, converting endlines to blanks and removing multiple blanks.
   //
   string t;
   auto prev = SPACE;

   for(size_t i = 0; i < s.size(); ++i)
   {
      auto c = s[i];
      if(c == CRLF) c = SPACE;
      if((c == SPACE) && (prev == SPACE)) continue;
      t += c;
      prev = c;
   }

   return t;
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

size_t FindSubstr(const string& s, const string& targ)
{
   Debug::ft("CodeTools.FindSubstr");

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

static size_t FindTemplateBegin(const string& name, size_t pos, size_t depth)
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

static size_t FindTemplateEnd(const string& name, size_t pos)
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

string GetFileExtension(const string& file)
{
   auto pos = file.rfind('.');
   if(pos == string::npos) return EMPTY_STR;
   if(file.back() == '.') return EMPTY_STR;
   return file.substr(pos + 1);
}

//------------------------------------------------------------------------------

string GetFileName(const string& path)
{
   Debug::ft("CodeTools.GetFileName");

   auto file = path;
   auto pos = file.rfind('/');

   if(pos != string::npos)
   {
      file = file.substr(pos + 1);
   }
   else
   {
      pos = file.rfind(BACKSLASH);
      if(pos != string::npos) file = file.substr(pos + 1);
   }

   return file;
}

//------------------------------------------------------------------------------

fn_name CodeTools_GetNameAndArgs = "CodeTools.GetNameAndArgs";

NameVector GetNameAndArgs(const string& name)
{
   Debug::ft(CodeTools_GetNameAndArgs);

   //  Put the outer name in NAMES[0] and its N template arguments in NAMES[1]
   //  through NAMES[N].  Append any nested template arguments (DEPTH > 1) to
   //  the template argument to which they belong.  This is necessary because
   //  DataSpec::NamesReferToArgs invokes NameRefersToItem recursively, which
   //  will unpack a nested template.  For example, A<B<C,D>,E> results in the
   //  strings A, B<C,D>, and E, where B<C,D> will be unpacked recursively.
   //
   NameVector names;
   NameAndPtrs curr;
   size_t depth = 0;  // depth of angle brackets

   for(size_t i = 0; i < name.size(); ++i)
   {
      auto c = name[i];

      if(c == '<') ++depth;

      //  Only the template name and its arguments are separated.  Any inner
      //  templates and arguments remain together, so just append characters
      //  when dealing with an inner name.
      //
      if(depth > 1)
      {
         curr.name.push_back(c);
         if(c == '>') --depth;
         continue;
      }

      switch(c)
      {
      case '<':
         //
         //  This is the start of the first template argument, so save the
         //  template name that preceded it.
         //
         names.push_back(curr);
         curr = NameAndPtrs();
         break;

      case '>':
         //
         //  This is the end of the last template argument, so save it.
         //
         --depth;
         names.push_back(curr);
         curr = NameAndPtrs();
         break;

      case '*':
         //
         //  This is a pointer tag for a template argument.
         //
         ++curr.ptrs;
         break;

      case ',':
         //
         //  This ends one template argument and precedes another.
         //
         names.push_back(curr);
         curr = NameAndPtrs();
         break;

      case '&':
         //
         //  Reference tags on template arguments disappear, but this
         //  marks the end of a template argument unless another '&'
         //  preceded it.
         //
         if(!curr.name.empty())
         {
            names.push_back(curr);
            curr = NameAndPtrs();
         }
         break;

      case '[':
         //
         //  This template argument has an array tag (for example, in a
         //  unique_ptr[] specialization).
         //
         ++curr.ptrs;
         break;

      case ']':
         //
         //  This is the end of an array tag.
         //
         break;

      case 'o':
         //
         //  Look for the keyword "operator", which can be used in a function
         //  template.  It must be the first (perhaps qualified) name in NAME.
         //
         //  NOTE: This has not been tested.  Nothing in the code base caused
         //  ====  its execution at the time it was written.
         //
         if((i == 0) || (name[i - 1] == ':'))
         {
            if((depth == 0) && (name.find(OPERATOR_STR, i) == i))
            {
               char op = SPACE;
               size_t j = i + strlen(OPERATOR_STR);

               for(NO_OP; j < name.size(); ++j)
               {
                  if(ValidOpChars.find(name.at(j)) == string::npos) break;
                  op = name.at(j);
               }

               //  If no operator followed "operator", it must be an identifier
               //  that simply begins with "operator".  Fall through and add C
               //  to the current name.  If an operator was found, it should be
               //  a function template, in which case OP should be a '<' that
               //  introduces a template argument.  Extract the operator's name,
               //  leaving the final '<' in place, and update I so that the '<'
               //  label will be entered the next time through the loop.
               //
               if(op != SPACE)
               {
                  if(op == '<')
                     --j;
                  else
                     Debug::SwLog(CodeTools_GetNameAndArgs, string(1, c), 0);

                  curr.name = name.substr(i, j - i + 1);
                  i = j + 1;
                  break;
               }
            }
         }
         [[fallthrough]];
      default:
         //
         //  Add C to the current name.
         //
         curr.name.push_back(c);
      }
   }

   return names;
}

//------------------------------------------------------------------------------

bool IsBlank(char c)
{
   return (WhitespaceChars.find_first_of(c) != string::npos);
}

//------------------------------------------------------------------------------

bool IsCodeFile(const string& file)
{
   Debug::ft("CodeTools.IsCodeFile");

   //  Besides the usual .h* and .c* extensions, treat a file with
   //  no extension (e.g. <iosfwd>) as a code file.
   //
   auto ext = GetFileExtension(file);
   if(ext.empty()) return true;
   if(ext == "h") return true;
   if(ext == "c") return true;
   if(ext == "hpp") return true;
   if(ext == "cpp") return true;
   if(ext == "hh") return true;
   if(ext == "cc") return true;
   if(ext == "hxx") return true;
   if(ext == "cxx") return true;
   if(ext == "h++") return true;
   if(ext == "c++") return true;
   return false;
}

//------------------------------------------------------------------------------

bool IsValidIdentifier(const string& id)
{
   Debug::ft("CodeTools.IsValidIdentifier");

   if(!CxxChar::Attrs[id.front()].validFirst) return false;

   for(size_t i = 1; i < id.size(); ++i)
   {
      if(!CxxChar::Attrs[id.at(i)].validNext) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

bool IsWordChar(char c)
{
   return (ValidNextChars.find_first_of(c) != string::npos);
}

//------------------------------------------------------------------------------

char LastCodeChar(const string& s, size_t slashSlashPos)
{
   if(slashSlashPos == string::npos) return s.back();

   auto pos = rfind_first_not_of(s, WhitespaceChars, slashSlashPos - 1);
   return s.at(pos);
}

//------------------------------------------------------------------------------

size_t NameCouldReferTo(const string& fqName, const string& name)
{
   Debug::ft("CodeTools.NameCouldReferTo");

   //  NAME must match a tail portion (or all of) fqName.  On a partial match,
   //  check that the match directly followed a scope operator.  Finally, if
   //  fqName has the form "Scope::Class::Class", a constructor call isn't to
   //  "Class::Class", but simply to "Class", so back up to "Scope::".
   //
   auto pos1 = fqName.rfind(name);
   if(pos1 == string::npos) return string::npos;

   if(pos1 + name.size() == fqName.size())
   {
      if(pos1 == 0) return 0;

      if(fqName.compare(pos1 - 2, 2, SCOPE_STR) == 0)
      {
         auto className = fqName.substr(0, pos1 - 2);
         auto pos2 = NameCouldReferTo(className, name);
         return (pos2 != string::npos ? pos2 : pos1);
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

static bool NextNonBlankIs(string& code, size_t pos, char c)
{
   Debug::ft("CodeTools.NextNonBlankIs");

   for(auto i = pos; i < code.size(); ++i)
   {
      if(WhitespaceChars.find(code[i]) == string::npos)
      {
         return (code[i] == c);
      }
   }

   return false;
}

//------------------------------------------------------------------------------

string Normalize(const string& name)
{
   string result;
   string next;

   //  Go through NAME, extracting each individual name and adding it
   //  to the result.  Remove any qualifying scopes before each name.
   //  Preserve angle brackets, commas, and spaces, which delimit names.
   //  It is assumed that NAME, which is internally generated, does not
   //  contain unnecessary blanks.
   //
   for(size_t i = 0; i < name.size(); ++i)
   {
      switch(name[i])
      {
      case '<':
      case '>':
      case ',':
      case SPACE:
         result += next + name[i];
         [[fallthrough]];
      case ':':
         next.clear();
         break;

      default:
         next.push_back(name[i]);
      }
   }

   result += next;
   return result;
}

//------------------------------------------------------------------------------

bool PathIncludes(const string& path, const string& dir)
{
   auto s = PATH_SEPARATOR + dir;
   auto pos = path.find(s);
   if(pos == string::npos) return false;
   if(pos == path.size() - s.size()) return true;
   return (path[pos + s.size()] == PATH_SEPARATOR);
}

//------------------------------------------------------------------------------

string& Prefix(string& scope, c_string separator)
{
   return (scope.empty() ? scope : scope.append(separator));
}

//------------------------------------------------------------------------------

string& Prefix(string&& scope, c_string separator)
{
   return (scope.empty() ? scope : scope.append(separator));
}

//------------------------------------------------------------------------------

string RemoveConsts(const string& type)
{
   Debug::ft("CodeTools.RemoveConsts");

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

string RemoveRatioParms(const string& type)
{
   Debug::ft("CodeTools.RemoveRatioParms");

   //  Remove any template parameters in occurrences of std::ratio.
   //
   auto result = type;

   for(auto rat = type.find("std::ratio"); rat < type.size();
      rat = type.find("std::ratio", rat))
   {
      rat = rat + strlen("std::ratio");
      auto lb = type.find('<', rat);
      if(lb != rat) continue;
      auto rb = type.find('>', lb);
      if(rb == string::npos) return type;
      result.erase(lb, rb - lb + 1);
   }

   return result;
}

//------------------------------------------------------------------------------

fixed_string OperatorAmpersand = "operator&";

string& RemoveRefs(string& type)
{
   Debug::ft("CodeTools.RemoveRefs");

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

string& RemoveTags(string& type)
{
   Debug::ft("CodeTools.RemoveTags");

   //  Erase any leading "const" and then any trailing ones.  When searching
   //  backwards for trailing ones, make sure that we don't erase a "const"
   //  in a template specification.
   //
   if(type.find("const ") == 0) type.erase(0, 6);

   while(true)
   {
      auto pos = type.rfind(" const");

      if((pos != string::npos) && (type.find('>', pos) == string::npos))
         type.erase(pos, 6);
      else
         break;
   }

   //  Erase trailing tags and spaces.
   //
   for(auto tags = true; tags; NO_OP)
   {
      switch(type.back())
      {
      case SPACE:
      case '*':
      case '&':
      case '[':
      case ']':
         type.pop_back();
         break;
      default:
         tags = false;
      }
   }

   return type;
}

//------------------------------------------------------------------------------

string RemoveTemplates(string&& type)
{
   Debug::ft("CodeTools.RemoveTemplates");

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

size_t Replace(string& code, const string& s1,
   const string& s2, size_t begin, size_t end, char c)
{
   Debug::ft("CodeTools.Replace");

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
         if((c == NUL) || !NextNonBlankIs(code, pos + size1, c))
         {
            code.replace(pos, size1, s2);
            pos += size2;
            if(end != string::npos) end += (size2 - size1);
            continue;
         }
      }

      pos += size1;
   }

   return end;
}

//------------------------------------------------------------------------------

size_t rfind_first_not_of(const string& str, const string& chars, size_t off)
{
   if(off == string::npos) off = str.size() - 1;

   for(NO_OP; off != string::npos; --off)
   {
      if(chars.find(str.at(off)) == string::npos) return off;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t rfind_first_of(const string& str, size_t off, const string& chars)
{
   for(NO_OP; off != string::npos; --off)
   {
      auto c = str.at(off);
      if(chars.find(c) != string::npos) return off;
   }

   return string::npos;
}
}
