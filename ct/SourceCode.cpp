//==============================================================================
//
//  SourceCode.cpp
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
#include "SourceCode.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <ios>
#include <istream>
#include <iterator>
#include <ostream>
#include <set>
#include <unordered_map>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "FunctionName.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
SourceLine::SourceLine(const string& source, size_t seqno) :
   code(source),
   line(seqno),
   depth(DEPTH_NOT_SET),
   cont(false),
   type(LineType_N)
{
   if(code.empty() || (code.back() != CRLF))
   {
      code.push_back(CRLF);
   }
}

//------------------------------------------------------------------------------

void SourceLine::Display(ostream& stream) const
{
   if(line != string::npos)
      stream << std::setw(4) << line;
   else
      stream << " new";

   stream << SPACE;

   if(depth != DEPTH_NOT_SET)
      stream << std::hex << std::setw(1) << int(depth) << std::dec;
   else
      stream << '?';

   stream << (cont ? '+' : SPACE);
   stream << LineTypeAttr::Attrs[type].symbol << SPACE;
   stream << code;
}

//==============================================================================

SourceCode::SourceCode():
   file_(nullptr),
   scanned_(false),
   slashAsterisk_(false),
   curr_(End()),
   prev_(End())
{
   Debug::ft("SourceCode.ctor");
}

//------------------------------------------------------------------------------

bool SourceCode::Advance()
{
   Debug::ft("SourceCode.Advance");

   prev_ = curr_;
   curr_ = NextPos(prev_);
   return true;
}

//------------------------------------------------------------------------------

bool SourceCode::Advance(size_t incr)
{
   Debug::ft("SourceCode.Advance(incr)");

   prev_ = curr_;
   curr_ = NextPos(prev_, incr);
   return true;
}

//------------------------------------------------------------------------------

