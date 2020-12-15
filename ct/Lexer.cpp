//==============================================================================
//
//  Lexer.cpp
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
#include "Lexer.h"
#include <cctype>
#include <cmath>
#include <cstring>
#include <set>
#include <unordered_map>
#include <utility>
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Singleton.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
Lexer::Lexer() :
   source_(nullptr),
   size_(0),
   lines_(0),
   curr_(0),
   prev_(0),
   scanned_(false)
{
   Debug::ft("Lexer.ctor");
}

//------------------------------------------------------------------------------

bool Lexer::Advance()
{
   Debug::ft("Lexer.Advance");

   prev_ = curr_;
   curr_ = NextPos(prev_);
   return true;
}

//------------------------------------------------------------------------------

bool Lexer::Advance(size_t incr)
{
   Debug::ft("Lexer.Advance(incr)");

   prev_ = curr_;
   curr_ = NextPos(prev_ + incr);
   return true;
}

//------------------------------------------------------------------------------

void Lexer::CalcDepths()
{
   Debug::ft("Lexer.CalcDepths");

   if(scanned_) return;
   if(source_->empty()) return;

   scanned_ = true;   // only run this once
   Reposition(0);     // start from the beginning of source_

   auto ns = false;   // set when "namespace" keyword is encountered
   auto en = false;   // set when "enum" keyword is encountered
   int8_t depth = 0;  // current depth for indentation
   int8_t next = 0;   // next depth for indentation
   size_t start = 0;  // last position whose depth was set
   size_t right;      // position of right brace that matches left brace
   string id;         // identifier extracted from source code

   while(curr_ < size_)
   {
      auto c = (*source_)[curr_];

      switch(c)
      {
      case '{':
         //
         //  Finalize the depth of lines since START.  Comments between curr_
         //  and the next parse position will be at depth NEXT.  The { gets
         //  marked as a continuation because a semicolon doesn't immediately
         //  precede it.  Fix this.  Find the matching right brace and put it
         //  at the same depth.  Increase the depth unless the { followed the
         //  keyword "namespace".
         //
         next = (ns ? depth : depth + 1);
         ns = false;
         SetDepth(start, depth, next);
         line_[GetLineNum(curr_)].cont = false;
         right = FindClosing('{', '}', curr_ + 1);
         line_[GetLineNum(right)].depth = depth;
         depth = next;
         Advance(1);
         break;

      case '}':
         //
         //  Finalize the depth of lines since START.  Comments between curr_
         //  and the next parse position will be at the depth of the }, which
         //  was set when its left brace was encountered.
         //
         next = line_[GetLineNum(curr_)].depth;
         en = false;
         SetDepth(start, depth, next);
         depth = next;
         Advance(1);
         break;

      case ';':
         //
         //  Finalize the depth of lines since START unless a for statement is
         //  open.  Clear NS to handle the case "using namespace <name>".
         //
         SetDepth(start, depth, depth);
         ns = false;
         Advance(1);
         break;

      default:
         //
         //  Take operators one character at a time so as not to skip over a
         //  brace or semicolon.  If this isn't an operator character, bypass
         //  it using FindIdentifier, which also skips string and character
         //  literals.
         //
         if(ValidOpChars.find_first_of(c) != string::npos)
         {
            Advance(1);
         }
         else if(FindIdentifier(id, true))
         {
            switch(ClassifyIndent(id))
            {
            case IndentResume:
               //
               //  The parse position has already advanced to the next parse
               //  position.
               //
               continue;

            case IndentCase:
               //
               //  "default:" is also treated as a case label, but continue
               //  if the keyword is specifying a defaulted function.  Put a
               //  case label at DEPTH - 1 and treat it as if it ends with a
               //  semicolon so that the code that follows will not be seen
               //  as a continuation.
               //
               Advance(id.size());
               if(CurrChar() == ';') continue;
               curr_ = FindFirstOf(":");
               line_[GetLineNum(curr_)].depth = depth - 1;
               SetDepth(start, depth, depth);
               Advance(1);
               continue;

            case IndentFor:
               //
               //  A for statement contains semicolons, but code between the
               //  parentheses is a continuation if on a subsequent line.
               //
               Advance(id.size());

               if(NextCharIs('('))
               {
                  curr_ = FindClosing('(', ')');
                  SetDepth(start, depth, depth);
                  Advance(1);
               }
               continue;

            case IndentDirective:
            {
               //  Put a preprocessor directive at depth 0 and treat it as if
               //  it ends with a semicolon so that code that follows will not
               //  be treated as a continuation.
               //
               auto line = GetLineNum(curr_);
               line_[line].depth = 0;
               curr_ = source_->find(CRLF, curr_);
               if(curr_ == string::npos) curr_ = size_ - 1;
               SetDepth(start, depth, depth);
               Advance(1);
               continue;
            }

            case IndentControl:
            {
               //  If this keyword is not followed by a colon, it controls the
               //  visibility of a base class and can be handled like a normal
               //  identifier.  If it *is* followed by a colon, it controls the
               //  visibility of the members that follow.  Put it at DEPTH - 1
               //  and treat it as if it ends with a semicolon so that the code
               //  that follows will not be treated as a continuation.
               //
               Advance(id.size());
               if(CurrChar() != ':') continue;
               line_[GetLineNum(curr_)].depth = depth - 1;
               SetDepth(start, depth, depth);
               Advance(1);
               continue;
            }

            case IndentNamespace:
               //
               //  Set this flag to prevent indentation after the left brace.
               //
               ns = true;
               break;

            case IndentEnum:
            {
               //  Set this flag to prevent enumerators from being treated as
               //  continuations and advance to the left brace.
               //
               en = true;
               auto left = FindFirstOf("{");
               curr_ = left - 1;
               SetDepth(start, depth, depth);
               Advance(1);
               continue;
            }

            default:
               //
               //  Within an enum, don't treat enumerations as continuations,
               //  which is done by setting the depth for each enumeration as
               //  it is found and skipping to the position after each comma.
               //
               if(en)
               {
                  auto end = FindFirstOf(",}");
                  curr_ = ((*source_)[end] == ',' ? end : end - 1);
                  SetDepth(start, depth, depth);
                  Advance(1);
                  continue;
               }
            }

            Advance(id.size());
         }
      }
   }

   //  Set the depth for any remaining lines and reinitialize the lexer.
   //
   curr_ = size_ - 1;
   SetDepth(start, depth, depth);
   Reposition(0);
}

