//==============================================================================
//
//  Lexer.cpp
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
#include "Lexer.h"
#include <cctype>
#include <cmath>
#include <cstring>
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
Lexer::DirectiveTablePtr Lexer::Directives = nullptr;
Lexer::KeywordTablePtr Lexer::Keywords = nullptr;
Lexer::OperatorTablePtr Lexer::CxxOps = nullptr;
Lexer::OperatorTablePtr Lexer::PreOps = nullptr;
Lexer::OperatorTablePtr Lexer::Reserved = nullptr;
Lexer::TypesTablePtr Lexer::Types = nullptr;
bool Lexer::Initialized = Initialize();

//------------------------------------------------------------------------------

fn_name Lexer_ctor1 = "Lexer.ctor";

Lexer::Lexer() :
   source_(nullptr),
   size_(0),
   lines_(0),
   curr_(0),
   prev_(0)
{
   Debug::ft(Lexer_ctor1);
}

//------------------------------------------------------------------------------

fn_name Lexer_Advance1 = "Lexer.Advance";

bool Lexer::Advance()
{
   Debug::ft(Lexer_Advance1);

   prev_ = curr_;
   curr_ = NextPos(prev_);
   return true;
}

//------------------------------------------------------------------------------

fn_name Lexer_Advance2 = "Lexer.Advance(incr)";

bool Lexer::Advance(size_t incr)
{
   Debug::ft(Lexer_Advance2);

   prev_ = curr_;
   curr_ = NextPos(prev_ + incr);
   return true;
}

//------------------------------------------------------------------------------

fn_name Lexer_CurrChar = "Lexer.CurrChar";

size_t Lexer::CurrChar(char& c) const
{
   Debug::ft(Lexer_CurrChar);

   if(curr_ >= size_) return string::npos;
   c = source_->at(curr_);
   return curr_;
}

//------------------------------------------------------------------------------

fn_name Lexer_Extract = "Lexer.Extract";