void SourceCode::CalcDepths()
{
   Debug::ft("SourceCode.CalcDepths");

   if(scanned_) return;
   if(source_.empty()) return;

   scanned_ = true;   // only run this once
   Reset();           // start from the beginning of source_

   auto ns = false;   // set when "namespace" keyword is encountered
   auto en = false;   // set when "enum" keyword is encountered
   int8_t depth = 0;  // current depth for indentation
   int8_t next = 0;   // next depth for indentation
   string id;         // identifier extracted from source code

   SourceLoc start(source_.begin());  // last position whose depth was set
   SourceLoc right(source_.begin());  // right brace that matches left brace

   while(curr_.iter != source_.end())
   {
      auto c = curr_.iter->code[curr_.pos];

      switch(c)
      {
      case '{':
         //
         //  Finalize the depth of lines since START.  Comments between curr_
         //  and the next parse position will be at depth NEXT.  The { got
         //  marked as a continuation because a semicolon doesn't immediately
         //  precede it.  Fix this.  Find the matching right brace and put it
         //  at the same depth.  Increase the depth unless the { followed the
         //  keyword "namespace".
         //
         next = (ns ? depth : depth + 1);
         ns = false;
         SetDepth(start, depth, next);
         curr_.iter->cont = false;
         Advance(1);
         right = FindClosing('{', '}');
         right.iter->depth = depth;
         depth = next;
         break;

      case '}':
         //
         //  Finalize the depth of lines since START.  Comments between curr_
         //  and the next parse position will be at the depth of the }, which
         //  was set when its left brace was encountered.
         //
         next = curr_.iter->depth;
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
               curr_.iter->depth = depth - 1;
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
               //
               //  Put a preprocessor directive at depth 0 and treat it as if
               //  it ends with a semicolon so that code that follows will not
               //  be treated as a continuation.
               //
               curr_.iter->depth = 0;
               curr_.pos = curr_.iter->code.size() - 1;
               SetDepth(start, depth, depth);
               Advance(1);
               continue;

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
               curr_.iter->depth = depth - 1;
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
               curr_ = Prev(left);
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
                  curr_ = (end.iter->code[end.pos] == ',' ? end : Prev(end));
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
   curr_ = LastLoc();
   SetDepth(start, depth, depth);
   Reset();
}

//------------------------------------------------------------------------------

LineType SourceCode::ClassifyLine
   (string s, bool& cont, std::set< Warning >& warnings)
{
   Debug::ft("SourceCode.ClassifyLine(string)");

   cont = false;

   auto length = s.size();
   if(length == 0) return BlankLine;

   //  Flag the line if it is too long.
   //
   if(length > LINE_LENGTH_MAX) warnings.insert(LineLength);

   //  Flag any tabs and convert them to spaces.
   //
   for(auto pos = s.find(TAB); pos != string::npos; pos = s.find(TAB))
   {
      warnings.insert(UseOfTab);
      s[pos] = SPACE;
   }

   //  Flag and strip trailing spaces.
   //
   if(s.find_first_not_of(SPACE) == string::npos)
   {
      warnings.insert(TrailingSpace);
      return BlankLine;
   }

   while(s.rfind(SPACE) == s.size() - 1)
   {
      warnings.insert(TrailingSpace);
      s.pop_back();
   }

   //  Flag a line that is not indented a multiple of the standard, unless
   //  it begins with a comment or string literal.
   //
   if(s.empty()) return BlankLine;
   auto pos = s.find_first_not_of(SPACE);
   if(pos > 0) s.erase(0, pos);

   if(pos % INDENT_SIZE != 0)
   {
      if((s[0] != '/') && (s[0] != QUOTE)) warnings.insert(Indentation);
   }

   //  Now that the line has been reformatted, recalculate its length.
   //
   length = s.size();

   //  Look for lines that contain nothing but a brace (or brace and semicolon).
   //
   if((s[0] == '{') && (length == 1)) return OpenBrace;

   if(s[0] == '}')
   {
      if(length == 1) return CloseBrace;
      if((s[1] == ';') && (length == 2)) return CloseBraceSemicolon;
   }

   //  Classify lines that contain only a // comment.
   //
   size_t slashSlashPos = s.find(COMMENT_STR);

   if(slashSlashPos == 0)
   {
      if(length == 2) return EmptyComment;      //
      if(s[2] == '-') return SeparatorComment;  //-
      if(s[2] == '=') return SeparatorComment;  //=
      if(s[2] == '/') return SeparatorComment;  ///
      if(s[2] != SPACE) return TaggedComment;   //$ [$ != one of above]
      return TextComment;                       //  text
   }

   //  Flag a /* comment and see if it ends on the same line.
   //
   pos = FindSubstr(s, COMMENT_BEGIN_STR);

   if(pos != string::npos)
   {
      warnings.insert(UseOfSlashAsterisk);
      if(pos == 0) return SlashAsteriskComment;
   }

   //  Look for preprocessor directives (e.g. #include, #ifndef).
   //
   if(s[0] == '#')
   {
      pos = s.find(HASH_INCLUDE_STR);
      if(pos == 0) return IncludeDirective;
      return HashDirective;
   }

   //  Look for using statements.
   //
   if(s.find("using ") == 0)
   {
      cont = (LastCodeChar(s, slashSlashPos) != ';');
      return UsingStatement;
   }

   //  Look for access controls.
   //
   pos = s.find_first_not_of(WhitespaceChars);

   if(pos != string::npos)
   {
      if(s.find(PUBLIC_STR) == pos) return AccessControl;
      if(s.find(PROTECTED_STR) == pos) return AccessControl;
      if(s.find(PRIVATE_STR) == pos) return AccessControl;
   }

   //  Look for invocations of Debug::ft and its variants.
   //
   if(FindSubstr(s, "Debug::ft(") != string::npos) return DebugFt;
   if(FindSubstr(s, "Debug::ftnt(") != string::npos) return DebugFt;
   if(FindSubstr(s, "Debug::noft(") != string::npos) return DebugFt;

   //  Look for strings that provide function names for Debug::ft.  These
   //  have the format
   //    fn_name ClassName_FunctionName = "ClassName.FunctionName";
   //  with an endline after the '=' if the line would exceed LineLengthMax
   //  characters.
   //
   string type(FunctionName::TypeStr);
   type.push_back(SPACE);

   while(true)
   {
      if(s.find(type) != 0) break;
      auto begin1 = s.find_first_not_of(SPACE, type.size());
      if(begin1 == string::npos) break;
      auto under = s.find('_', begin1);
      if(under == string::npos) break;
      auto equals = s.find('=', under);
      if(equals == string::npos) break;

      if(LastCodeChar(s, slashSlashPos) == '=')
      {
         cont = true;
         return FunctionName;
      }

      auto end1 = s.find_first_not_of(ValidNextChars, under);
      if(end1 == string::npos) break;
      auto begin2 = s.find(QUOTE, equals);
      if(begin2 == string::npos) break;
      auto dot = s.find('.', begin2);
      if(dot == string::npos) break;
      auto end2 = s.find(QUOTE, dot);
      if(end2 == string::npos) break;

      auto front = under - begin1;
      if(s.substr(begin1, front) == s.substr(begin2 + 1, front))
      {
         return FunctionName;
      }
      break;
   }

   pos = FindSubstr(s, "  ");

   if(pos != string::npos)
   {
      auto next = s.find_first_not_of(SPACE, pos);

      if((next != string::npos) && (next != slashSlashPos) && (s[next] != '='))
      {
         warnings.insert(AdjacentSpaces);
      }
   }

   cont = (LastCodeChar(s, slashSlashPos) != ';');
   return CodeLine;
}

//------------------------------------------------------------------------------

LineType SourceCode::ClassifyLine(size_t n, bool& cont)
{
   Debug::ft("SourceCode.ClassifyLine(size_t)");

   //  Get the code for line N and classify it.
   //
   string s;
   if(!GetNthLine(n, s)) return LineType_N;

   std::set< Warning > warnings;
   auto type = ClassifyLine(s, cont, warnings);

   //  A line within a /* comment can be logged spuriously.
   //
   if(slashAsterisk_)
   {
      warnings.erase(Indentation);
      warnings.erase(AdjacentSpaces);
   }

   //  Log any warnings that were reported.
   //
   if(file_ != nullptr)
   {
      for(auto w = warnings.cbegin(); w != warnings.cend(); ++w)
      {
         file_->LogLine(n, *w);
      }
   }

   //  There are some things that can only be determined by knowing what
   //  happened on previous lines.  First, see if a /* comment ended.
   //
   if(slashAsterisk_)
   {
      auto pos = s.find(COMMENT_END_STR);
      if(pos != string::npos) slashAsterisk_ = false;
      return TextComment;
   }

   //  See if a /* comment began, and whether it is still open.  Note that
   //  when a /* comment is used, a line that contains code after the */
   //  is classified as a comment unless the /* occurred somewhere after
   //  the start of that line.

   if(warnings.find(UseOfSlashAsterisk) != warnings.end())
   {
      if(s.find(COMMENT_END_STR) == string::npos) slashAsterisk_ = true;
      if(s.find(COMMENT_BEGIN_STR) == 0) return SlashAsteriskComment;
   }

   return type;
}

//------------------------------------------------------------------------------

void SourceCode::ClassifyLines()
{
   Debug::ft("SourceCode.ClassifyLines");

   //  Categorize each line.  If the previous line failed to finish
   //  a using statement or function name definition, carry it over
   //  to the next line.
   //
   auto prevCont = false;
   auto prevType = LineType_N;

   for(auto s = source_.begin(); s != source_.end(); ++s)
   {
      auto currCont = false;
      auto currType = ClassifyLine(s->line, currCont);

      if(prevCont)
      {
         if((prevType != UsingStatement) && (prevType != FunctionName))
         {
            prevCont = false;
         }
      }

      s->type = (prevCont ? prevType: currType);
      s->cont = currCont;
      prevCont = currCont;
      prevType = currType;
   }

   for(auto s = source_.begin(); s != source_.end(); ++s)
   {
      auto t = s->type;

      if(LineTypeAttr::Attrs[t].isCode) break;

      if((t != EmptyComment) && (t != SlashAsteriskComment))
      {
         s->type = FileComment;
      }
   }
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::CurrChar(char& c)
{
   Debug::ft("SourceCode.CurrChar");

   c = NUL;
   if(curr_.iter == source_.end()) return End();
   c = curr_.iter->code[curr_.pos];
   return curr_;
}

//------------------------------------------------------------------------------

void SourceCode::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Base::Display(stream, prefix, options);

   stream << prefix << "scanned : " << scanned_ << CRLF;
   stream << prefix << "source  : " << CRLF;

   for(auto i = source_.begin(); i != source_.end(); ++i)
   {
      i->Display(stream);
   }
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::End()
{
   return SourceLoc(source_.end(), string::npos);
}

//------------------------------------------------------------------------------

string SourceCode::Extract(const SourceLoc& loc, size_t count)
{
   Debug::ft("SourceCode.Extract(count)");

   string s;

   for(auto i = loc.iter; ((i != source_.end()) && (count > 0)); ++i)
   {
      auto prev = s.size();
      s += i->code.substr(loc.pos, count);
      count -= (s.size() - prev);
   }

   return Compress(s);
}

//------------------------------------------------------------------------------

string SourceCode::Extract(const SourceLoc& begin, const SourceLoc& end)
{
   Debug::ft("SourceCode.Extract(range)");

   string s;

   for(auto loc = begin; loc != end; loc.NextChar())
   {
      s.push_back(loc.iter->code[loc.pos]);
   }

   return s;
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::FindClosing(char lhc, char rhc, SourceLoc loc)
{
   Debug::ft("SourceCode.FindClosing");

   //  Look for the RHC that matches LHC.  Skip over comments and literals.
   //
   size_t level = 1;
   auto f = false;

   loc = NextPos(loc);

   while(loc.iter != source_.end())
   {
      auto c = loc.iter->code[loc.pos];

      if(c == rhc)
      {
         if(--level == 0) return loc;
      }
      else if(c == lhc)
      {
         ++level;
      }
      else if(c == QUOTE)
      {
         SkipStrLiteral(loc, f);
      }
      else if(c == APOSTROPHE)
      {
         SkipCharLiteral(loc);
      }

      loc = NextPos(loc, 1);
   }

   return End();
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::FindClosing(char lhc, char rhc)
{
   return FindClosing(lhc, rhc, curr_);
}

//------------------------------------------------------------------------------

Cxx::Directive SourceCode::FindDirective()
{
   Debug::ft("SourceCode.FindDirective");

   string s;

   while(curr_.iter != source_.end())
   {
      if(curr_.iter->code[curr_.pos] == '#')
         return NextDirective(s);
      else
         Reposition(FindLineEnd(curr_));
   }

   return Cxx::NIL_DIRECTIVE;
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::FindFirstOf(const std::string& targs)
{
   Debug::ft("SourceCode.FindFirstOf");

   //  Return the position of the first occurrence of a character in TARGS.
   //  Start by advancing from POS, in case it's a blank or the start of a
   //  comment.  Jump over any literals or nested expressions.
   //
   auto loc = NextPos(curr_);

   while(loc.iter != source_.end())
   {
      auto f = false;
      auto c = loc.iter->code[loc.pos];

      if(targs.find(c) != string::npos)
      {
         //  This function can be invoked to look for the colon that delimits
         //  a field width or a label, so don't stop at a colon that is part
         //  of a scope resolution operator.
         //
         if(c != ':') return loc;
         if(loc.iter->code[loc.pos + 1] != ':') return loc;
         loc = NextPos(loc, 2);
         continue;
      }

      switch(c)
      {
      case QUOTE:
         SkipStrLiteral(loc, f);
         break;
      case APOSTROPHE:
         SkipCharLiteral(loc);
         break;
      case '{':
         loc = FindClosing('{', '}', loc.NextChar());
         break;
      case '(':
         loc = FindClosing('(', ')', loc.NextChar());
         break;
      case '[':
         loc = FindClosing('[', ']', loc.NextChar());
         break;
      case '<':
      {
         auto end = SkipTemplateSpec(loc);
         if(end.iter != source_.end()) loc = end;
         break;
      }
      }

      if(loc.iter == source_.end()) return loc;
      loc = NextPos(loc, 1);
   }

   return End();
}

//------------------------------------------------------------------------------

bool SourceCode::FindIdentifier(std::string& id, bool tokenize)
{
   Debug::ft("SourceCode.FindIdentifier");

   if(tokenize) id = "$";  // returned if non-identifier found

   while(curr_.iter != source_.end())
   {
      auto f = false;
      auto c = curr_.iter->code[curr_.pos];

      switch(c)
      {
      case QUOTE:
         SkipStrLiteral(curr_, f);
         Advance(1);
         if(tokenize) return true;
         continue;

      case APOSTROPHE:
         SkipCharLiteral(curr_);
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

SourceLoc SourceCode::FindLineEnd(SourceLoc loc)
{
   Debug::ft("SourceCode.FindLineEnd");

   auto bs = false;

   for(NO_OP; loc.iter != source_.end(); loc.NextChar())
   {
      switch(loc.iter->code[loc.pos])
      {
      case CRLF:
         if(!bs) return loc;
         bs = false;
         break;

      case BACKSLASH:
         bs = !bs;
         break;
      }
   }

   return End();
}

//------------------------------------------------------------------------------

bool SourceCode::GetAccess(Cxx::Access& access)
{
   Debug::ft("SourceCode.GetAccess");

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

bool SourceCode::GetChar(uint32_t& c)
{
   Debug::ft("SourceCode.GetChar");

   if(curr_.iter == source_.end()) return false;
   c = curr_.iter->code[curr_.pos];
   curr_.NextChar();

   if(c == BACKSLASH)
   {
      //  This is an escape sequence.  The next character is
      //  taken verbatim unless it has a special meaning.
      //
      int64_t n;

      if(curr_.iter == source_.end()) return false;
      c = curr_.iter->code[curr_.pos];

      switch(c)
      {
      case '0':
      case '1':  // character's octal value
         GetOct(n);
         c = n;
         break;
      case 'x':  // character's 2-byte hex value
         curr_.NextChar();
         if(curr_.iter == source_.end()) return false;
         GetHexNum(n, 2);
         c = n;
         break;
      case 'u':  // character's 4-byte hex value
         curr_.NextChar();
         if(curr_.iter == source_.end()) return false;
         GetHexNum(n, 4);
         c = n;
         break;
      case 'U':  // character's 8-byte hex value
         curr_.NextChar();
         if(curr_.iter == source_.end()) return false;
         GetHexNum(n, 8);
         c = n;
         break;
      case 'a':
         c = 0x07;  // bell
         curr_.NextChar();
         break;
      case 'b':
         c = 0x08;  // backspace
         curr_.NextChar();
         break;
      case 'f':
         c = 0x0c;  // form feed
         curr_.NextChar();
         break;
      case 'n':
         c = 0x0a;  // line feed
         curr_.NextChar();
         break;
      case 'r':
         c = 0x0d;  // carriage return
         curr_.NextChar();
         break;
      case 't':
         c = 0x09;  // horizontal tab
         curr_.NextChar();
         break;
      case 'v':
         c = 0x0b;  // vertical tab
         curr_.NextChar();
         break;
      default:
         curr_.NextChar();
      }
   }

   return true;
}

//------------------------------------------------------------------------------

bool SourceCode::GetClassTag(Cxx::ClassTag& tag, bool type)
{
   Debug::ft("SourceCode.GetClassTag");

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

void SourceCode::GetCVTags(KeywordSet& tags)
{
   Debug::ft("SourceCode.GetCVTags");

   string str;

   while(true)
   {
      auto kwd = NextKeyword(str);

      switch(kwd)
      {
      case Cxx::CONST:
      case Cxx::VOLATILE:
         tags.insert(kwd);
         Reposition(curr_, str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

Cxx::Operator SourceCode::GetCxxOp()
{
   Debug::ft("SourceCode.GetCxxOp");

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

void SourceCode::GetDataTags(KeywordSet& tags)
{
   Debug::ft("SourceCode.GetDataTags");

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
         Reposition(curr_, str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

void SourceCode::GetDepth(size_t line, int8_t& depth, bool& cont)
{
   if(scanned_)
   {
      auto loc = GetLineStart(line);

      if(loc.iter != source_.end())
      {
         depth = loc.iter->depth;
         if(depth < 0) depth = 0;
         cont = loc.iter->cont;
         return;
      }
   }

   depth = 0;
   cont = false;
}

//------------------------------------------------------------------------------

void SourceCode::GetFloat(long double& num)
{
   Debug::ft("SourceCode.GetFloat");

   //  NUM has already been set to the value that preceded the decimal point.
   //  Any exponent is parsed after returning.
   //
   int64_t frac;
   word digits = GetInt(frac);
   if((digits == 0) || (frac == 0)) return;
   num += (frac * std::pow(10.0, -digits));
}

//------------------------------------------------------------------------------

void SourceCode::GetFuncBackTags(KeywordSet& tags)
{
   Debug::ft("SourceCode.GetFuncBackTags");

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
         Reposition(curr_, str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

void SourceCode::GetFuncFrontTags(KeywordSet& tags)
{
   Debug::ft("SourceCode.GetFuncFrontTags");

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
         Reposition(curr_, str.size());
         continue;

      default:
         return;
      }
   }
}

//------------------------------------------------------------------------------

size_t SourceCode::GetHex(int64_t& num)
{
   Debug::ft("SourceCode.GetHex");

   //  The initial '0' has already been parsed.
   //
   if(ThisCharIs('x') || ThisCharIs('X'))
   {
      return GetHexNum(num);
   }

   return 0;
}

//------------------------------------------------------------------------------

size_t SourceCode::GetHexNum(int64_t& num, size_t max)
{
   Debug::ft("SourceCode.GetHexNum");

   size_t count = 0;
   num = 0;

   while((curr_.iter != source_.end()) && (max > 0))
   {
      auto c = curr_.iter->code[curr_.pos];
      auto value = CxxChar::Attrs[c].hexValue;
      if(value < 0) return count;
      ++count;
      num <<= 4;
      num += value;
      curr_.NextChar();
      --max;
   }

   return count;
}

//------------------------------------------------------------------------------

bool SourceCode::GetIncludeFile(SourceLoc loc, string& file, bool& angle)
{
   Debug::ft("SourceCode.GetIncludeFile");

   //  Starting at LOC, skip spaces, look for a '#', skip spaces, look for
   //  "include", skip spaces, and look for "filename" or <filename> while
   //  staying on the original line.
   //
   auto line = loc.iter;
   loc = NextPos(loc);
   if(loc.iter != line) return false;
   if(loc.iter->code.find(HASH_INCLUDE_STR, loc.pos) != loc.pos) return false;
   loc = NextPos(loc, strlen(HASH_INCLUDE_STR));
   if(loc.iter != line) return false;

   char delimiter;

   switch(loc.iter->code[loc.pos])
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

   loc = Next(loc);
   auto end = loc.iter->code.find(delimiter, loc.pos);
   if(end == string::npos) return false;
   file = loc.iter->code.substr(loc.pos, end - loc.pos);
   return true;
}

//------------------------------------------------------------------------------

TagCount SourceCode::GetIndirectionLevel(char c, bool& space)
{
   Debug::ft("SourceCode.GetIndirectionLevel");

   space = false;
   if(curr_.iter == source_.end()) return 0;
   auto start = curr_;
   TagCount count = 0;
   while(NextCharIs(c)) ++count;
   space = ((count > 0) && (start.iter->code[start.pos - 1] == SPACE));
   return count;
}

//------------------------------------------------------------------------------

size_t SourceCode::GetInt(int64_t& num)
{
   Debug::ft("SourceCode.GetInt");

   size_t count = 0;
   num = 0;

   while(curr_.iter != source_.end())
   {
      auto c = curr_.iter->code[curr_.pos];
      auto value = CxxChar::Attrs[c].intValue;
      if(value < 0) return count;
      ++count;
      num *= 10;
      num += value;
      curr_.NextChar();
   }

   return count;
}

//------------------------------------------------------------------------------

string SourceCode::GetLine(const SourceLoc& loc)
{
   Debug::ft("SourceCode.GetLine");

   string text;

   if(loc.iter != source_.end())
   {
      text = loc.iter->code;
      text.insert(loc.pos, 1, '$');
   }

   return text;
}

//------------------------------------------------------------------------------

size_t SourceCode::GetLineNum(const SourceLoc& loc)
{
   if(loc.iter == source_.end()) return string::npos;
   return loc.iter->line;
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::GetLineStart(size_t line)
{
   for(auto i = source_.begin(); i != source_.end(); ++i)
   {
      if(i->line == line) return SourceLoc(i);
   }

   return End();
}

//------------------------------------------------------------------------------

LineType SourceCode::GetLineType(size_t line) const
{
   for(auto i = source_.begin(); i != source_.end(); ++i)
   {
      if(i->line == line) return i->type;
   }

   return LineType_N;
}

//------------------------------------------------------------------------------

bool SourceCode::GetName(string& name, Constraint constraint)
{
   Debug::ft("SourceCode.GetName");

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
         {
            return false;
         }
      }
   }

   name += id;
   return Advance(id.size());
}

//------------------------------------------------------------------------------

fn_name SourceCode_GetName2 = "SourceCode.GetName(oper)";

bool SourceCode::GetName(string& name, Cxx::Operator& oper)
{
   Debug::ft(SourceCode_GetName2);

   oper = Cxx::NIL_OPERATOR;
   if(!GetName(name, AnyKeyword)) return false;

   if(name == OPERATOR_STR)
   {
      if(GetOpOverride(oper)) return true;
      Debug::SwLog(SourceCode_GetName2, name, oper, false);
   }
   else
   {
      if((Cxx::Types->lower_bound(name) == Cxx::Types->cend()) &&
         (Cxx::Keywords->lower_bound(name) == Cxx::Keywords->cend()))
      {
         return true;
      }
   }

   Reposition(prev_);
   return false;
}

//------------------------------------------------------------------------------

bool SourceCode::GetNthLine(size_t n, string& s)
{
   auto loc = GetLineStart(n);

   if(loc.iter != source_.end())
   {
      s = loc.iter->code;
      s.pop_back();
      return true;
   }

   s.clear();
   return false;
}

//------------------------------------------------------------------------------

string SourceCode::GetNthLine(size_t n)
{
   string s;
   GetNthLine(n, s);
   return s;
}

//------------------------------------------------------------------------------

bool SourceCode::GetNum(TokenPtr& item)
{
   Debug::ft("SourceCode.GetNum");

   //  It is already known that the next character is a digit, so a lot of
   //  nonsense can be avoided by seeing if that digit appears alone.
   //
   auto loc = curr_;
   loc.NextChar();
   if(loc.iter == source_.end()) return false;
   auto c = loc.iter->code[loc.pos];

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
      if(c == '.') curr_.NextChar();

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

size_t SourceCode::GetOct(int64_t& num)
{
   Debug::ft("SourceCode.GetOct");

   //  The initial '0' has already been parsed.
   //
   size_t count = 0;
   num = 0;

   while(curr_.iter != source_.end())
   {
      auto c = curr_.iter->code[curr_.pos];
      auto value = CxxChar::Attrs[c].octValue;
      if(value < 0) return count;
      ++count;
      num <<= 3;
      num += value;
      curr_.NextChar();
   }

   return count;
}

//------------------------------------------------------------------------------

bool SourceCode::GetOpOverride(Cxx::Operator& oper)
{
   Debug::ft("SourceCode.GetOpOverride");

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
         curr_.pos += token.size();

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

Cxx::Operator SourceCode::GetPreOp()
{
   Debug::ft("SourceCode.GetPreOp");

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

bool SourceCode::GetTemplateSpec(string& spec)
{
   Debug::ft("SourceCode.GetTemplateSpec");

   spec.clear();
   auto end = SkipTemplateSpec(curr_);
   if(end.iter == source_.end()) return false;
   spec = Extract(curr_, end);
   return Advance(spec.size());
}

//------------------------------------------------------------------------------

bool SourceCode::Initialize(const CodeFile& file)
{
   Debug::ft("SourceCode.Initialize");

   auto input = file.InputStream();
   if(input == nullptr) return false;

   file_ = &file;
   source_.clear();

   input->clear();
   input->seekg(0);

   string str;

   for(size_t line = 0; input->peek() != EOF; ++line)
   {
      std::getline(*input, str);
      str.push_back(CRLF);
      source_.push_back(SourceLine(str, line));
   }

   input.reset();

   scanned_ = false;
   slashAsterisk_ = false;
   curr_ = SourceLoc(source_.begin());
   prev_ = SourceLoc(source_.begin());
   Advance();
   return true;
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::LastLoc()
{
   auto loc = End();
   return (source_.empty() ? loc : Prev(loc));
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::Next(const SourceLoc& loc)
{
   auto next = loc;
   if(next.iter == source_.end()) return next;
   if(++next.pos < next.iter->code.size()) return next;
   ++next.iter;
   next.pos = 0;
   return next;
}

//------------------------------------------------------------------------------

void SourceCode::NextAfter(SourceLoc loc, const string& str)
{
   while(loc.iter != source_.end())
   {
      auto pos = loc.iter->code.find(str, loc.pos);

      if(pos != string::npos)
      {
         loc.pos = pos;
         loc = NextPos(loc, str.size());
         return;
      }

      loc.NextLine();
   }
}

//------------------------------------------------------------------------------

bool SourceCode::NextCharIs(char c)
{
   Debug::ft("SourceCode.NextCharIs");

   if(curr_.iter == source_.end()) return false;
   if(curr_.iter->code[curr_.pos] != c) return false;
   return Advance(1);
}

//------------------------------------------------------------------------------

Cxx::Directive SourceCode::NextDirective(string& str)
{
   Debug::ft("SourceCode.NextDirective");

   str = NextIdentifier();
   if(str.empty()) return Cxx::NIL_DIRECTIVE;

   auto match = Cxx::Directives->lower_bound(str);
   return
      (match != Cxx::Directives->cend() ? match->second : Cxx::NIL_DIRECTIVE);
}

//------------------------------------------------------------------------------

std::string SourceCode::NextIdentifier()
{
   Debug::ft("SourceCode.NextIdentifier");

   if(curr_.iter == source_.end()) return EMPTY_STR;

   string str;
   auto loc = curr_;

   //  We assume that the code already compiles.  This means that we
   //  don't have to screen out reserved words that aren't types.
   //
   auto c = loc.iter->code[loc.pos];
   if(!CxxChar::Attrs[c].validFirst) return str;
   str += c;

   for(loc.NextChar(); loc.iter != source_.end(); loc.NextChar())
   {
      c = loc.iter->code[loc.pos];
      if(!CxxChar::Attrs[c].validNext) return str;
      str += c;
   }

   return str;
}

//------------------------------------------------------------------------------

Cxx::Keyword SourceCode::NextKeyword(string& str)
{
   Debug::ft("SourceCode.NextKeyword");

   str = NextIdentifier();
   if(str.empty()) return Cxx::NIL_KEYWORD;

   auto first = str.front();
   if(first == '#') return Cxx::HASH;
   if(first == '~') return Cxx::NVDTOR;

   auto match = Cxx::Keywords->lower_bound(str);
   return (match != Cxx::Keywords->cend() ? match->second : Cxx::NIL_KEYWORD);
}

//------------------------------------------------------------------------------

std::string SourceCode::NextOperator()
{
   Debug::ft("SourceCode.NextOperator");

   if(curr_.iter == source_.end()) return EMPTY_STR;
   string token;
   auto loc = curr_;
   auto c = loc.iter->code[loc.pos];

   while(CxxChar::Attrs[c].validOp)
   {
      token += c;
      loc.NextChar();
      c = loc.iter->code[loc.pos];
   }

   return token;
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::NextPos(const SourceLoc& start)
{
   auto loc = start;

   //  Find the next character to be parsed.
   //
   while(loc.iter != source_.end())
   {
      auto c = loc.iter->code[loc.pos];

      switch(c)
      {
      case SPACE:
      case CRLF:
      case TAB:
         //
         //  Skip these.
         //
         loc.NextChar();
         break;

      case '/':
         //
         //  See if this begins a comment (// or /*).
         //
         loc.NextChar();
         if(loc.iter == source_.end()) return End();

         switch(loc.iter->code[loc.pos])
         {
         case '/':
            //
            //  This is a // comment.  Continue on the next line.
            //
            loc.NextLine();
            if(loc.iter == source_.end()) return End();
            break;

         case '*':
            //
            //  This is a /* comment.  Continue where it ends.
            //
            loc.NextChar();
            if(loc.iter == source_.end()) return End();
            NextAfter(loc, COMMENT_END_STR);
            if(loc.iter == source_.end()) return End();
            break;

         default:
            //
            //  The / did not introduce a comment, so it is the next
            //  character of interest.
            //
            return loc.PrevChar();
         }
         break;

      case BACKSLASH:
         //
         //  See if this is a continuation of the current line.
         //
         if(loc.pos < loc.iter->code.size() - 1) return loc;
         loc.NextChar();
         break;

      default:
         return loc;
      }
   }

   return End();
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::NextPos(const SourceLoc& start, size_t skip)
{
   auto loc = start;

   while(skip > 0)
   {
      auto end = loc.iter->code.size();

      if(loc.pos + skip < end) break;

      skip -= (end - loc.pos);
      ++loc.iter;
      loc.pos = 0;
      if(loc.iter == source_.end()) return End();
   }

   loc.pos += skip;
   return NextPos(loc);
}

//------------------------------------------------------------------------------

bool SourceCode::NextStringIs(fixed_string str, bool check)
{
   Debug::ft("SourceCode.NextStringIs");

   if(curr_.iter == source_.end()) return false;

   auto size = strlen(str);
   if(curr_.iter->code.compare(curr_.pos, size, str) != 0) return false;

   SourceLoc loc(curr_.iter, curr_.pos + size);
   if(!check) return Reposition(loc);

   auto next = loc.iter->code[loc.pos];

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

   return Reposition(loc);
}

//------------------------------------------------------------------------------

string SourceCode::NextToken()
{
   Debug::ft("SourceCode.NextToken");

   auto token = NextIdentifier();
   if(!token.empty()) return token;
   return NextOperator();
}

//------------------------------------------------------------------------------

Cxx::Type SourceCode::NextType()
{
   Debug::ft("SourceCode.NextType");

   auto token = NextIdentifier();
   if(token.empty()) return Cxx::NIL_TYPE;
   auto type = Cxx::GetType(token);
   if(type != Cxx::NIL_TYPE) Advance(token.size());
   return type;
}

//------------------------------------------------------------------------------

void SourceCode::Preprocess()
{
   Debug::ft("SourceCode.Preprocess");

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
            for(size_t i = 0; i < id.size(); ++i)
            {
               curr_.iter->code[curr_.pos + i] = SPACE;
            }

            def->WasRead();
         }
      }

      Advance(id.size());
   }
}

//------------------------------------------------------------------------------

void SourceCode::PreprocessSource()
{
   Debug::ft("SourceCode.PreprocessSource");

   //  Clone this lexer to avoid having to restore it to its current state.
   //
   auto clone = *this;
   clone.Preprocess();
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::Prev(const SourceLoc& loc)
{
   auto prev = loc;

   if(prev.iter == source_.end())
   {
      --prev.iter;
      prev.pos = prev.iter->code.size() - 1;
   }
   else if((prev.pos > 0) && (prev.pos < prev.iter->code.size() - 1))
   {
      --prev.pos;
   }
   else if(prev.iter != source_.begin())
   {
      --prev.iter;
      prev.pos = prev.iter->code.size() - 1;
   }
   else
   {
      return End();
   }

   return prev;
}

//------------------------------------------------------------------------------

bool SourceCode::Reposition(const SourceLoc& loc)
{
   Debug::ft("SourceCode.Reposition");

   prev_ = loc;
   curr_ = NextPos(prev_);
   return true;
}

//------------------------------------------------------------------------------

bool SourceCode::Reposition(const SourceLoc& loc, size_t incr)
{
   Debug::ft("SourceCode.Reposition(incr)");

   prev_ = loc;
   prev_.pos += incr;
   curr_ = NextPos(prev_);
   return true;
}

//------------------------------------------------------------------------------

void SourceCode::Reset()
{
   curr_= SourceLoc(source_.begin());
}

//------------------------------------------------------------------------------

bool SourceCode::Retreat(const SourceLoc& loc)
{
   Debug::ft("SourceCode.Retreat");

   prev_ = loc;
   curr_ = loc;
   return false;
}

//------------------------------------------------------------------------------

void SourceCode::SetDepth(SourceLoc& start, int8_t depth1, int8_t depth2)
{
   //  START is the last position where a new line of code started, and curr_
   //  has finalized the depth of that code.  Each line from START to the one
   //  above the next parse position is therefore at DEPTH unless its depth
   //  has already been determined.  If there is more than one line in this
   //  range, the subsequent ones are continuations of the first.
   //
   auto begin1 = start.iter;
   auto endline1 = curr_.iter->line;
   auto begin2 = std::next(curr_.iter);
   start = NextPos(curr_, 1);
   auto end2 = start.iter;
   auto endline2 = (end2 == source_.end() ? source_.size() : end2->line);

   for(auto i = begin1; (i != source_.end()) && (i->line <= endline1); ++i)
   {
      if(i->depth == DEPTH_NOT_SET)
      {
         i->depth = depth1;
         i->cont = (i != begin1);
      }
   }

   for(auto i = begin2; (i != source_.end()) && (i->line < endline2); ++i)
   {
      if(i->depth == DEPTH_NOT_SET)
      {
         i->depth = depth2;
         i->cont = (i != begin2);
      }
   }
}

//------------------------------------------------------------------------------

bool SourceCode::Skip()
{
   Debug::ft("SourceCode.Skip");

   //  Advance to whatever follows the current line.
   //
   if(curr_.iter == source_.end()) return true;
   curr_.pos = curr_.iter->code.size() - 1;
   return Advance(1);
}

//------------------------------------------------------------------------------

void SourceCode::SkipCharLiteral(SourceLoc& loc)
{
   Debug::ft("SourceCode.SkipCharLiteral");

   //  The literal ends at the next non-escaped occurrence of an apostrophe.
   //
   for(loc.NextChar(); loc.iter != source_.end(); loc.NextChar())
   {
      auto c = loc.iter->code[loc.pos];
      if(c == APOSTROPHE) return;
      if(c == BACKSLASH) loc.NextChar();
   }

   loc = End();
}

//------------------------------------------------------------------------------

void SourceCode::SkipStrLiteral(SourceLoc& loc, bool& fragmented)
{
   Debug::ft("SourceCode.SkipStrLiteral");

   //  The literal ends at the next non-escaped occurrence of a quotation mark,
   //  unless it is followed by spaces and endlines, and then another quotation
   //  mark that continues the literal.
   //
   for(loc.NextChar(); loc.iter != source_.end(); loc.NextChar())
   {
      auto c = loc.iter->code[loc.pos];

      switch(c)
      {
      case QUOTE:
      {
         auto next = NextPos(loc, 1);
         if(next.iter == source_.end()) return;
         if(next.iter->code[next.pos] != QUOTE) return;
         fragmented = true;
         loc = next;
         break;
      }
      case BACKSLASH:
         loc.NextChar();
      }
   }

   loc = End();
}

//------------------------------------------------------------------------------

SourceLoc SourceCode::SkipTemplateSpec(SourceLoc loc)
{
   Debug::ft("SourceCode.SkipTemplateSpec");

   if(loc.iter == source_.end()) return loc;

   //  Extract the template specification, which must begin with a '<', end
   //  with a balanced '>', and contain identifiers or template punctuation.
   //
   auto c = loc.iter->code[loc.pos];
   if(c != '<') return End();
   loc.NextChar();

   size_t depth;

   for(depth = 1; ((loc.iter != source_.end()) && (depth > 0)); loc.NextChar())
   {
      c = loc.iter->code[loc.pos];

      if(ValidTemplateSpecChars.find(c) == string::npos)
      {
         return End();
      }

      if(c == '>')
         --depth;
      else if(c == '<')
         ++depth;
   }

   if(depth != 0) return End();
   return loc.PrevChar();
}

//------------------------------------------------------------------------------

bool SourceCode::ThisCharIs(char c)
{
   Debug::ft("SourceCode.ThisCharIs");

   //  If the next character is C, advance to the character that follows it.
   //
   if(curr_.iter == source_.end()) return false;
   if(curr_.iter->code[curr_.pos] != c) return false;
   curr_.NextChar();
   return true;
}
}