//------------------------------------------------------------------------------

size_t Lexer::CurrChar(char& c) const
{
   Debug::ft("Lexer.CurrChar");

   if(curr_ >= size_) return string::npos;
   c = (*source_)[curr_];
   return curr_;
}

//------------------------------------------------------------------------------

string Lexer::Extract(size_t pos, size_t count) const
{
   Debug::ft("Lexer.Extract");

   string s = source_->substr(pos, count);
   return Compress(s);
}

//------------------------------------------------------------------------------

size_t Lexer::FindClosing(char lhc, char rhc, size_t pos) const
{
   Debug::ft("Lexer.FindClosing");

   //  Look for the RHC that matches LHC.  Skip over comments and literals.
   //
   auto f = false;
   size_t level = 1;

   if(pos == string::npos) pos = curr_;

   pos = NextPos(pos);

   while(pos < size_)
   {
      auto c = (*source_)[pos];

      if(c == rhc)
      {
         if(--level == 0) return pos;
      }
      else if(c == lhc)
      {
         ++level;
      }
      else if(c == QUOTE)
      {
         pos = SkipStrLiteral(pos, f);
      }
      else if(c == APOSTROPHE)
      {
         pos = SkipCharLiteral(pos);
      }

      pos = NextPos(pos + 1);
   }

   return string::npos;
}

//------------------------------------------------------------------------------

void Lexer::FindCode(OptionalCode* opt, bool compile)
{
   Debug::ft("Lexer.FindCode");

   //  If the code that follows OPT is to be compiled, just return.
   //
   if(compile) return;

   //  Skip the code that follows OPT by advancing to where the compiler should
   //  resume.  This will be the next non-nested #elif, #else, or #endif.
   //
   auto begin = FindLineEnd(prev_) + 1;  // include leading blanks on first line
   auto level = 0;

   for(auto d = FindDirective(); d != Cxx::NIL_DIRECTIVE; d = FindDirective())
   {
      switch(d)
      {
      case Cxx::_IF:
      case Cxx::_IFDEF:
      case Cxx::_IFNDEF:
         ++level;
         break;

      case Cxx::_ELIF:
      case Cxx::_ELSE:
         if(level == 0) return opt->SetSkipped(begin, curr_ - 1);
         break;

      case Cxx::_ENDIF:
         if(level == 0) return opt->SetSkipped(begin, curr_ - 1);
         --level;
         break;
      }

      Reposition(FindLineEnd(curr_));
   }
}

//------------------------------------------------------------------------------

Cxx::Directive Lexer::FindDirective()
{
   Debug::ft("Lexer.FindDirective");

   string s;

   while(curr_ < size_)
   {
      if((*source_)[curr_] == '#')
         return NextDirective(s);
      else
         Reposition(FindLineEnd(curr_));
   }

   return Cxx::NIL_DIRECTIVE;
}

//------------------------------------------------------------------------------