string Lexer::Extract(size_t pos, size_t count) const
{
   Debug::ft(Lexer_Extract);

   //  Copy S to T, converting endlines to blanks and removing multiple blanks.
   //
   string s = source_->substr(pos, count);
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

fn_name Lexer_FindClosing = "Lexer.FindClosing";

size_t Lexer::FindClosing(char lhc, char rhc, size_t pos) const
{
   Debug::ft(Lexer_FindClosing);

   //  Look for the RHC that matches LHC.  Skip over comments and literals.
   //
   auto f = false;
   size_t level = 1;

   if(pos == string::npos)
      pos = curr_;
   else
      ++pos;

   pos = NextPos(pos);

   while(pos < size_)
   {
      auto c = source_->at(pos);

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

fn_name Lexer_FindCode = "Lexer.FindCode";

void Lexer::FindCode(OptionalCode* opt, bool compile)
{
   Debug::ft(Lexer_FindCode);

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

fn_name Lexer_FindDirective = "Lexer.FindDirective";

Cxx::Directive Lexer::FindDirective()
{
   Debug::ft(Lexer_FindDirective);

   string s;

   while(curr_ < size_)
   {
      if(source_->at(curr_) == '#')
         return NextDirective(s);
      else
         Reposition(FindLineEnd(curr_));
   }

   return Cxx::NIL_DIRECTIVE;
}

//------------------------------------------------------------------------------

fn_name Lexer_FindFirstOf = "Lexer.FindFirstOf";

size_t Lexer::FindFirstOf(const string& targs) const
{
   Debug::ft(Lexer_FindFirstOf);

   //  Return the position of the first occurrence of a character in TARGS.
   //  Start by advancing from POS, in case it's a blank or the start of a
   //  comment.  Jump over any literals or nested expressions.
   //
   auto pos = NextPos(curr_);

   while(pos < size_)
   {
      auto f = false;
      auto c = source_->at(pos);

      if(targs.find(c) != string::npos)
      {
         //  This function can be invoked to look for the colon that delimits
         //  a field width or a label, so don't stop at a colon that is part
         //  of a scope resolution operator.
         //
         if(c != ':') return pos;
         if(source_->at(pos + 1) != ':') return pos;
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
         pos = FindClosing('{', '}', pos);
         break;
      case '(':
         pos = FindClosing('(', ')', pos);
         break;
      case '[':
         pos = FindClosing('[', ']', pos);
         break;
      }

      if(pos == string::npos) return string::npos;
      pos = NextPos(pos + 1);
   }

   return string::npos;
}

//------------------------------------------------------------------------------

fn_name Lexer_FindIdentifier = "Lexer.FindIdentifier";

bool Lexer::FindIdentifier(string& id)
{
   Debug::ft(Lexer_FindIdentifier);

   while(curr_ < size_)
   {
      auto f = false;
      auto c = source_->at(curr_);

      switch(c)
      {
      case QUOTE:
         curr_ = SkipStrLiteral(curr_, f);
         Advance(1);
         continue;
      case APOSTROPHE:
         curr_ = SkipCharLiteral(curr_);
         Advance(1);
         continue;
      default:
         if(CxxChar::Attrs[c].validFirst)
         {
            id = NextIdentifier();
            return true;
         }
         if(CxxChar::Attrs[c].validOp)
         {
            id = NextOperator();
            Advance(id.size());
            continue;
         }
         if(CxxChar::Attrs[c].validInt)
         {
            TokenPtr num;
            if(GetNum(num))
            {
               num.release();
               continue;
            }
         }
         Advance(1);
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Lexer_FindLineEnd = "Lexer.FindLineEnd";

size_t Lexer::FindLineEnd(size_t pos) const
{
   Debug::ft(Lexer_FindLineEnd);

   auto bs = false;

   for(NO_OP; pos < size_; ++pos)
   {
      switch(source_->at(pos))
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

fn_name Lexer_GetAccess = "Lexer.GetAccess";

bool Lexer::GetAccess(Cxx::Access& access)
{
   Debug::ft(Lexer_GetAccess);

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

fn_name Lexer_GetChar = "Lexer.GetChar";

bool Lexer::GetChar(char& c)
{
   Debug::ft(Lexer_GetChar);

   if(curr_ >= size_) return false;
   auto end = SkipCharLiteral(curr_);
   if(end == string::npos) return false;
   c = source_->at(end - 1);
   return Reposition(end + 1);
}

//------------------------------------------------------------------------------

fn_name Lexer_GetClassTag = "Lexer.GetClassTag";

bool Lexer::GetClassTag(Cxx::ClassTag& tag, bool type)
{
   Debug::ft(Lexer_GetClassTag);

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

fn_name Lexer_GetCxxOp = "Lexer.GetCxxOp";

Cxx::Operator Lexer::GetCxxOp()
{
   Debug::ft(Lexer_GetCxxOp);

   //  Match TOKEN to an operator.  If no match occurs, drop the last character
   //  and keep trying until no characters remain.
   //
   auto token = NextOperator();

   while(!token.empty())
   {
      auto match = CxxOps->lower_bound(token);

      if(match != CxxOps->cend())
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

fn_name Lexer_GetFloat = "Lexer.GetFloat";

void Lexer::GetFloat(long double& num)
{
   Debug::ft(Lexer_GetFloat);

   //  NUM has already been set to the value that preceded the decimal point.
   //  Any exponent is parsed after returning.
   //
   int64_t frac;
   int digits = GetInt(frac);
   if((digits == 0) || (frac == 0)) return;
   num += (frac * pow(10.0, -digits));
}

//------------------------------------------------------------------------------

fn_name Lexer_GetHex = "Lexer.GetHex";

size_t Lexer::GetHex(int64_t& num)
{
   Debug::ft(Lexer_GetHex);

   //  The initial '0' has already been parsed.
   //
   if(ThisCharIs('x') || ThisCharIs('X'))
   {
      size_t count = 0;
      num = 0;

      while(curr_ < size_)
      {
         auto c = source_->at(curr_);
         auto value = CxxChar::Attrs[c].hexValue;
         if(value < 0) return count;
         ++count;
         num <<= 4;
         num += value;
         ++curr_;
      }

      return count;
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Lexer_GetIncludeFile = "Lexer.GetIncludeFile";

bool Lexer::GetIncludeFile(size_t pos, string& file, bool& angle) const
{
   Debug::ft(Lexer_GetIncludeFile);

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

   switch(source_->at(pos))
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

fn_name Lexer_GetIndirectionLevel = "Lexer.GetIndirectionLevel";

TagCount Lexer::GetIndirectionLevel(char c, bool& space)
{
   Debug::ft(Lexer_GetIndirectionLevel);

   space = false;
   if(curr_ >= size_) return 0;
   auto start = curr_;
   TagCount count = 0;
   while(NextCharIs(c)) ++count;
   space = ((count > 0) && (source_->at(start - 1) == SPACE));
   return count;
}

//------------------------------------------------------------------------------

fn_name Lexer_GetInt = "Lexer.GetInt";

size_t Lexer::GetInt(int64_t& num)
{
   Debug::ft(Lexer_GetInt);

   size_t count = 0;
   num = 0;

   while(curr_ < size_)
   {
      auto c = source_->at(curr_);
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

fn_name Lexer_GetLine = "Lexer.GetLine";

string Lexer::GetLine(size_t pos) const
{
   Debug::ft(Lexer_GetLine);

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
   int min = 0;
   int max = lines_ - 1;

   while(min < max)
   {
      auto mid = (min + max + 1) >> 1;

      if(start_[mid] > pos)
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
   return start_[line];
}

//------------------------------------------------------------------------------

fn_name Lexer_GetName1 = "Lexer.GetName";

bool Lexer::GetName(string& name, Constraint constraint)
{
   Debug::ft(Lexer_GetName1);

   auto str = NextIdentifier();
   if(str.size() == 0) return false;

   //  There are two exceptions to CONSTRAINT:
   //  o NonKeyword is used to look for function names, so "operator"
   //    (which is in Keywords) must be allowed.
   //  o TypeKeyword is used to look for types, so "auto" (which is
   //    also in Keywords) must be allowed.
   //
   switch(constraint)
   {
   case NonKeyword:
      if(str != OPERATOR_STR)
      {
         if(Types->lower_bound(str) != Types->cend()) return false;
         if(Keywords->lower_bound(str) != Keywords->cend()) return false;
      }
      break;

   case TypeKeyword:
      if(str != AUTO_STR)
      {
         if(Keywords->lower_bound(str) != Keywords->cend()) return false;
      }
   }

   name += str;
   return Advance(str.size());
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
      Debug::SwErr(Lexer_GetName2, name, oper, InfoLog);
   }
   else
   {
      if((Types->lower_bound(name) == Types->cend()) &&
         (Keywords->lower_bound(name) == Keywords->cend()))
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

   auto curr = start_[n];

   if(n == lines_ - 1)
      s = source_->substr(curr, size_ - curr - 1);
   else
      s = source_->substr(curr, start_[n + 1] - curr - 1);

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

fn_name Lexer_GetNum = "Lexer.GetNum";

bool Lexer::GetNum(TokenPtr& item)
{
   Debug::ft(Lexer_GetNum);

   //  It is already known that the next character is a digit, so a lot of
   //  nonsense can be avoided by seeing if that digit appears alone.
   //
   if(curr_ >= size_ - 1) return false;
   auto pos = curr_ + 1;
   auto c = source_->at(pos);

   if(!CxxChar::Attrs[c].validInt)
   {
      IntLiteral::Tags tags;
      tags.radix_ = IntLiteral::DEC;
      tags.unsigned_ = false;
      tags.size_ = IntLiteral::SIZE_I;
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

      FloatLiteral::Tags tags;
      tags.exp_ = false;

      if(ThisCharIs('E') || ThisCharIs('e'))
      {
         tags.exp_ = true;
         auto sign = 1;
         if(ThisCharIs('-')) sign = -1;
         else if(ThisCharIs('+')) sign = 1;
         if(GetInt(num) == 0) return Retreat(start);
         if(sign == -1) num = -num;
         fp *= pow(10.0, int(num));
      }

      //  Finally, look for tags that specify a float or long double type.
      //
      if(ThisCharIs('L') || ThisCharIs('l'))
         tags.size_ = FloatLiteral::SIZE_L;
      else if(ThisCharIs('F') || ThisCharIs('f'))
         tags.size_ = FloatLiteral::SIZE_F;
      else
         tags.size_ = FloatLiteral::SIZE_D;

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

   IntLiteral::Tags tags;
   tags.radix_ = radix;
   tags.unsigned_ = uns;
   tags.size_ = size;
   item.reset(new IntLiteral(num, tags));
   return Advance();
}

//------------------------------------------------------------------------------

fn_name Lexer_GetOct = "Lexer.GetOct";

size_t Lexer::GetOct(int64_t& num)
{
   Debug::ft(Lexer_GetOct);

   //  The initial '0' has already been parsed.
   //
   size_t count = 0;
   num = 0;

   while(curr_ < size_)
   {
      auto c = source_->at(curr_);
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

fn_name Lexer_GetOpOverride = "Lexer.GetOpOverride";

bool Lexer::GetOpOverride(Cxx::Operator& oper)
{
   Debug::ft(Lexer_GetOpOverride);

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
      auto match = CxxOps->lower_bound(token);

      if(match != CxxOps->cend())
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

fn_name Lexer_GetPreOp = "Lexer.GetPreOp";

Cxx::Operator Lexer::GetPreOp()
{
   Debug::ft(Lexer_GetPreOp);

   //  Match TOKEN to an operator.  If no match occurs, drop the last character
   //  and keep trying until no characters remain.
   //
   auto token = NextOperator();

   while(!token.empty())
   {
      auto match = PreOps->lower_bound(token);

      if(match != PreOps->cend())
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

fn_name Lexer_GetReserved = "Lexer.GetReserved";

Cxx::Operator Lexer::GetReserved(const string& name)
{
   Debug::ft(Lexer_GetReserved);

   //  See if NAME matches one of a selected group of reserved words.
   //
   auto match = Reserved->lower_bound(name);
   if(match != Reserved->cend()) return match->second;
   return Cxx::NIL_OPERATOR;
}

//------------------------------------------------------------------------------

fn_name Lexer_GetStr = "Lexer.GetStr";

bool Lexer::GetStr(string& s)
{
   Debug::ft(Lexer_GetStr);

   auto frag = false;
   auto end = SkipStrLiteral(curr_, frag);
   if(end == string::npos) return false;
   ++curr_;
   s = source_->substr(curr_, end - curr_);
   if(frag) Concatenate(s);
   return Reposition(end + 1);
}

//------------------------------------------------------------------------------

fn_name Lexer_GetTemplateSpec = "Lexer.GetTemplateSpec";

bool Lexer::GetTemplateSpec(string& spec)
{
   Debug::ft(Lexer_GetTemplateSpec);

   //  Extract the template specification, which must begin with a '<', end
   //  with a balanced '>', and contain identifiers or template punctuation.
   //
   spec.clear();

   auto pos = curr_;
   if(pos >= size_) return false;

   auto c = source_->at(pos);
   if(c != '<') return false;
   spec += c;
   ++pos;

   size_t depth;

   for(depth = 1; ((pos < size_) && (depth > 0)); ++pos)
   {
      c = source_->at(pos);
      if(ValidTemplateSpecChars.find(c) == string::npos) return false;
      spec += c;

      if(c == '>')
         --depth;
      else if(c == '<')
         ++depth;
   }

   if(depth != 0) return false;
   return Advance(spec.size());
}

//------------------------------------------------------------------------------

fn_name Lexer_GetType = "Lexer.GetType";

Cxx::Type Lexer::GetType(const string& name)
{
   Debug::ft(Lexer_GetType);

   auto match = Types->lower_bound(name);
   if(match != Types->cend()) return match->second;
   return Cxx::NIL_TYPE;
}

//------------------------------------------------------------------------------

fn_name Lexer_Initialize1 = "Lexer.Initialize(startup)";

bool Lexer::Initialize()
{
   Debug::ft(Lexer_Initialize1);

   Directives.reset(new DirectiveTable);
   Directives->insert(DirectivePair(HASH_DEFINE_STR, Cxx::_DEFINE));
   Directives->insert(DirectivePair(HASH_ELIF_STR, Cxx::_ELIF));
   Directives->insert(DirectivePair(HASH_ELSE_STR, Cxx::_ELSE));
   Directives->insert(DirectivePair(HASH_ENDIF_STR, Cxx::_ENDIF));
   Directives->insert(DirectivePair(HASH_ERROR_STR, Cxx::_ERROR));
   Directives->insert(DirectivePair(HASH_IF_STR, Cxx::_IF));
   Directives->insert(DirectivePair(HASH_IFDEF_STR, Cxx::_IFDEF));
   Directives->insert(DirectivePair(HASH_IFNDEF_STR, Cxx::_IFNDEF));
   Directives->insert(DirectivePair(HASH_INCLUDE_STR, Cxx::_INCLUDE));
   Directives->insert(DirectivePair(HASH_LINE_STR, Cxx::_LINE));
   Directives->insert(DirectivePair(HASH_PRAGMA_STR, Cxx::_PRAGMA));
   Directives->insert(DirectivePair(HASH_UNDEF_STR, Cxx::_UNDEF));

   Keywords.reset(new KeywordTable);
   Keywords->insert(KeywordPair(AUTO_STR, Cxx::AUTO));
   Keywords->insert(KeywordPair(BREAK_STR, Cxx::BREAK));
   Keywords->insert(KeywordPair(CASE_STR, Cxx::CASE));
   Keywords->insert(KeywordPair(CLASS_STR, Cxx::CLASS));
   Keywords->insert(KeywordPair(CONST_STR, Cxx::CONST));
   Keywords->insert(KeywordPair(CONSTEXPR_STR, Cxx::CONSTEXPR));
   Keywords->insert(KeywordPair(CONTINUE_STR, Cxx::CONTINUE));
   Keywords->insert(KeywordPair(DEFAULT_STR, Cxx::DEFAULT));
   Keywords->insert(KeywordPair(DO_STR, Cxx::DO));
   Keywords->insert(KeywordPair(ENUM_STR, Cxx::ENUM));
   Keywords->insert(KeywordPair(EXPLICIT_STR, Cxx::EXPLICIT));
   Keywords->insert(KeywordPair(EXTERN_STR, Cxx::EXTERN));
   Keywords->insert(KeywordPair(FOR_STR, Cxx::FOR));
   Keywords->insert(KeywordPair(FRIEND_STR, Cxx::FRIEND));
   Keywords->insert(KeywordPair(IF_STR, Cxx::IF));
   Keywords->insert(KeywordPair(INLINE_STR, Cxx::INLINE));
   Keywords->insert(KeywordPair(MUTABLE_STR, Cxx::MUTABLE));
   Keywords->insert(KeywordPair(NAMESPACE_STR, Cxx::NAMESPACE));
   Keywords->insert(KeywordPair(OPERATOR_STR, Cxx::OPERATOR));
   Keywords->insert(KeywordPair(PRIVATE_STR, Cxx::PRIVATE));
   Keywords->insert(KeywordPair(PROTECTED_STR, Cxx::PROTECTED));
   Keywords->insert(KeywordPair(PUBLIC_STR, Cxx::PUBLIC));
   Keywords->insert(KeywordPair(RETURN_STR, Cxx::RETURN));
   Keywords->insert(KeywordPair(STATIC_STR, Cxx::STATIC));
   Keywords->insert(KeywordPair(STRUCT_STR, Cxx::STRUCT));
   Keywords->insert(KeywordPair(SWITCH_STR, Cxx::SWITCH));
   Keywords->insert(KeywordPair(TEMPLATE_STR, Cxx::TEMPLATE));
   Keywords->insert(KeywordPair(TRY_STR, Cxx::TRY));
   Keywords->insert(KeywordPair(TYPEDEF_STR, Cxx::TYPEDEF));
   Keywords->insert(KeywordPair(UNION_STR, Cxx::UNION));
   Keywords->insert(KeywordPair(USING_STR, Cxx::USING));
   Keywords->insert(KeywordPair(VIRTUAL_STR, Cxx::VIRTUAL));
   Keywords->insert(KeywordPair(WHILE_STR, Cxx::WHILE));

   //  Each string can only have one entry in a hash table.  If a string
   //  is ambiguous, it maps to the operator with the highest precedence,
   //  and other interpretations are commented out.  The parser resolves
   //  the ambiguity.
   //
   CxxOps.reset(new OperatorTable);
   CxxOps->insert(OperatorPair(SCOPE_STR, Cxx::SCOPE_RESOLUTION));
   CxxOps->insert(OperatorPair(".", Cxx::REFERENCE_SELECT));
   CxxOps->insert(OperatorPair("->", Cxx::POINTER_SELECT));
   CxxOps->insert(OperatorPair("[", Cxx::ARRAY_SUBSCRIPT));
   CxxOps->insert(OperatorPair("(", Cxx::FUNCTION_CALL));
   CxxOps->insert(OperatorPair("++", Cxx::POSTFIX_INCREMENT));
   CxxOps->insert(OperatorPair("--", Cxx::POSTFIX_DECREMENT));
   CxxOps->insert(OperatorPair(TYPEID_STR, Cxx::TYPE_NAME));
   CxxOps->insert(OperatorPair(CONST_CAST_STR, Cxx::CONST_CAST));
   CxxOps->insert(OperatorPair(DYNAMIC_CAST_STR, Cxx::DYNAMIC_CAST));
   CxxOps->insert(OperatorPair(REINTERPRET_CAST_STR, Cxx::REINTERPRET_CAST));
   CxxOps->insert(OperatorPair(STATIC_CAST_STR, Cxx::STATIC_CAST));
   CxxOps->insert(OperatorPair(SIZEOF_STR, Cxx::SIZEOF_TYPE));
// CxxOps->insert(OperatorPair("++", Cxx::PREFIX_INCREMENT));
// CxxOps->insert(OperatorPair("--", Cxx::PREFIX_DECREMENT));
   CxxOps->insert(OperatorPair("~", Cxx::ONES_COMPLEMENT));
   CxxOps->insert(OperatorPair("!", Cxx::LOGICAL_NOT));
   CxxOps->insert(OperatorPair("+", Cxx::UNARY_PLUS));
   CxxOps->insert(OperatorPair("-", Cxx::UNARY_MINUS));
   CxxOps->insert(OperatorPair("&", Cxx::ADDRESS_OF));
   CxxOps->insert(OperatorPair("*", Cxx::INDIRECTION));
   CxxOps->insert(OperatorPair(NEW_STR, Cxx::OBJECT_CREATE));
   CxxOps->insert(OperatorPair(NEW_ARRAY_STR, Cxx::OBJECT_CREATE_ARRAY));
   CxxOps->insert(OperatorPair(DELETE_STR, Cxx::OBJECT_DELETE));
   CxxOps->insert(OperatorPair(DELETE_ARRAY_STR, Cxx::OBJECT_DELETE_ARRAY));
// CxxOps->insert(OperatorPair("(", Cxx::CAST));
   CxxOps->insert(OperatorPair(".*", Cxx::REFERENCE_SELECT_MEMBER));
   CxxOps->insert(OperatorPair("->*", Cxx::POINTER_SELECT_MEMBER));
// CxxOps->insert(OperatorPair("*", Cxx::MULTIPLY));
   CxxOps->insert(OperatorPair("/", Cxx::DIVIDE));
   CxxOps->insert(OperatorPair("%", Cxx::MODULO));
// CxxOps->insert(OperatorPair("+", Cxx::ADD));
// CxxOps->insert(OperatorPair("-", Cxx::SUBTRACT));
   CxxOps->insert(OperatorPair("<<", Cxx::LEFT_SHIFT));
   CxxOps->insert(OperatorPair(">>", Cxx::RIGHT_SHIFT));
   CxxOps->insert(OperatorPair("<", Cxx::LESS));
   CxxOps->insert(OperatorPair("<=", Cxx::LESS_OR_EQUAL));
   CxxOps->insert(OperatorPair(">", Cxx::GREATER));
   CxxOps->insert(OperatorPair(">=", Cxx::GREATER_OR_EQUAL));
   CxxOps->insert(OperatorPair("==", Cxx::EQUALITY));
   CxxOps->insert(OperatorPair("!=", Cxx::INEQUALITY));
// CxxOps->insert(OperatorPair("&", Cxx::BITWISE_AND));
   CxxOps->insert(OperatorPair("^", Cxx::BITWISE_XOR));
   CxxOps->insert(OperatorPair("|", Cxx::BITWISE_OR));
   CxxOps->insert(OperatorPair("&&", Cxx::LOGICAL_AND));
   CxxOps->insert(OperatorPair("||", Cxx::LOGICAL_OR));
   CxxOps->insert(OperatorPair("?", Cxx::CONDITIONAL));
   CxxOps->insert(OperatorPair("=", Cxx::ASSIGN));
   CxxOps->insert(OperatorPair("*=", Cxx::MULTIPLY_ASSIGN));
   CxxOps->insert(OperatorPair("/=", Cxx::DIVIDE_ASSIGN));
   CxxOps->insert(OperatorPair("%=", Cxx::MODULO_ASSIGN));
   CxxOps->insert(OperatorPair("+=", Cxx::ADD_ASSIGN));
   CxxOps->insert(OperatorPair("-=", Cxx::SUBTRACT_ASSIGN));
   CxxOps->insert(OperatorPair("<<=", Cxx::LEFT_SHIFT_ASSIGN));
   CxxOps->insert(OperatorPair(">>=", Cxx::RIGHT_SHIFT_ASSIGN));
   CxxOps->insert(OperatorPair("&=", Cxx::BITWISE_AND_ASSIGN));
   CxxOps->insert(OperatorPair("^=", Cxx::BITWISE_XOR_ASSIGN));
   CxxOps->insert(OperatorPair("|=", Cxx::BITWISE_OR_ASSIGN));
   CxxOps->insert(OperatorPair(THROW_STR, Cxx::THROW));
   CxxOps->insert(OperatorPair(",", Cxx::STATEMENT_SEPARATOR));

   PreOps.reset(new OperatorTable);
   PreOps->insert(OperatorPair("[", Cxx::ARRAY_SUBSCRIPT));
   PreOps->insert(OperatorPair("(", Cxx::FUNCTION_CALL));
   PreOps->insert(OperatorPair(DEFINED_STR, Cxx::DEFINED));
   PreOps->insert(OperatorPair("~", Cxx::ONES_COMPLEMENT));
   PreOps->insert(OperatorPair("!", Cxx::LOGICAL_NOT));
   PreOps->insert(OperatorPair("+", Cxx::UNARY_PLUS));
   PreOps->insert(OperatorPair("-", Cxx::UNARY_MINUS));
   PreOps->insert(OperatorPair("*", Cxx::MULTIPLY));
   PreOps->insert(OperatorPair("/", Cxx::DIVIDE));
   PreOps->insert(OperatorPair("%", Cxx::MODULO));
   PreOps->insert(OperatorPair("+", Cxx::ADD));
   PreOps->insert(OperatorPair("-", Cxx::SUBTRACT));
   PreOps->insert(OperatorPair("<<", Cxx::LEFT_SHIFT));
   PreOps->insert(OperatorPair(">>", Cxx::RIGHT_SHIFT));
   PreOps->insert(OperatorPair("<", Cxx::LESS));
   PreOps->insert(OperatorPair("<=", Cxx::LESS_OR_EQUAL));
   PreOps->insert(OperatorPair(">", Cxx::GREATER));
   PreOps->insert(OperatorPair(">=", Cxx::GREATER_OR_EQUAL));
   PreOps->insert(OperatorPair("==", Cxx::EQUALITY));
   PreOps->insert(OperatorPair("!=", Cxx::INEQUALITY));
   PreOps->insert(OperatorPair("&", Cxx::BITWISE_AND));
   PreOps->insert(OperatorPair("^", Cxx::BITWISE_XOR));
   PreOps->insert(OperatorPair("|", Cxx::BITWISE_OR));
   PreOps->insert(OperatorPair("&&", Cxx::LOGICAL_AND));
   PreOps->insert(OperatorPair("||", Cxx::LOGICAL_OR));
   PreOps->insert(OperatorPair("?", Cxx::CONDITIONAL));

   Reserved.reset(new OperatorTable);
   Reserved->insert(OperatorPair(CONST_CAST_STR, Cxx::CONST_CAST));
   Reserved->insert(OperatorPair(DELETE_STR, Cxx::OBJECT_DELETE));
   Reserved->insert(OperatorPair(DYNAMIC_CAST_STR, Cxx::DYNAMIC_CAST));
   Reserved->insert(OperatorPair(FALSE_STR, Cxx::FALSE));
   Reserved->insert(OperatorPair(NEW_STR, Cxx::OBJECT_CREATE));
   Reserved->insert(OperatorPair(NULLPTR_STR, Cxx::NULLPTR));
   Reserved->insert(OperatorPair(REINTERPRET_CAST_STR, Cxx::REINTERPRET_CAST));
   Reserved->insert(OperatorPair(SIZEOF_STR, Cxx::SIZEOF_TYPE));
   Reserved->insert(OperatorPair(STATIC_CAST_STR, Cxx::STATIC_CAST));
   Reserved->insert(OperatorPair(THROW_STR, Cxx::THROW));
   Reserved->insert(OperatorPair(TRUE_STR, Cxx::TRUE));
   Reserved->insert(OperatorPair(TYPEID_STR, Cxx::TYPE_NAME));

   Types.reset(new TypesTable);
   Types->insert(TypePair(AUTO_STR, Cxx::AUTO_TYPE));
   Types->insert(TypePair(BOOL_STR, Cxx::BOOL));
   Types->insert(TypePair(CHAR_STR, Cxx::CHAR));
   Types->insert(TypePair(DOUBLE_STR, Cxx::DOUBLE));
   Types->insert(TypePair(FLOAT_STR, Cxx::FLOAT));
   Types->insert(TypePair(INT_STR, Cxx::INT));
   Types->insert(TypePair(LONG_STR, Cxx::LONG));
   Types->insert(TypePair(NULLPTR_T_STR, Cxx::NULLPTR_TYPE));
   Types->insert(TypePair(SHORT_STR, Cxx::SHORT));
   Types->insert(TypePair(SIGNED_STR, Cxx::SIGNED));
   Types->insert(TypePair(UNSIGNED_STR, Cxx::UNSIGNED));
   Types->insert(TypePair(VOID_STR, Cxx::VOID));
   Types->insert(TypePair(DELETE_STR, Cxx::NON_TYPE));
   Types->insert(TypePair(NEW_STR, Cxx::NON_TYPE));
   Types->insert(TypePair(THROW_STR, Cxx::NON_TYPE));

   return true;
}

//------------------------------------------------------------------------------

fn_name Lexer_Initialize2 = "Lexer.Initialize";

void Lexer::Initialize(const string* source)
{
   Debug::ft(Lexer_Initialize2);

   source_ = source;
   size_ = source_->size();
   lines_ = 0;
   start_.clear();
   curr_ = 0;
   prev_ = 0;
   if(size_ == 0) return;

   for(size_t n = 0; n < size_; ++n)
   {
      if(source_->at(n) == CRLF) ++lines_;
   }

   if(source_->back() != CRLF) ++lines_;

   for(size_t n = 0, pos = 0; n < lines_; ++n)
   {
      start_.push_back(pos);
      pos = source_->find(CRLF, pos) + 1;
   }

   Advance();
}

//------------------------------------------------------------------------------

fn_name Lexer_IsValidIdentifier = "Lexer.IsValidIdentifier";

bool Lexer::IsValidIdentifier(const std::string& id)
{
   Debug::ft(Lexer_IsValidIdentifier);

   if(!CxxChar::Attrs[id.front()].validFirst) return false;

   for(auto i = 1; i < id.size(); ++i)
   {
      if(!CxxChar::Attrs[id.at(i)].validNext) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Lexer_NextCharIs = "Lexer.NextCharIs";

bool Lexer::NextCharIs(char c)
{
   Debug::ft(Lexer_NextCharIs);

   if((curr_ >= size_) || (source_->at(curr_) != c)) return false;
   return Advance(1);
}

//------------------------------------------------------------------------------

fn_name Lexer_NextDirective = "Lexer.NextDirective";

Cxx::Directive Lexer::NextDirective(string& str) const
{
   Debug::ft(Lexer_NextDirective);

   str = NextIdentifier();
   if(str.empty()) return Cxx::NIL_DIRECTIVE;

   auto match = Directives->lower_bound(str);
   return (match != Directives->cend() ? match->second : Cxx::NIL_DIRECTIVE);
}

//------------------------------------------------------------------------------

fn_name Lexer_NextIdentifier = "Lexer.NextIdentifier";

string Lexer::NextIdentifier() const
{
   Debug::ft(Lexer_NextIdentifier);

   if(curr_ >= size_) return EMPTY_STR;

   string str;
   auto pos = curr_;

   //  We assume that the code already compiles.  This means that we
   //  don't have to screen out reserved words that aren't types.
   //
   auto c = source_->at(pos);
   if(!CxxChar::Attrs[c].validFirst) return str;
   str += c;

   while(++pos < size_)
   {
      c = source_->at(pos);
      if(!CxxChar::Attrs[c].validNext) return str;
      str += c;
   }

   return str;
}

//------------------------------------------------------------------------------

fn_name Lexer_NextKeyword = "Lexer.NextKeyword";

Cxx::Keyword Lexer::NextKeyword(string& str) const
{
   Debug::ft(Lexer_NextKeyword);

   str = NextIdentifier();
   if(str.empty()) return Cxx::NIL_KEYWORD;

   auto first = str.front();
   if(first == '#') return Cxx::HASH;
   if(first == '~') return Cxx::NVDTOR;

   auto match = Keywords->lower_bound(str);
   return (match != Keywords->cend() ? match->second : Cxx::NIL_KEYWORD);
}

//------------------------------------------------------------------------------

fn_name Lexer_NextOperator = "Lexer.NextOperator";

string Lexer::NextOperator() const
{
   Debug::ft(Lexer_NextOperator);

   if(curr_ >= size_) return EMPTY_STR;
   string token;
   auto pos = curr_;
   auto c = source_->at(pos);

   while(CxxChar::Attrs[c].validOp)
   {
      token += c;
      ++pos;
      c = source_->at(pos);
   }

   return token;
}

//------------------------------------------------------------------------------

fn_name Lexer_NextPos = "Lexer.NextPos";

size_t Lexer::NextPos(size_t pos) const
{
   Debug::ft(Lexer_NextPos);

   //  Find the next character to be parsed.
   //
   while(pos < size_)
   {
      auto c = source_->at(pos);

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

         switch(source_->at(pos))
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
         if(source_->at(pos) != CRLF) return string::npos;
         ++pos;
         break;

      default:
         return pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

fn_name Lexer_NextStringIs = "Lexer.NextStringIs";

bool Lexer::NextStringIs(fixed_string str, bool check)
{
   Debug::ft(Lexer_NextStringIs);

   if(curr_ >= size_) return false;

   auto size = strlen(str);
   if(source_->compare(curr_, size, str) != 0) return false;

   auto pos = curr_ + size;
   if(!check || (pos >= size_)) return Reposition(pos);

   auto next = source_->at(pos);

   switch(next)
   {
   case SPACE:
   case CRLF:
      break;
   default:
      //  If NEXT is valid in an identifier, the last character in STR
      //  must not be valid in an identifier, and vice versa.
      //
      if(CxxChar::Attrs[next].validNext ^
         CxxChar::Attrs[str[size - 1]].validNext) break;
      return false;
   }

   return Reposition(pos);
}

//------------------------------------------------------------------------------

fn_name Lexer_NextToken = "Lexer.NextToken";

string Lexer::NextToken() const
{
   Debug::ft(Lexer_NextToken);

   auto token = NextIdentifier();
   if(!token.empty()) return token;
   return NextOperator();
}

//------------------------------------------------------------------------------

fn_name Lexer_NextType = "Lexer.NextType";

Cxx::Type Lexer::NextType()
{
   Debug::ft(Lexer_NextType);

   auto token = NextIdentifier();
   if(token.empty()) return Cxx::NIL_TYPE;
   auto type = GetType(token);
   if(type != Cxx::NIL_TYPE) Advance(token.size());
   return type;
}

//------------------------------------------------------------------------------

fn_name Lexer_Preprocess = "Lexer.Preprocess";

void Lexer::Preprocess()
{
   Debug::ft(Lexer_Preprocess);

   //  Keep fetching identifiers, erasing any that are #defined symbols that
   //  map to empty strings.  Skip preprocessor directives.
   //
   auto syms = Singleton< CxxSymbols >::Instance();
   auto file = Context::File();
   auto scope = Singleton< CxxRoot >::Instance()->GlobalNamespace();
   string id;

   while(FindIdentifier(id))
   {
      if(id.front() == '#')
      {
         Reposition(FindLineEnd(curr_));
         continue;
      }

      SymbolView view;
      auto item = syms->FindSymbol(file, scope, id, MACRO_MASK, &view);  //*

      if(item != nullptr)
      {
         auto def = static_cast< Define* >(item);

         if(def->Empty())
         {
            auto code = const_cast< string* >(source_);
            for(auto i = 0; i < id.size(); ++i) code->at(curr_ + i) = SPACE;
            def->WasRead();
         }
      }

      Advance(id.size());
   }
}

//------------------------------------------------------------------------------

fn_name Lexer_PreprocessSource = "Lexer.PreprocessSource";

void Lexer::PreprocessSource() const
{
   Debug::ft(Lexer_PreprocessSource);

   //  Clone this lexer to avoid having to restore it to its current state.
   //
   auto clone = *this;
   clone.Preprocess();
}

//------------------------------------------------------------------------------

fn_name Lexer_Reposition = "Lexer.Reposition";

bool Lexer::Reposition(size_t pos)
{
   Debug::ft(Lexer_Reposition);

   prev_ = pos;
   curr_ = NextPos(prev_);
   return true;
}

//------------------------------------------------------------------------------

fn_name Lexer_Retreat = "Lexer.Retreat";

bool Lexer::Retreat(size_t pos)
{
   Debug::ft(Lexer_Retreat);

   prev_ = pos;
   curr_ = pos;
   return false;
}

//------------------------------------------------------------------------------

fn_name Lexer_Skip = "Lexer.Skip";

bool Lexer::Skip()
{
   Debug::ft(Lexer_Skip);

   //  Advance to whatever follows the current line.
   //
   if(curr_ >= size_) return true;
   curr_ = source_->find(CRLF, curr_);
   return Advance(1);
}

//------------------------------------------------------------------------------

fn_name Lexer_SkipCharLiteral = "Lexer.SkipCharLiteral";

size_t Lexer::SkipCharLiteral(size_t pos) const
{
   Debug::ft(Lexer_SkipCharLiteral);

   //  The literal ends at the next non-escaped occurrence of an apostrophe.
   //
   while(++pos < size_)
   {
      auto c = source_->at(pos);
      if(c == APOSTROPHE) return pos;
      if(c == BACKSLASH) ++pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

fn_name Lexer_SkipStrLiteral = "Lexer.SkipStrLiteral";

size_t Lexer::SkipStrLiteral(size_t pos, bool& fragmented) const
{
   Debug::ft(Lexer_SkipStrLiteral);

   //  The literal ends at the next non-escaped occurrence of a quotation mark,
   //  unless it is followed by spaces and endlines, and then another quotation
   //  mark that continues the literal.
   //
   size_t next;

   while(++pos < size_)
   {
      auto c = source_->at(pos);

      switch(c)
      {
      case QUOTE:
         next = source_->find_first_not_of(WhitespaceChars, pos + 1);
         if(next == string::npos) return pos;
         if(source_->at(next) != QUOTE) return pos;
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

fn_name Lexer_ThisCharIs = "Lexer.ThisCharIs";

bool Lexer::ThisCharIs(char c)
{
   Debug::ft(Lexer_ThisCharIs);

   //  If the next character is C, advance to the character that follows it.
   //
   if((curr_ >= size_) || (source_->at(curr_) != c)) return false;
   ++curr_;
   return true;
}
}