size_t Lexer::FindFirstOf(const string& targs) const
{
   Debug::ft("Lexer.FindFirstOf");

   //  Return the position of the first occurrence of a character in TARGS.
   //  Start by advancing from POS, in case it's a blank or the start of a
   //  comment.  Jump over any literals or nested expressions.
   //
   auto pos = NextPos(curr_);
   size_t end;

   while(pos < size_)
   {
      auto f = false;
      auto c = (*source_)[pos];

      if(targs.find(c) != string::npos)
      {
         //  This function can be invoked to look for the colon that delimits
         //  a field width or a label, so don't stop at a colon that is part
         //  of a scope resolution operator.
         //
         if(c != ':') return pos;
         if((*source_)[pos + 1] != ':') return pos;
         pos = NextPos(pos + 2);
         continue;
      }

      switch(c)
      {
      case QUOTE:
         pos = SkipStrLiteral(pos, f);
         break;
      case APOSTROPHE:
         pos = SkipCharLiteral(pos);
         break;
      case '{':
         pos = FindClosing('{', '}', pos + 1);
         break;
      case '(':
         pos = FindClosing('(', ')', pos + 1);
         break;
      case '[':
         pos = FindClosing('[', ']', pos + 1);
         break;
      case '<':
         end = SkipTemplateSpec(pos);
         if(end != string::npos) pos = end;
         break;
      }

      if(pos == string::npos) return string::npos;
      pos = NextPos(pos + 1);
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::FindIdentifier(string& id, bool tokenize)
{
   Debug::ft("Lexer.FindIdentifier");

   if(tokenize) id = "$";  // returned if non-identifier found

   while(curr_ < size_)
   {
      auto f = false;
      auto c = (*source_)[curr_];

      switch(c)
      {
      case QUOTE:
         curr_ = SkipStrLiteral(curr_, f);
         Advance(1);
         if(tokenize) return true;
         continue;

      case APOSTROPHE:
         curr_ = SkipCharLiteral(curr_);
         Advance(1);
         if(tokenize) return true;
         continue;

      default:
         if(CxxChar::Attrs[c].validFirst)
         {
            id = NextIdentifier();
            return true;
         }

         if(CxxChar::Attrs[c].validOp)
         {
            if(tokenize) return true;
            id = NextOperator();
            Advance(id.size());
            continue;
         }

         if(CxxChar::Attrs[c].validInt)
         {
            TokenPtr num;

            if(GetNum(num))
            {
               num.reset();
               if(tokenize) return true;
               continue;
            }
         }

         Advance(1);
      }
   }

   return false;
}

//------------------------------------------------------------------------------

size_t Lexer::FindLineEnd(size_t pos) const
{
   Debug::ft("Lexer.FindLineEnd");

   auto bs = false;

   for(NO_OP; pos < size_; ++pos)
   {
      switch((*source_)[pos])
      {
      case CRLF:
         if(!bs) return pos;
         bs = false;
         break;

      case BACKSLASH:
         bs = !bs;
         break;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::GetAccess(Cxx::Access& access)
{
   Debug::ft("Lexer.GetAccess");

   //  <Access> = ("public" | "protected" | "private")
   //
   auto str = NextIdentifier();

   if(str.size() < strlen(PUBLIC_STR)) return false;
   else if(str == PUBLIC_STR) access = Cxx::Public;
   else if(str == PROTECTED_STR) access = Cxx::Protected;
   else if(str == PRIVATE_STR) access = Cxx::Private;
   else return false;

   return Advance(str.size());
}

//------------------------------------------------------------------------------

bool Lexer::GetChar(uint32_t& c)
{
   Debug::ft("Lexer.GetChar");

   if(curr_ >= size_) return false;
   c = (*source_)[curr_];
   ++curr_;

   if(c == BACKSLASH)
   {
      //  This is an escape sequence.  The next character is
      //  taken verbatim unless it has a special meaning.
      //
      int64_t n;

      if(curr_ >= size_) return false;
      c = (*source_)[curr_];

      switch(c)
      {
      case '0':
      case '1':  // character's octal value
         GetOct(n);
         c = n;
         break;
      case 'x':  // character's 2-byte hex value
         ++curr_;
         if(curr_ >= size_) return false;
         GetHexNum(n, 2);
         c = n;
         break;
      case 'u':  // character's 4-byte hex value
         ++curr_;
         if(curr_ >= size_) return false;
         GetHexNum(n, 4);
         c = n;
         break;
      case 'U':  // character's 8-byte hex value
         ++curr_;
         if(curr_ >= size_) return false;
         GetHexNum(n, 8);
         c = n;
         break;
      case 'a':
         c = 0x07;  // bell
         ++curr_;
         break;
      case 'b':
         c = 0x08;  // backspace
         ++curr_;
         break;
      case 'f':
         c = 0x0c;  // form feed
         ++curr_;
         break;
      case 'n':
         c = 0x0a;  // line feed
         ++curr_;
         break;
      case 'r':
         c = 0x0d;  // carriage return
         ++curr_;
         break;
      case 't':
         c = 0x09;  // horizontal tab
         ++curr_;
         break;
      case 'v':
         c = 0x0b;  // vertical tab
         ++curr_;
         break;
      default:
         ++curr_;
      }
   }

   return true;
}

//------------------------------------------------------------------------------

bool Lexer::GetClassTag(Cxx::ClassTag& tag, bool type)
{
   Debug::ft("Lexer.GetClassTag");

   //  <ClassTag> = ("class" | "struct" | "union" | "typename")
   //
   auto str = NextIdentifier();

   if(str.size() < strlen(CLASS_STR)) return false;
   else if(str == CLASS_STR) tag = Cxx::ClassType;
   else if(str == STRUCT_STR) tag = Cxx::StructType;
   else if(str == UNION_STR) tag = Cxx::UnionType;
   else if(type && (str == TYPENAME_STR)) tag = Cxx::Typename;
   else return false;

   return Advance(str.size());
}

//------------------------------------------------------------------------------

void Lexer::GetCVTags(KeywordSet& tags)
{
   Debug::ft("Lexer.GetCVTags");

   string str;

   while(true)
   {
      auto kwd = NextKeyword(str);

      switch(kwd)
      {
      case Cxx::CONST:
      case Cxx::VOLATILE:
         tags.insert(kwd);
         Reposition(curr_ + str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

Cxx::Operator Lexer::GetCxxOp()
{
   Debug::ft("Lexer.GetCxxOp");

   //  Match TOKEN to an operator.  If no match occurs, drop the last character
   //  and keep trying until no characters remain.
   //
   auto token = NextOperator();

   while(!token.empty())
   {
      auto match = Cxx::CxxOps->lower_bound(token);

      if(match != Cxx::CxxOps->cend())
      {
         Advance(token.size());
         return match->second;
      }

      if(token.empty()) break;
      token.pop_back();
   }

   return Cxx::NIL_OPERATOR;
}

//------------------------------------------------------------------------------

void Lexer::GetDataTags(KeywordSet& tags)
{
   Debug::ft("Lexer.GetDataTags");

   string str;

   while(true)
   {
      auto kwd = NextKeyword(str);

      switch(kwd)
      {
      //  "const" and "volatile" go with the type, not the data,
      //  but can still appear before the other keywords.  Barf.
      //
      case Cxx::CONST:
      case Cxx::CONSTEXPR:
      case Cxx::EXTERN:
      case Cxx::STATIC:
      case Cxx::MUTABLE:
      case Cxx::THREAD_LOCAL:
      case Cxx::VOLATILE:
         tags.insert(kwd);
         Reposition(curr_ + str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

void Lexer::GetDepth(size_t line, int8_t& depth, bool& cont) const
{
   if(!scanned_ || (line >= lines_))
   {
      depth = 0;
      cont = false;
   }
   else
   {
      depth = line_[line].depth;
      if(depth < 0) depth = 0;
      cont = line_[line].cont;
   }
}

//------------------------------------------------------------------------------

void Lexer::GetFloat(long double& num)
{
   Debug::ft("Lexer.GetFloat");

   //  NUM has already been set to the value that preceded the decimal point.
   //  Any exponent is parsed after returning.
   //
   int64_t frac;
   word digits = GetInt(frac);
   if((digits == 0) || (frac == 0)) return;
   num += (frac * std::pow(10.0, -digits));
}

//------------------------------------------------------------------------------

void Lexer::GetFuncBackTags(KeywordSet& tags)
{
   Debug::ft("Lexer.GetFuncBackTags");

   //  The only tags are "override" and "final": if present, "const"
   //  and/or "noexcept" precede them and have already been parsed.
   //
   string str;

   while(true)
   {
      auto kwd = NextKeyword(str);

      switch(kwd)
      {
      case Cxx::OVERRIDE:
      case Cxx::FINAL:
         tags.insert(kwd);
         Reposition(curr_ + str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

void Lexer::GetFuncFrontTags(KeywordSet& tags)
{
   Debug::ft("Lexer.GetFuncFrontTags");

   string str;

   while(true)
   {
      //  "const" and "volatile" apply to the return type, not the function,
      //  but can still appear before the other keywords.  Barf.
      //
      auto kwd = NextKeyword(str);

      switch(kwd)
      {
      case Cxx::CONST:
      case Cxx::VIRTUAL:
      case Cxx::STATIC:
      case Cxx::EXPLICIT:
      case Cxx::INLINE:
      case Cxx::CONSTEXPR:
      case Cxx::EXTERN:
      case Cxx::VOLATILE:
         tags.insert(kwd);
         Reposition(curr_ + str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

size_t Lexer::GetHex(int64_t& num)
{
   Debug::ft("Lexer.GetHex");

   //  The initial '0' has already been parsed.
   //
   if(ThisCharIs('x') || ThisCharIs('X'))
   {
      return GetHexNum(num);
   }

   return 0;
}

//------------------------------------------------------------------------------

size_t Lexer::GetHexNum(int64_t& num, size_t max)
{
   Debug::ft("Lexer.GetHexNum");

   size_t count = 0;
   num = 0;

   while((curr_ < size_) && (max > 0))
   {
      auto c = (*source_)[curr_];
      auto value = CxxChar::Attrs[c].hexValue;
      if(value < 0) return count;
      ++count;
      num <<= 4;
      num += value;
      ++curr_;
      --max;
   }

   return count;
}

//------------------------------------------------------------------------------

bool Lexer::GetIncludeFile(size_t pos, string& file, bool& angle) const
{
   Debug::ft("Lexer.GetIncludeFile");

   //  While staying on this line, skip spaces, look for a '#', skip spaces,
   //  look for "include", skip spaces, and look for "filename" or <filename>.
   //
   auto stop = source_->find(CRLF, pos);
   pos = NextPos(pos);
   if(pos >= stop) return false;
   if(source_->find(HASH_INCLUDE_STR, pos) != pos) return false;
   pos = NextPos(pos + strlen(HASH_INCLUDE_STR));
   if(pos >= stop) return false;

   char delimiter;

   switch((*source_)[pos])
   {
   case QUOTE:
      delimiter = QUOTE;
      angle = false;
      break;
   case '<':
      delimiter = '>';
      angle = true;
      break;
   default:
      return false;
   }

   ++pos;
   auto end = source_->find(delimiter, pos);
   if(end >= stop) return false;
   file = source_->substr(pos, end - pos);
   return true;
}

//------------------------------------------------------------------------------

TagCount Lexer::GetIndirectionLevel(char c, bool& space)
{
   Debug::ft("Lexer.GetIndirectionLevel");

   space = false;
   if(curr_ >= size_) return 0;
   auto start = curr_;
   TagCount count = 0;
   while(NextCharIs(c)) ++count;
   space = ((count > 0) && ((*source_)[start - 1] == SPACE));
   return count;
}

//------------------------------------------------------------------------------

size_t Lexer::GetInt(int64_t& num)
{
   Debug::ft("Lexer.GetInt");

   size_t count = 0;
   num = 0;

   while(curr_ < size_)
   {
      auto c = (*source_)[curr_];
      auto value = CxxChar::Attrs[c].intValue;
      if(value < 0) return count;
      ++count;
      num *= 10;
      num += value;
      ++curr_;
   }

   return count;
}

//------------------------------------------------------------------------------

string Lexer::GetLine(size_t pos) const
{
   Debug::ft("Lexer.GetLine");

   auto first = source_->rfind(CRLF, pos);
   if(first == string::npos)
      first = 0;
   else
      ++first;
   auto last = source_->find(CRLF, pos);
   auto text = source_->substr(first, last - first);
   text.insert(pos - first, 1, '$');
   return text;
}

//------------------------------------------------------------------------------

size_t Lexer::GetLineNum(size_t pos) const
{
   if(pos > size_) return string::npos;

   //  Do a binary search over the lines' starting positions.
   //
   word min = 0;
   word max = lines_ - 1;

   while(min < max)
   {
      auto mid = (min + max + 1) >> 1;

      if(line_[mid].start > pos)
         max = mid - 1;
      else
         min = mid;
   }

   return min;
}

//------------------------------------------------------------------------------

size_t Lexer::GetLineStart(size_t line) const
{
   if(line >= lines_) return string::npos;
   return line_[line].start;
}

//------------------------------------------------------------------------------

bool Lexer::GetName(string& name, Constraint constraint)
{
   Debug::ft("Lexer.GetName");

   auto id = NextIdentifier();
   if(id.empty()) return false;

   //  There are two exceptions to CONSTRAINT:
   //  o "override" and "final" are not actually keywords but are
   //    in Keywords for convenience.
   //  o NonKeyword is used to look for function names, so "operator"
   //    (which is in Keywords) must be allowed.
   //  o TypeKeyword is used to look for types, so "auto" (which is
   //    also in Keywords) must be allowed.
   //
   switch(constraint)
   {
   case NonKeyword:
      if(Cxx::Types->lower_bound(id) != Cxx::Types->cend()) return false;

      if(Cxx::Keywords->lower_bound(id) != Cxx::Keywords->cend())
      {
         if((id != OPERATOR_STR) && (id != OVERRIDE_STR) && (id != FINAL_STR))
         {
            return false;
         }
      }
      break;

   case TypeKeyword:
      if(id != AUTO_STR)
      {
         if(Cxx::Keywords->lower_bound(id) != Cxx::Keywords->cend())
            return false;
      }
   }

   name += id;
   return Advance(id.size());
}

//------------------------------------------------------------------------------

fn_name Lexer_GetName2 = "Lexer.GetName(oper)";

bool Lexer::GetName(string& name, Cxx::Operator& oper)
{
   Debug::ft(Lexer_GetName2);

   oper = Cxx::NIL_OPERATOR;
   if(!GetName(name, AnyKeyword)) return false;

   if(name == OPERATOR_STR)
   {
      if(GetOpOverride(oper)) return true;
      Debug::SwLog(Lexer_GetName2, name, oper, false);
   }
   else
   {
      if((Cxx::Types->lower_bound(name) == Cxx::Types->cend()) &&
         (Cxx::Keywords->lower_bound(name) == Cxx::Keywords->cend()))
         return true;
   }

   Reposition(prev_);
   return false;
}

//------------------------------------------------------------------------------

bool Lexer::GetNthLine(size_t n, string& s) const
{
   if(n >= lines_)
   {
      s.clear();
      return false;
   }

   auto curr = line_[n].start;

   if(n == lines_ - 1)
      s = source_->substr(curr, size_ - curr - 1);
   else
      s = source_->substr(curr, line_[n + 1].start - curr - 1);

   return true;
}

//------------------------------------------------------------------------------

string Lexer::GetNthLine(size_t n) const
{
   string s;
   GetNthLine(n, s);
   return s;
}

//------------------------------------------------------------------------------

bool Lexer::GetNum(TokenPtr& item)
{
   Debug::ft("Lexer.GetNum");

   //  It is already known that the next character is a digit, so a lot of
   //  nonsense can be avoided by seeing if that digit appears alone.
   //
   if(curr_ >= size_ - 1) return false;
   auto pos = curr_ + 1;
   auto c = (*source_)[pos];

   if(!CxxChar::Attrs[c].validInt)
   {
      IntLiteral::Tags tags(IntLiteral::DEC, false, IntLiteral::SIZE_I);
      auto value = CxxChar::Attrs[CurrChar()].intValue;
      if(value < 0) return false;
      item.reset(new IntLiteral(value, tags));
      return Advance(1);
   }

   //  It doesn't look like the integer appeared alone.
   //
   auto start = curr_;

   int64_t num = 0;
   auto radix = IntLiteral::DEC;

   do
   {
      if(NextCharIs('0'))
      {
         //  Look for a hex or octal literal.  If it isn't either of those,
         //  back up and look for an integer or floating point literal.
         //
         radix = IntLiteral::HEX;
         if(GetHex(num) > 0) break;
         radix = IntLiteral::OCT;
         if(GetOct(num) > 0) break;
         radix = IntLiteral::DEC;
         curr_ = start;
      }

      //  Look for an integer and then see if a decimal point or exponent
      //  follows it.
      //
      if(GetInt(num) == 0) return Retreat(start);
      c = CurrChar();
      if((c != '.') && (c != 'E') && (c != 'e')) break;
      if(c == '.') ++curr_;

      //  A decimal point or exponent followed the integer, so this is a
      //  floating point literal.  Get the portion after the decimal point
      //  and then handle any exponent.
      //
      long double fp = num;
      GetFloat(fp);

      FloatLiteral::Tags tags(false, FloatLiteral::SIZE_D);

      if(ThisCharIs('E') || ThisCharIs('e'))
      {
         tags.exp_ = true;
         auto sign = 1;
         if(ThisCharIs('-')) sign = -1;
         else if(ThisCharIs('+')) sign = 1;
         if(GetInt(num) == 0) return Retreat(start);
         if(sign == -1) num = -num;
         fp *= std::pow(10.0, int(num));
      }

      //  Finally, look for tags that specify a float or long double type.
      //
      if(ThisCharIs('L') || ThisCharIs('l'))
         tags.size_ = FloatLiteral::SIZE_L;
      else if(ThisCharIs('F') || ThisCharIs('f'))
         tags.size_ = FloatLiteral::SIZE_F;

      item.reset(new FloatLiteral(fp, tags));

      return true;
   }
   while(false);

   //  This is an integer literal, possibly hex or octal.  Look for tags
   //  that specified an unsigned, long, long long, or 64-bit type.
   //
   auto uns = false;
   auto size = IntLiteral::SIZE_I;

   for(auto done = false; !done; NO_OP)
   {
      done = true;

      if(ThisCharIs('U') || ThisCharIs('u'))
      {
         if(uns) return Retreat(start);
         uns = true;
         done = false;
      }

      if(ThisCharIs('L') || ThisCharIs('l'))
      {
         switch(size)
         {
         case IntLiteral::SIZE_I:
            size = IntLiteral::SIZE_L;
            break;
         case IntLiteral::SIZE_L:
            size = IntLiteral::SIZE_LL;
            break;
         default:
            return Retreat(start);
         }

         done = false;
      }
   }

   IntLiteral::Tags tags(radix, uns, size);
   item.reset(new IntLiteral(num, tags));
   return Advance();
}

//------------------------------------------------------------------------------

size_t Lexer::GetOct(int64_t& num)
{
   Debug::ft("Lexer.GetOct");

   //  The initial '0' has already been parsed.
   //
   size_t count = 0;
   num = 0;

   while(curr_ < size_)
   {
      auto c = (*source_)[curr_];
      auto value = CxxChar::Attrs[c].octValue;
      if(value < 0) return count;
      ++count;
      num <<= 3;
      num += value;
      ++curr_;
   }

   return count;
}

//------------------------------------------------------------------------------

bool Lexer::GetOpOverride(Cxx::Operator& oper)
{
   Debug::ft("Lexer.GetOpOverride");

   //  Get the next token, which is either non-alphabetic (uninterrupted
   //  punctuation) or alphabetic (which looks like an identifier).
   //
   auto token = NextToken();
   if(token.empty()) return false;

   //  An alphabetic token must immediately match an operator in the list.
   //  If a non-alphabetic token does not match any operator in the list,
   //  its last character is dropped and the list is searched again until
   //  the token eventually becomes empty.
   //
   size_t count = (isalpha(token.front()) ? 1 : token.size());

   while(count > 0)
   {
      auto match = Cxx::CxxOps->lower_bound(token);

      if(match != Cxx::CxxOps->cend())
      {
         oper = Cxx::Operator(match->second);
         curr_ += token.size();

         switch(oper)
         {
         case Cxx::OBJECT_CREATE:
         case Cxx::OBJECT_DELETE:
            //
            //  Handle operators new[] and delete[].  NextToken only
            //  returned the "new" or "delete" portion.
            //
            Advance();

            if(NextStringIs(ARRAY_STR, false))
            {
               if(oper == Cxx::OBJECT_CREATE)
                  oper = Cxx::OBJECT_CREATE_ARRAY;
               else
                  oper = Cxx::OBJECT_DELETE_ARRAY;
            }
            break;

         case Cxx::ARRAY_SUBSCRIPT:
            //
            //  The CxxOps table has this as "[" because code contains an
            //  expression before the "]".
            //
            if(!NextCharIs(']')) return false;
            break;

         case Cxx::FUNCTION_CALL:
         case Cxx::CAST:
            //
            //  The CxxOps table has this as "(" because code may contain
            //  an expression before the ")".
            //
            if(!NextCharIs(')')) return false;
            break;
         }

         return Advance();
      }

      if(count > 1)
      {
         token.pop_back();
         --count;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

Cxx::Operator Lexer::GetPreOp()
{
   Debug::ft("Lexer.GetPreOp");

   //  Match TOKEN to an operator.  If no match occurs, drop the last character
   //  and keep trying until no characters remain.
   //
   auto token = NextOperator();

   while(!token.empty())
   {
      auto match = Cxx::PreOps->lower_bound(token);

      if(match != Cxx::PreOps->cend())
      {
         Advance(token.size());
         return match->second;
      }

      if(token.empty()) break;
      token.pop_back();
   }

   return Cxx::NIL_OPERATOR;
}

//------------------------------------------------------------------------------

Cxx::Operator Lexer::GetReserved(const string& name)
{
   Debug::ft("Lexer.GetReserved");

   //  See if NAME matches one of a selected group of reserved words.
   //
   auto match = Cxx::Reserved->lower_bound(name);
   if(match != Cxx::Reserved->cend()) return match->second;
   return Cxx::NIL_OPERATOR;
}

//------------------------------------------------------------------------------

bool Lexer::GetTemplateSpec(string& spec)
{
   Debug::ft("Lexer.GetTemplateSpec");

   spec.clear();
   auto end = SkipTemplateSpec(curr_);
   if(end == string::npos) return false;
   spec = source_->substr(curr_, end - curr_ + 1);
   return Advance(spec.size());
}

//------------------------------------------------------------------------------

Cxx::Type Lexer::GetType(const string& name)
{
   Debug::ft("Lexer.GetType");

   auto match = Cxx::Types->lower_bound(name);
   if(match != Cxx::Types->cend()) return match->second;
   return Cxx::NIL_TYPE;
}

//------------------------------------------------------------------------------

void Lexer::Initialize(const string& source)
{
   Debug::ft("Lexer.Initialize");

   source_ = &source;
   size_ = source_->size();
   lines_ = 0;
   line_.clear();
   curr_ = 0;
   prev_ = 0;
   scanned_ = false;
   if(size_ == 0) return;

   for(size_t n = 0; n < size_; ++n)
   {
      if((*source_)[n] == CRLF) ++lines_;
   }

   if(source_->back() != CRLF) ++lines_;

   for(size_t n = 0, pos = 0; n < lines_; ++n)
   {
      line_.push_back(LineInfo(pos));
      pos = source_->find(CRLF, pos) + 1;
   }

   Advance();
}

//------------------------------------------------------------------------------

bool Lexer::NextCharIs(char c)
{
   Debug::ft("Lexer.NextCharIs");

   if((curr_ >= size_) || ((*source_)[curr_] != c)) return false;
   return Advance(1);
}

//------------------------------------------------------------------------------

Cxx::Directive Lexer::NextDirective(string& str) const
{
   Debug::ft("Lexer.NextDirective");

   str = NextIdentifier();
   if(str.empty()) return Cxx::NIL_DIRECTIVE;

   auto match = Cxx::Directives->lower_bound(str);
   return
      (match != Cxx::Directives->cend() ? match->second : Cxx::NIL_DIRECTIVE);
}

//------------------------------------------------------------------------------

string Lexer::NextIdentifier() const
{
   Debug::ft("Lexer.NextIdentifier");

   if(curr_ >= size_) return EMPTY_STR;

   string str;
   auto pos = curr_;

   //  We assume that the code already compiles.  This means that we
   //  don't have to screen out reserved words that aren't types.
   //
   auto c = (*source_)[pos];
   if(!CxxChar::Attrs[c].validFirst) return str;
   str += c;

   while(++pos < size_)
   {
      c = (*source_)[pos];
      if(!CxxChar::Attrs[c].validNext) return str;
      str += c;
   }

   return str;
}

//------------------------------------------------------------------------------

Cxx::Keyword Lexer::NextKeyword(string& str) const
{
   Debug::ft("Lexer.NextKeyword");

   str = NextIdentifier();
   if(str.empty()) return Cxx::NIL_KEYWORD;

   auto first = str.front();
   if(first == '#') return Cxx::HASH;
   if(first == '~') return Cxx::NVDTOR;

   auto match = Cxx::Keywords->lower_bound(str);
   return (match != Cxx::Keywords->cend() ? match->second : Cxx::NIL_KEYWORD);
}

//------------------------------------------------------------------------------

string Lexer::NextOperator() const
{
   Debug::ft("Lexer.NextOperator");

   if(curr_ >= size_) return EMPTY_STR;
   string token;
   auto pos = curr_;
   auto c = (*source_)[pos];

   while(CxxChar::Attrs[c].validOp)
   {
      token += c;
      ++pos;
      c = (*source_)[pos];
   }

   return token;
}

//------------------------------------------------------------------------------

size_t Lexer::NextPos(size_t pos) const
{
   //  Find the next character to be parsed.
   //
   while(pos < size_)
   {
      auto c = (*source_)[pos];

      switch(c)
      {
      case SPACE:
      case CRLF:
      case TAB:
         //
         //  Skip these.
         //
         ++pos;
         break;

      case '/':
         //
         //  See if this begins a comment (// or /*).
         //
         if(++pos >= size_) return string::npos;

         switch((*source_)[pos])
         {
         case '/':
            //
            //  This is a // comment.  Continue on the next line.
            //
            pos = source_->find(CRLF, pos);
            if(pos == string::npos) return pos;
            ++pos;
            break;

         case '*':
            //
            //  This is a /* comment.  Continue where it ends.
            //
            if(++pos >= size_) return string::npos;
            pos = source_->find(COMMENT_END_STR, pos);
            if(pos == string::npos) return string::npos;
            pos += 2;
            break;

         default:
            //
            //  The / did not introduce a comment, so it is the next
            //  character of interest.
            //
            return --pos;
         }
         break;

      case BACKSLASH:
         //
         //  See if this is a continuation of the current line.
         //
         if(++pos >= size_) return string::npos;
         if((*source_)[pos] != CRLF) return pos - 1;
         ++pos;
         break;

      default:
         return pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::NextStringIs(fixed_string str, bool check)
{
   Debug::ft("Lexer.NextStringIs");

   if(curr_ >= size_) return false;

   auto size = strlen(str);
   if(source_->compare(curr_, size, str) != 0) return false;

   auto pos = curr_ + size;
   if(!check || (pos >= size_)) return Reposition(pos);

   auto next = (*source_)[pos];

   switch(next)
   {
   case SPACE:
   case CRLF:
   case TAB:
      break;
   default:
      //  If the last character in STR is valid for an identifier, the
      //  character at NEXT must not be valid in an identifier.  This
      //  check prevents an identifier that starts with a keyword from
      //  being recognized as that keyword.
      //
      if(CxxChar::Attrs[str[size - 1]].validNext)
      {
         if(CxxChar::Attrs[next].validNext) return false;
      }
   }

   return Reposition(pos);
}

//------------------------------------------------------------------------------

string Lexer::NextToken() const
{
   Debug::ft("Lexer.NextToken");

   auto token = NextIdentifier();
   if(!token.empty()) return token;
   return NextOperator();
}

//------------------------------------------------------------------------------

Cxx::Type Lexer::NextType()
{
   Debug::ft("Lexer.NextType");

   auto token = NextIdentifier();
   if(token.empty()) return Cxx::NIL_TYPE;
   auto type = GetType(token);
   if(type != Cxx::NIL_TYPE) Advance(token.size());
   return type;
}

//------------------------------------------------------------------------------

void Lexer::Preprocess()
{
   Debug::ft("Lexer.Preprocess");

   //  Keep fetching identifiers, erasing any that are #defined symbols that
   //  map to empty strings.  Skip preprocessor directives.
   //
   auto syms = Singleton< CxxSymbols >::Instance();
   auto file = Context::File();
   auto scope = Singleton< CxxRoot >::Instance()->GlobalNamespace();
   string id;

   while(FindIdentifier(id, false))
   {
      if(id.front() == '#')
      {
         Reposition(FindLineEnd(curr_));
         continue;
      }

      SymbolView view;
      auto item = syms->FindSymbol(file, scope, id, MACRO_MASK, &view);

      if(item != nullptr)
      {
         auto def = static_cast< Define* >(item);

         if(def->Empty())
         {
            auto code = const_cast< string* >(source_);
            for(size_t i = 0; i < id.size(); ++i) code->at(curr_ + i) = SPACE;
            def->WasRead();
         }
      }

      Advance(id.size());
   }
}

//------------------------------------------------------------------------------

void Lexer::PreprocessSource() const
{
   Debug::ft("Lexer.PreprocessSource");

   //  Clone this lexer to avoid having to restore it to its current state.
   //
   auto clone = *this;
   clone.Preprocess();
}

//------------------------------------------------------------------------------

bool Lexer::Reposition(size_t pos)
{
   Debug::ft("Lexer.Reposition");

   prev_ = pos;
   curr_ = NextPos(prev_);
   return true;
}

//------------------------------------------------------------------------------

bool Lexer::Retreat(size_t pos)
{
   Debug::ft("Lexer.Retreat");

   prev_ = pos;
   curr_ = pos;
   return false;
}

//------------------------------------------------------------------------------

void Lexer::SetDepth(size_t& start, int8_t depth1, int8_t depth2)
{
   //  START is the last position where a new line of code started, and curr_
   //  has finalized the depth of that code.  Each line from START to the one
   //  above the next parse position is therefore at DEPTH unless its depth
   //  has already been determined.  If there is more than one line in this
   //  range, the subsequent ones are continuations of the first.
   //
   auto begin = GetLineNum(start);
   auto mid = GetLineNum(curr_);
   start = NextPos(curr_ + 1);
   if(start == string::npos) start = size_ - 1;
   auto end = GetLineNum(start);

   for(auto i = begin; i <= mid; ++i)
   {
      if(line_[i].depth == DEPTH_NOT_SET)
      {
         line_[i].depth = depth1;
         line_[i].cont = (i != begin);
      }
   }

   for(auto i = mid + 1; i < end; ++i)
   {
      if(line_[i].depth == DEPTH_NOT_SET)
      {
         line_[i].depth = depth2;
         line_[i].cont = (i != mid + 1);
      }
   }
}

//------------------------------------------------------------------------------

bool Lexer::Skip()
{
   Debug::ft("Lexer.Skip");

   //  Advance to whatever follows the current line.
   //
   if(curr_ >= size_) return true;
   curr_ = source_->find(CRLF, curr_);
   return Advance(1);
}

//------------------------------------------------------------------------------

size_t Lexer::SkipCharLiteral(size_t pos) const
{
   Debug::ft("Lexer.SkipCharLiteral");

   //  The literal ends at the next non-escaped occurrence of an apostrophe.
   //
   while(++pos < size_)
   {
      auto c = (*source_)[pos];
      if(c == APOSTROPHE) return pos;
      if(c == BACKSLASH) ++pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Lexer::SkipStrLiteral(size_t pos, bool& fragmented) const
{
   Debug::ft("Lexer.SkipStrLiteral");

   //  The literal ends at the next non-escaped occurrence of a quotation mark,
   //  unless it is followed by spaces and endlines, and then another quotation
   //  mark that continues the literal.
   //
   size_t next;

   while(++pos < size_)
   {
      auto c = (*source_)[pos];

      switch(c)
      {
      case QUOTE:
         next = NextPos(pos + 1);
         if(next == string::npos) return pos;
         if((*source_)[next] != QUOTE) return pos;
         fragmented = true;
         pos = next;
         break;
      case BACKSLASH:
         ++pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Lexer::SkipTemplateSpec(size_t pos) const
{
   Debug::ft("Lexer.SkipTemplateSpec");

   if(pos >= size_) return string::npos;

   //  Extract the template specification, which must begin with a '<', end
   //  with a balanced '>', and contain identifiers or template punctuation.
   //
   auto c = (*source_)[pos];
   if(c != '<') return string::npos;
   ++pos;

   size_t depth;

   for(depth = 1; ((pos < size_) && (depth > 0)); ++pos)
   {
      c = (*source_)[pos];
      if(ValidTemplateSpecChars.find(c) == string::npos) return string::npos;

      if(c == '>')
         --depth;
      else if(c == '<')
         ++depth;
   }

   if(depth != 0) return string::npos;
   return --pos;
}

//------------------------------------------------------------------------------

bool Lexer::ThisCharIs(char c)
{
   Debug::ft("Lexer.ThisCharIs");

   //  If the next character is C, advance to the character that follows it.
   //
   if((curr_ >= size_) || ((*source_)[curr_] != c)) return false;
   ++curr_;
   return true;
}
}
