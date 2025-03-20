//==============================================================================
//
//  Lexer.cpp
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
#include "Lexer.h"
#include <bitset>
#include <cctype>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <ios>
#include <memory>
#include <ostream>
#include <set>
#include <unordered_map>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxStatement.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "NbTypes.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Contexts where a left brace can appear.
//
enum LeftBraceRole
{
   LB_None,  // left brace not yet found (bottom stack entry)
   LB_Area,  // class/struct/union declaration
   LB_Func,  // function definition or block
   LB_Enum,  // enumeration
   LB_Init,  // brace initialization
   LB_Space  // namespace
};

//  Cases that govern indentation.
//
enum IndentRule
{
   IndentStandard,     // standard rules
   IndentLiteral,      // numeric/character/string literal
   IndentArea,         // class/struct/union
   IndentCase,         // case label
   IndentConditional,  // do/for/if/while
   IndentControl,      // access control
   IndentDirective,    // preprocessor directive
   IndentElse,         // else
   IndentEnum,         // enumeration
   IndentNamespace     // namespace
};

//------------------------------------------------------------------------------
//
//  Returns the indentation rule for ID, which is usually a keyword.
//
static IndentRule ClassifyIndent(string& id)
{
   switch(id.front())
   {
   case '$':
      return IndentLiteral;
   case '#':
      return IndentDirective;
   case 'c':
      if(id == CASE_STR) return IndentCase;
      if(id == CLASS_STR) return IndentArea;
      break;
   case 'p':
      if(id == PUBLIC_STR) return IndentControl;
      if(id == PRIVATE_STR) return IndentControl;
      if(id == PROTECTED_STR) return IndentControl;
      break;
   case 'i':
      if(id == IF_STR) return IndentConditional;
      break;
   case 'n':
      if(id == NAMESPACE_STR) return IndentNamespace;
      break;
   case 'f':
      if(id == FOR_STR) return IndentConditional;
      break;
   case 'd':
      if(id == DEFAULT_STR) return IndentCase;
      if(id == DO_STR) return IndentConditional;
      break;
   case 'e':
      if(id == ELSE_STR) return IndentElse;
      if(id == ENUM_STR) return IndentEnum;
      break;
   case 'w':
      if(id == WHILE_STR) return IndentConditional;
      break;
   case 's':
      if(id == STRUCT_STR) return IndentArea;
      break;
   case 'u':
      if(id == UNION_STR) return IndentArea;
      break;
   }

   return IndentStandard;
}

//------------------------------------------------------------------------------
//
//  Returns true if in a nested brace initialization.
//
static bool InNestedBraceInit(const std::vector<LeftBraceRole>& roles)
{
   Debug::ft("CodeTools.InNestedBraceInit");

   if(roles.size() < 2) return false;
   return (roles.at(roles.size() - 2) == LB_Init);
}

//==============================================================================

LineInfo::LineInfo(size_t start) :
   begin(start),
   depth(DEPTH_NOT_SET),
   ctorBraceInit(false),
   continuation(false),
   mergeable(false),
   c_comment(false),
   type(LineType_N)
{
}

//------------------------------------------------------------------------------

void LineInfo::Display(ostream& stream) const
{
   if(depth != DEPTH_NOT_SET)
      stream << std::hex << std::setw(1) << int(depth) << std::dec;
   else
      stream << '?';

   if(c_comment)
   {
      stream << "/*";
   }
   else
   {
      stream << (continuation ? '+' : SPACE);
      stream << (mergeable ? '^' : SPACE);
   }

   stream << LineTypeAttr::Attrs[type].symbol << SPACE;
}

//==============================================================================

Lexer::Lexer() :
   file_(nullptr),
   slashAsterisk_(false),
   curr_(0),
   prev_(0),
   nextLine_(0)
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

fn_name Lexer_CalcDepths = "Lexer.CalcDepths";

void Lexer::CalcDepths()
{
   Debug::ft(Lexer_CalcDepths);

   if(lines_.empty())
   {
      FindLines();
      CalcLineTypes();
   }

   string id;              // identifier extracted from code
   Cxx::Keyword kwd;       // keyword extracted from code
   auto ctor = false;      // if parsing a constructor's initialization list
   auto ternary = false;   // if parsing operator?
   int8_t currDepth = 0;   // current depth for indentation
   int8_t nextDepth = 0;   // next depth for indentation
   int8_t semiDepth = -1;  // depth to restore when reaching next semicolon
   size_t commaPos;        // position of comma that sets depths
   size_t rparPos;         // position of right parenthesis that sets depths
   bool elseif = false;    // if processing an "else if"

   std::vector<LeftBraceRole> lbStack;
   std::vector<size_t> lbDepths;
   lbStack.push_back(LB_None);
   lbDepths.push_back(SIZE_MAX);
   kwd = Cxx::NIL_KEYWORD;
   commaPos = string::npos;
   rparPos = string::npos;

   Reposition(0);
   nextLine_ = 0;

   for(auto size = code_.size(); curr_ < size; NO_OP)
   {
      auto currChar = code_[curr_];

      switch(currChar)
      {
      case '{':
      {
         //  Push this left brace's role onto the stack.
         //
         auto lbDepth = currDepth;
         auto prevChar = code_[prev_];

         if(kwd == Cxx::CLASS)
            lbStack.push_back(LB_Area);
         else if(kwd == Cxx::NAMESPACE)
            lbStack.push_back(LB_Space);
         else if(kwd == Cxx::ENUM)
            lbStack.push_back(LB_Enum);
         else if(IsBraceInit(prevChar))
            lbStack.push_back(LB_Init);
         else
            lbStack.push_back(LB_Func);

         //  Find the next depth.  Indentation usually occurs, but not always:
         //  o If the left brace immediately followed a label, the indentation
         //    remains the same, but the left brace is indented one level less
         //    (the same as the label itself).  For lbDepths, however, pretend
         //    that the left brace is at the current depth so that when the
         //    right brace pops the level off the stack, it will be correct.
         //  o After the first namespace scope, the indentation is flexible.
         //    Whether to indent is up to the source code.
         //
         if(kwd == Cxx::CASE)
         {
            if(prevChar == ':')
            {
               lbDepth = currDepth - 1;
               nextDepth = currDepth;
            }
            else
            {
               nextDepth = currDepth + 1;
            }
         }
         else if((lbStack.back() == LB_Space) && (lbStack.size() == 2))
         {
            if(NextLineIndentation(curr_) > 0)
               nextDepth = currDepth + 1;
            else
               nextDepth = currDepth;
         }
         else if(ctor && (lbStack.back() != LB_Init))
         {
            ctor = false;
            lbDepth = currDepth - 1;
         }
         else
         {
            nextDepth = currDepth + 1;
         }

         //  Save the left brace's depth and record its depth unless it is
         //  already set.  If it is not first on its line, its depth is
         //  whatever was appropriate for this line.  It is a continuation
         //  (meaning that it must be indented) if
         //  o the depth of the previous line is not set, and
         //  o the right brace is also on the same line, and
         //  o it does not appear in a nested brace initialization (because
         //    the successive elements should not be indented).
         //
         lbDepths.push_back(kwd == Cxx::CASE ? currDepth : lbDepth);

         auto currInfo = GetLineInfo(curr_);
         auto rbPos = FindClosing('{', '}', curr_ + 1);

         if(currInfo->depth == DEPTH_NOT_SET)
         {
            auto lbFirst = (LineFindFirst(curr_) == curr_);
            currInfo->depth = (lbFirst ? lbDepth : currDepth);

            auto merge = currInfo->mergeable;
            if(lbStack.back() == LB_Init) merge = false;
            if(!merge) currInfo->mergeable = false;

            auto prevInfo = GetLineInfo(PrevBegin(curr_));
            if((prevInfo != nullptr) && (prevInfo->depth == DEPTH_NOT_SET))
            {
               if(OnSameLine(curr_, rbPos) && !InNestedBraceInit(lbStack))
                  currInfo->continuation = true;
            }
         }

         //  Record a brace initialization in a member initialization list.
         //
         if(ctor && (lbStack.back() == LB_Init))
            currInfo->ctorBraceInit = true;

         //  Set the depth of the matching right brace unless it is already
         //  set (because it is on the same line as the left brace).
         //
         currInfo = GetLineInfo(rbPos);
         if(currInfo->depth == DEPTH_NOT_SET) currInfo->depth = lbDepth;

         //  Finalize the depth of lines to this point.  Comments between curr_
         //  and the next parse position will be at nextDepth.
         //
         auto merge = (lbStack.back() != LB_Init);
         if(curr_ >= commaPos) commaPos = string::npos;
         SetDepth(currDepth, nextDepth, merge);
         currDepth = nextDepth;
         kwd = Cxx::NIL_KEYWORD;
         Advance(1);
         break;
      }

      case '}':
      {
         //  Finalize the depth of lines to this point.  Comments between
         //  curr_ and the next parse position will be at the depth of the
         //  right brace, which was set when its left brace was found.
         //
         auto merge = (curr_ >= commaPos);
         if(curr_ >= commaPos) commaPos = string::npos;
         nextDepth = lbDepths.back();
         lbStack.pop_back();
         lbDepths.pop_back();
         SetDepth(currDepth, nextDepth, merge);
         currDepth = nextDepth;
         Advance(1);
         break;
      }

      case ';':
         //
         //  If a right parenthesis is pending, this semicolon appears at the
         //  beginning of a for statement, so continue.  Otherwise, finalize
         //  the depth of lines to this point.  The last keyword is then no
         //  longer in effect.  And if we were parsing an unbraced conditional,
         //  undo its indentation by returning to semiDepth.
         //
         if(rparPos == string::npos)
         {
            nextDepth = (semiDepth >= 0 ? semiDepth : currDepth);
            SetDepth(currDepth, nextDepth);
            currDepth = nextDepth;
            semiDepth = -1;
            kwd = Cxx::NIL_KEYWORD;
         }

         Advance(1);
         break;

      case ':':
         //
         //  A colon can appear
         //  (a) in a scope resolution operator
         //  (b) before a base class list
         //  (c) after operator?
         //  (d) before a field width
         //  (e) before a constructor initialization list: indent its entries
         //  (f) after a label (handled below, in IndentCase)
         //  (g) after an access control (handled below, in IndentControl)
         do
         {
            if(code_[curr_ + 1] == ':')
            {
               Advance(1);  // (a)
               break;
            }

            if(kwd == Cxx::CLASS) break;  // (b)

            if(ternary)
            {
               ternary = false;  // (c)
               break;
            }

            auto pos = FindFirstOf("{;");
            if(code_[pos] == ';') break;  // (d)

            ctor = true;
            nextDepth = currDepth + 1;
            SetDepth(currDepth, nextDepth, false);  // (e)
            currDepth = nextDepth;
         }
         while(false);

         Advance(1);
         break;

      case ')':
         //
         //  If this is the end of a parenthesized condition, finalize its
         //  depths.  If a left brace does not follow, indent the following
         //  statement and restore the current depth when the next semicolon
         //  is reached.  Unbraced conditionals can nest, so don't let a
         //  subsequent one update the depth set for the semicolon.
         //
         if(curr_ >= rparPos)
         {
            rparPos = string::npos;
            SetDepth(currDepth, currDepth);
            Advance(1);

            if(elseif)
            {
               elseif = false;
               currDepth -= 2;
            }

            if(code_[curr_] == '{') continue;
            if(semiDepth < 0) semiDepth = currDepth;
            ++currDepth;
            continue;
         }

         Advance(1);
         continue;

      case ',':
         //
         //  A comma finalizes depths to this point when the next line
         //  should not be treated as a continuation.  This occurs after
         //  o the definition of an enumerator
         //  o an item in a constructor's initialization list
         //  o an item in a brace initialization
         //  Note that commaPos is simply set to the next delimiter for
         //  these items, so it may actually be a left or right brace.
         //  The case statements that handle a brace therefore clear it.
         //
         if(curr_ >= commaPos)
         {
            commaPos = string::npos;
            SetDepth(currDepth, currDepth, false);
         }

         Advance(1);
         break;

      case '>':
         //  Cancel KWD, which could be "class" in a template parameter list.
         //
         kwd = Cxx::NIL_KEYWORD;
         [[fallthrough]];
      default:
         //
         //  Take operators one character at a time so as not to skip over a
         //  brace or semicolon.  If this isn't an operator character, bypass
         //  it using FindIdentifier, which also skips literals.
         //
         if(ValidOpChars.find_first_of(currChar) != string::npos)
         {
            if(currChar == '?') ternary = true;
            Advance(1);
         }
         else if(FindIdentifier(curr_, id, true))
         {
            auto rule = ClassifyIndent(id);

            switch(rule)
            {
            case IndentStandard:
               //
               //  The following are not preceded by a semicolon, so they would
               //  normally be treated as continuations:
               //    o enumerators
               //    o elements in a brace initialization
               //    o elements in a constructor initialization list
               //  However, each of these should be indented to the same level.
               //  Do this by setting the depth for each one as it is found and
               //  then advancing to the position that follows it, taking care
               //  not to skip over a brace.
               //
               switch(lbStack.back())
               {
               case LB_Enum:
                  if(commaPos == string::npos)
                     commaPos = FindFirstOf(",}");
                  break;

               case LB_Init:
                  if(commaPos == string::npos)
                     commaPos = FindFirstOf(",}{");
                  break;

               default:
                  if(ctor && (commaPos == string::npos))
                     commaPos = FindFirstOf(",{");
               }
               break;

            case IndentLiteral:
               //
               //  We have already advanced to the next parse position.
               //  Literals in a brace initialization arrive here, so
               //  handle them as above so they don't get marked as
               //  continuations.
               //
               if(lbStack.back() == LB_Init)
               {
                  if(commaPos == string::npos)
                     commaPos = FindFirstOf(",}{");
               }
               continue;

            case IndentCase:
               //
               //  "default:" is also treated as a case label, but continue
               //  if the keyword is specifying a defaulted function.  Put a
               //  case label at currDepth - 1 and finalize depths up to this
               //  point so that the code that follows will not be seen as a
               //  as a continuation.
               //
               kwd = Cxx::CASE;
               Advance(id.size());
               if(CurrChar() == ';') continue;
               curr_ = FindFirstOf(":");
               GetLineInfo(curr_)->depth = currDepth - 1;
               SetDepth(currDepth, currDepth);
               Advance(1);
               continue;

            case IndentConditional:
               //
               //  If a parenthesized expression follows (it does, other
               //  than at the beginning of a do-while), find the closing
               //  right parenthesis.  In an "else if", finalize the depth
               //  to this point and indent the statement that follows an
               //  extra level.
               //
               Advance(id.size());

               if(NextCharIs('('))
               {
                  if(rparPos == string::npos)
                     rparPos = FindClosing('(', ')');

                  if(elseif)
                  {
                     SetDepth(currDepth, currDepth);
                     currDepth += 2;
                  }

                  Advance(1);
               }

               continue;

            case IndentElse:
               //
               //  For "else if", continue so that the "if" can enter the
               //  IndentConditional case above and get processed as usual.
               //
               Advance(id.size());
               if(code_[curr_] == '{') continue;
               if(NextIdentifier(curr_) == IF_STR)
               {
                  if(code_.rfind(CRLF, curr_) < code_.rfind('e', curr_))
                  {
                     elseif = true;
                     continue;
                  }
               }

               //  A statement immediately follows this else.  Its depth should
               //  be increased.  But curr_ has now reached its start, so back
               //  up to the end of "else" before invoking SetDepth.  Indent and
               //  return to the current depth upon reaching the next semicolon.
               //
               curr_ = code_.rfind('e', curr_ - 1);
               SetDepth(currDepth, currDepth);
               if(semiDepth < 0) semiDepth = currDepth;
               ++currDepth;
               Advance(1);
               continue;

            case IndentDirective:
               //
               //  Put a preprocessor directive at depth 0 and finalize depths
               //  up to this point so that the code that follows will not be
               //  seen as a as a continuation.  Conditionally compiled code
               //  will only be indented if the line that follows is indented.
               //
               GetLineInfo(curr_)->depth = 0;
               curr_ = code_.find(CRLF, curr_);
               if(curr_ == string::npos) curr_ = size - 1;
               SetDepth(currDepth, currDepth);

               if(id.rfind("#if", 0) == 0)
               {
                  currDepth = NextLineIndentation(curr_);
               }

               curr_ = code_.find_first_not_of(WhitespaceChars, curr_);
               if(curr_ == string::npos) curr_ = size - 1;
               Advance();
               continue;

            case IndentControl:
               //
               //  If this keyword is not followed by a colon, it controls the
               //  visibility of a base class and can be handled like a normal
               //  identifier.  If it *is* followed by a colon, it controls the
               //  visibility of the members that follow.  Put it at DEPTH - 1
               //  and finalize depths up to this point so that the code that
               //  follows will not be seen as a as a continuation.
               //
               Advance(id.size());
               if(CurrChar() != ':') continue;
               GetLineInfo(curr_)->depth = currDepth - 1;
               SetDepth(currDepth, currDepth);
               Advance(1);
               continue;

            case IndentEnum:
               //
               //  Advance to the left brace.
               //
               kwd = Cxx::ENUM;
               SetDepth(currDepth, currDepth);
               curr_ = FindFirstOf("{");
               continue;

            case IndentNamespace:
               //
               //  Update KWD in case a left brace follows.
               //
               kwd = Cxx::NAMESPACE;
               break;

            case IndentArea:
               //
               //  Update KWD in case a left brace follows.
               //
               kwd = Cxx::CLASS;
               break;

            default:
               Debug::SwLog(Lexer_CalcDepths, "unexpected IndentRule", rule);
            }

            Advance(id.size());
         }
      }
   }

   //  Reinitialize the lexer.
   //
   Reposition(0);
}

//------------------------------------------------------------------------------

LineType Lexer::CalcLineType(size_t n, bool& cont, bool& c_comment)
{
   Debug::ft("Lexer.CalcLineType");

   //  Get the code for line N and classify it.
   //
   string s;
   if(!GetNthLine(n, s)) return LineType_N;

   auto type = CodeTools::CalcLineType(s, cont);

   //  There are some things that can only be determined by knowing what
   //  happened on previous lines.  First, see if a /* comment ended.
   //
   if(slashAsterisk_)
   {
      auto pos = FindSubstr(s, COMMENT_END_STR);
      if(pos != string::npos) slashAsterisk_ = false;
      c_comment = true;
      return TextComment;
   }

   //  See if a /* comment began and whether it is still open.  Note that
   //  when a /* comment is used, a line that contains code after the */
   //  is classified as a comment unless the /* occurred on the same line
   //  and was preceded by code.
   //
   auto pos = FindSubstr(s, COMMENT_BEGIN_STR);

   if(pos != string::npos)
   {
      if(FindSubstr(s, COMMENT_END_STR) == string::npos) slashAsterisk_ = true;

      if(pos == 0)
      {
         c_comment = true;
         return TextComment;
      }
   }

   return type;
}

//------------------------------------------------------------------------------

void Lexer::CalcLineTypes()
{
   Debug::ft("Lexer.CalcLineTypes");

   if(lines_.empty())
   {
      FindLines();
   }

   slashAsterisk_ = false;

   //  Categorize each line.  If the previous line failed to finish
   //  a using statement or function name definition, carry it over
   //  to the next line.
   //
   auto size = lines_.size();
   auto prevCont = false;
   auto prevType = LineType_N;

   for(size_t n = 0; n < size; ++n)
   {
      auto currCont = false;
      auto c_comment = false;
      auto currType = CalcLineType(n, currCont, c_comment);

      if(prevCont)
      {
         if((prevType != UsingStatement) && (prevType != FunctionName))
         {
            prevCont = false;
         }
      }

      auto& info = lines_[n];
      info.type = (prevCont ? prevType : currType);
      info.mergeable = LineTypeAttr::Attrs[info.type].isMergeable;
      info.c_comment = c_comment;

      prevCont = currCont;
      prevType = currType;
   }

   for(size_t n = 0; n < size; ++n)
   {
      auto& info = lines_[n];
      auto t = info.type;

      if(LineTypeAttr::Attrs[t].isCode) break;

      if((t != EmptyComment) && !info.c_comment)
      {
         lines_[n].type = FileComment;
      }
   }
}

//------------------------------------------------------------------------------

word Lexer::CheckDepth(size_t n) const
{
   //  Return if (a) the depth was not set, (b) the line lies within a C-style
   //  comment, or (c) the line only contains whitespace.
   //
   const auto& info = lines_[n];
   auto desired = info.depth;
   if(desired == DEPTH_NOT_SET) return -1;
   if(info.c_comment) return -1;
   auto first = code_.find_first_not_of(WhitespaceChars, info.begin);
   if(first == string::npos) return -1;
   if((n < lines_.size() - 1) && (first >= lines_[n + 1].begin)) return -1;

   //  Return if the indentation matches the desired depth.
   //
   if(info.continuation && LineTypeAttr::Attrs[info.type].isCode) ++desired;
   auto actual = first - info.begin;
   if(actual == (IndentSize() * desired)) return -1;

   //  A comment if exempt if it is not indented (probably because it
   //  comments out code) or if it is aligned with a trailing comment
   //  on the next or a previous line.
   //
   auto comment = (code_.compare(first, 2, COMMENT_STR) == 0);

   if(comment)
   {
      if(actual == 0) return -1;
      if(LineHasTrailingCommentAt(n - 1, actual)) return -1;
      if(LineHasTrailingCommentAt(n - 2, actual)) return -1;
      if(LineHasTrailingCommentAt(n + 1, actual)) return -1;
   }

   //  A string literal is exempt if it is too long to be indented or
   //  if it contains a continuation.
   //
   if(code_[first] == QUOTE)
   {
      if(LineSize(info.begin) + IndentSize() > LineLengthMax()) return -1;
      auto prev = RfindNonBlank(lines_[n].begin - 1);
      if(code_[prev] == QUOTE) return -1;
      auto last = RfindNonBlank(lines_[n + 1].begin - 1);
      if(code_[last] == QUOTE) return -1;
   }

   return desired;
}

//------------------------------------------------------------------------------

int Lexer::CheckLineMerge(size_t n) const
{
   if(n + 1 >= lines_.size()) return -1;
   const auto& line1 = lines_[n];
   if(!line1.mergeable) return -1;
   const auto& line2 = lines_[n + 1];
   if(!line2.mergeable) return -1;

   auto begin1 = line1.begin;
   auto end1 = line2.begin - 1;
   auto begin2 = line2.begin;
   auto end2 = code_.find(CRLF, begin2);

   //  The first line must not end in a trailing comment, semicolon, colon,
   //  or right brace and must not start with an "if" or "else".  If merging,
   //  a space may also have to be inserted.
   //
   while(WhitespaceChars.find(code_[end2]) != string::npos) --end2;
   if((end2 < begin2) || (end2 == string::npos)) return -1;

   while(WhitespaceChars.find(code_[end1]) != string::npos) --end1;
   if((end1 < begin1) || (end2 == string::npos)) return -1;
   auto c = code_[end1];
   if((c == ';') || (c == ':') || (c == '}')) return -1;

   auto first1 = code_.find_first_not_of(WhitespaceChars, begin1);
   if(code_.find(COMMENT_STR, first1) < end1) return -1;
   if(code_.compare(first1, strlen(IF_STR), IF_STR) == 0) return -1;
   if(code_.compare(first1, strlen(ELSE_STR), ELSE_STR) == 0) return -1;

   auto first2 = code_.find_first_not_of(WhitespaceChars, begin2);
   auto size = (end1 - begin1 + 1) + (end2 - first2 + 1);
   auto space = (!IsWordChar(c) || (code_[first2] != '('));
   if(space) ++size;

   if(size > LineLengthMax()) return -1;
   return (space ? 1 : 0);
}

//------------------------------------------------------------------------------

void Lexer::CheckLines()
{
   Debug::ft("Lexer.CheckLines");

   auto last = lines_.size() - 1;
   auto prev = string::npos;
   string s;
   std::set<Warning> warnings;

   for(size_t n = 0; n <= last; ++n)
   {
      //  Check the code for line N.  Note that CheckLine modifies S.
      //
      GetNthLine(n, s);
      warnings.clear();
      CheckLine(s, warnings);

      if(LineTypeAttr::Attrs[lines_[n].type].isCode)
      {
         //  If this line of code has a trailing comment, see if it
         //  aligns with one immediately above it.
         //
         auto curr = FindComment(lines_[n].begin);
         if(curr != string::npos)
         {
            curr -= lines_[n].begin;

            if((prev != string::npos) && (curr != prev))
            {
               warnings.insert(TrailingCommentAlignment);
            }
         }
         prev = curr;
      }
      else
      {
         //  Don't report a comment for adjacent spaces.
         //
         warnings.erase(AdjacentSpaces);
         prev = string::npos;
      }

      //  Check the line's indentation.
      //
      auto indent = CheckDepth(n);

      if(indent >= 0)
      {
         warnings.insert(Indentation);
      }

      //  Log any warnings that were reported.
      //
      for(auto w = warnings.cbegin(); w != warnings.cend(); ++w)
      {
         file_->LogLine(n, *w);
      }
   }
}

//------------------------------------------------------------------------------

void Lexer::CheckPunctuation() const
{
   Debug::ft("Lexer.CheckPunctuation");

   auto frag = false;

   for(size_t pos = 0; pos < code_.size(); pos = NextPos(pos + 1))
   {
      switch(code_[pos])
      {
      case '{':
         if(!GetLineInfo(pos)->ctorBraceInit)
         {
            if(WhitespaceChars.find(code_[pos - 1]) == string::npos)
               file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "_{");
            if(WhitespaceChars.find(code_[pos + 1]) == string::npos)
               file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "{_");
         }
         break;

      case '}':
         if(!GetLineInfo(pos)->ctorBraceInit)
         {
            if(WhitespaceChars.find(code_[pos - 1]) == string::npos)
               file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "_}");
         }
         if(pos + 1 < code_.size())
         {
            if(WhitespaceChars.find(code_[pos + 1]) == string::npos)
            {
               if(code_[pos + 1] == ';') continue;
               if(code_[pos + 1] == ',') continue;
               file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "}_");
            }
         }
         break;

      case '(':
         if(SpaceOrTab.find(code_[pos + 1]) != string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "(@");
         break;

      case ')':
         if(WhitespaceChars.find(code_[pos - 1]) != string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "@)");
         break;

      case '[':
         if(WhitespaceChars.find(code_[pos + 1]) != string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "[@");
         break;

      case ']':
         if(WhitespaceChars.find(code_[pos - 1]) != string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "@]");
         break;

      case ';':
         if(WhitespaceChars.find(code_[pos - 1]) != string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "@;");
         if(WhitespaceChars.find(code_[pos + 1]) == string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, ";_");
         break;

      case ',':
         if(WhitespaceChars.find(code_[pos - 1]) != string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "@,");
         if(WhitespaceChars.find(code_[pos + 1]) == string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, ",_");
         break;

      case ':':
         if(code_[pos + 1] == ':')
         {
            ++pos;
            continue;
         }

         if(WhitespaceChars.find(code_[pos - 1]) == string::npos)
         {
            if(NoSpaceBeforeColon(pos)) continue;
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, "_:");
         }

         if(WhitespaceChars.find(code_[pos + 1]) == string::npos)
            file_->LogPos(pos, PunctuationSpacing, nullptr, 0, ":_");
         break;

      case APOSTROPHE:
         pos = SkipCharLiteral(pos);
         break;

      case QUOTE:
         pos = SkipStrLiteral(pos, frag);
         break;
      }
   }
}

//------------------------------------------------------------------------------

void Lexer::CheckSwitch(const Switch& code) const
{
   Debug::ft("Lexer.CheckSwitch");

   size_t begin, pos, end;
   if(!code.GetSpan3(begin, pos, end)) return;
   if(pos == string::npos) return;

   //  Starting at the switch statement's left brace, find the identifier
   //  at the beginning of each statement.
   //
   auto info = GetLineInfo(pos);
   auto depth = info->depth;
   auto casePos = string::npos;
   auto codePos = string::npos;
   string id;

   while(FindIdentifier(pos, id, false) && (pos <= end))
   {
      info = GetLineInfo(pos);

      if(info->depth == depth)
      {
         if((id == CASE_STR) || (id == DEFAULT_STR))
         {
            //  If the previous case statement did not end with a jump
            //  statement, log a warning if there isn't a fallthrough
            //  comment between it and the last identifier.
            //
            if((casePos != string::npos) && (codePos != string::npos))
            {
               auto fpos = code_.rfind(FALLTHROUGH_STR, pos);
               if((fpos == string::npos) || (fpos < casePos))
               {
                  file_->LogPos(casePos, NoJumpOrFallthrough);
               }
            }

            casePos = (id == CASE_STR ? pos : string::npos);
            codePos = string::npos;
            pos = FindFirstOf(":", pos);
            continue;
         }
      }
      else if(info->depth == depth + 1)
      {
         //  This isn't nested code, so a jump statement clears any pending
         //  case label.
         //
         if((id == BREAK_STR) || (id == RETURN_STR) ||
            (id == CONTINUE_STR) || (id == THROW_STR) || (id == GOTO_STR))
         {
            casePos = string::npos;
            pos = FindFirstOf(";", pos);
            continue;
         }
         else
         {
            codePos = pos;
         }
      }

      //  Continue with the next statement.
      //
      pos = FindFirstOf(";{", pos);

      if(code_[pos] == '{')
      {
         pos = FindClosing('{', '}', pos + 1);
      }
   }
}

//------------------------------------------------------------------------------

string Lexer::CheckVerticalSpacing() const
{
   Debug::ft("Lexer.CheckVerticalSpacing");

   auto size = lines_.size();
   if(size <= 1) return EMPTY_STR;

   string action(size, LineOK);
   auto prevType = lines_[0].type;

   for(size_t currLine = 1; currLine < size; ++currLine)
   {
      auto prevLine = currLine - 1;
      auto currType = lines_[currLine].type;
      auto nextLine = currLine + 1;
      auto nextType = (nextLine < size ? lines_[nextLine].type : LineType_N);

      switch(currType)
      {
      case CodeLine:
         switch(prevType)
         {
         case FileComment:
         case FunctionName:
         case IncludeDirective:
         case UsingStatement:
            action[currLine] = InsertBlank;
            break;
         }
         break;

      case BlankLine:
      case EmptyComment:
         switch(prevType)
         {
         case BlankLine:
         case EmptyComment:
         case OpenBrace:
            action[currLine] = DeleteLine;
            break;

         case RuleComment:
            switch(nextType)
            {
            case RuleComment:
            case CloseBrace:
            case LineType_N:
               action[prevLine] = DeleteLine;
               action[currLine] = DeleteLine;
               break;

            case TextComment:
               //
               //  Change a blank line between a rule comment and an actual
               //  comment to an empty comment if the rule is not indented.
               //
               if((currType == BlankLine) &&
                  (code_[lines_[prevLine].begin] == '/'))
               {
                  action[currLine] = ChangeToEmptyComment;
               }
               break;
            }
            break;
         }
         break;

      case RuleComment:
         if(action[currLine] != DeleteLine)
         {
            if(!LineTypeAttr::Attrs[prevType].isBlank)
               action[currLine] = InsertBlank;
            if(!LineTypeAttr::Attrs[nextType].isBlank && (nextLine < size))
               action[nextLine] = InsertBlank;
         }
         break;

      case OpenBrace:
         for(auto line = nextLine; line < size; ++line)
         {
            auto type = lines_[line].type;

            if(LineTypeAttr::Attrs[type].isBlank || (type == RuleComment))
               action[line] = DeleteLine;
            else
               break;
         }
         [[fallthrough]];
      case CloseBrace:
      case CloseBraceSemicolon:
         if(LineTypeAttr::Attrs[prevType].isBlank)
            action[prevLine] = DeleteLine;
         break;

      case AccessControl:
         if(LineTypeAttr::Attrs[prevType].isBlank)
            action[prevLine] = DeleteLine;
         if(LineTypeAttr::Attrs[nextType].isBlank)
            action[nextLine] = DeleteLine;

         switch(nextType)
         {
         case CloseBraceSemicolon:
         case AccessControl:
            action[currLine] = DeleteLine;
            break;
         }
         break;

      case FunctionName:
         switch(prevType)
         {
         case BlankLine:
         case EmptyComment:
         case OpenBrace:
         case FunctionName:
            break;
         case TextComment:
            if(file_->IsCpp())
               action[currLine] = InsertBlank;
            break;
         default:
            action[currLine] = InsertBlank;
         }
         break;

      case IncludeDirective:
         if(prevType == HashDirective)
         {
            action[currLine] = InsertBlank;
         }
         break;

      case UsingStatement:
         if(prevType == IncludeDirective)
         {
            action[currLine] = InsertBlank;
         }
         break;

      case FileComment:
      case TextComment:
      case DebugFt:
      case HashDirective:
         break;
      }

      prevType = currType;
   }

   return action;
}

//------------------------------------------------------------------------------

int Lexer::CompareCode(size_t pos, const std::string& str) const
{
   Debug::ft("Lexer.CompareCode");

   if(pos >= code_.size()) return -2;
   return code_.compare(pos, str.size(), str);
}

//------------------------------------------------------------------------------

size_t Lexer::CurrBegin(size_t pos) const
{
   if(pos >= code_.size()) return string::npos;
   if(code_[pos] == CRLF) --pos;
   auto crlf = code_.rfind(CRLF, pos);
   return (crlf == string::npos ? 0 : crlf + 1);
}

//------------------------------------------------------------------------------

size_t Lexer::CurrChar(char& c) const
{
   Debug::ft("Lexer.CurrChar");

   if(curr_ >= code_.size())
   {
      c = NUL;
      return string::npos;
   }

   c = code_[curr_];
   return curr_;
}

//------------------------------------------------------------------------------

size_t Lexer::CurrEnd(size_t pos) const
{
   if(pos >= code_.size()) return string::npos;
   auto crlf = code_.find(CRLF, pos);
   return (crlf == string::npos ? code_.size() - 1 : crlf);
}

//------------------------------------------------------------------------------

void Lexer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Base::Display(stream, prefix, options);

   stream << prefix << "file : " << file_ << CRLF;
   stream << prefix << "curr : " << curr_ << CRLF;
   stream << prefix << "prev : " << prev_ << CRLF;

   if(!options.test(DispVerbose)) return;

   stream << prefix << "code : " << CRLF;

   const auto& info = GetLinesInfo();

   for(auto i = info.cbegin(); i != info.cend(); ++i)
   {
      i->Display(stream);
      stream << SPACE << GetCode(i->begin);
   }
}

//------------------------------------------------------------------------------

void Lexer::DisplayComments(std::ostream& stream) const
{
   const auto& info = GetLinesInfo();

   for(auto i = info.cbegin(); i != info.cend(); ++i)
   {
      if((i->type == TextComment) || LineTypeAttr::Attrs[i->type].isCode)
      {
         auto pos = FindComment(i->begin);

         if(pos != string::npos)
         {
            auto end = CurrEnd(pos);
            auto comment = code_.substr(pos + 3, end - pos - 2);
            stream << comment;
         }
      }
   }
}

//------------------------------------------------------------------------------

size_t Lexer::Find(size_t pos, const string& str) const
{
   Debug::ft("Lexer.Find");

   for(pos = NextPos(pos); pos != string::npos; pos = NextPos(pos + 1))
   {
      if(code_.compare(pos, str.size(), str) == 0)
      {
         return pos;
      }
   }

   return string::npos;
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

   for(auto size = code_.size(); pos < size; pos = NextPos(pos + 1))
   {
      auto c = code_[pos];

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
   }

   return string::npos;
}

//------------------------------------------------------------------------------

void Lexer::FindCode(const OptionalCode* opt, bool compile)
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
      case Cxx::HASH_IF:
      case Cxx::HASH_IFDEF:
      case Cxx::HASH_IFNDEF:
         ++level;
         break;

      case Cxx::HASH_ELIF:
      case Cxx::HASH_ELSE:
         if(level == 0) return opt->SetSkipped(begin, curr_ - 1);
         break;

      case Cxx::HASH_ENDIF:
         if(level == 0) return opt->SetSkipped(begin, curr_ - 1);
         --level;
         break;
      }

      Reposition(FindLineEnd(curr_));
   }
}

//------------------------------------------------------------------------------

size_t Lexer::FindComment(size_t pos) const
{
   Debug::ft("Lexer.FindComment");

   auto end = CurrEnd(pos);
   auto targ = code_.find(COMMENT_STR, pos);
   if(targ < end) return targ;
   targ = code_.find(COMMENT_BEGIN_STR, pos);
   return (targ < end ? targ : string::npos);
}

//------------------------------------------------------------------------------

Cxx::Directive Lexer::FindDirective()
{
   Debug::ft("Lexer.FindDirective");

   string s;

   while(curr_ < code_.size())
   {
      if(code_[curr_] == '#')
         return NextDirective(s);
      else
         Reposition(FindLineEnd(curr_));
   }

   return Cxx::NIL_DIRECTIVE;
}

//------------------------------------------------------------------------------

size_t Lexer::FindFirstOf(const string& targs, size_t pos) const
{
   Debug::ft("Lexer.FindFirstOf");

   //  Return the position of the first occurrence of a character in TARGS.
   //  Start by advancing from POS, in case it's a blank or the start of a
   //  comment.  Jump over any literals or nested expressions.
   //
   if(pos == string::npos) pos = curr_;
   pos = NextPos(pos);
   size_t end;

   for(auto size = code_.size(); pos < size; NO_OP)
   {
      auto f = false;
      auto c = code_[pos];

      if(targs.find(c) != string::npos)
      {
         //  This function can be invoked to look for the colon that delimits
         //  a field width or a label, so don't stop at a colon that is part
         //  of a scope resolution operator.
         //
         if(c != ':') return pos;
         if(code_[pos + 1] != ':') return pos;
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

bool Lexer::FindIdentifier(size_t& pos, string& id, bool tokenize) const
{
   Debug::ft("Lexer.FindIdentifier");

   if(tokenize) id = "$";  // returned if non-identifier found

   for(auto size = code_.size(); pos < size; NO_OP)
   {
      auto f = false;
      auto c = code_[pos];

      switch(c)
      {
      case QUOTE:
         pos = SkipStrLiteral(pos, f);
         pos = NextPos(pos + 1);
         if(tokenize) return true;
         continue;

      case APOSTROPHE:
         pos = SkipCharLiteral(pos);
         pos = NextPos(pos + 1);
         if(tokenize) return true;
         continue;

      default:
         if(CxxChar::Attrs[c].validFirst)
         {
            id = NextIdentifier(pos);
            return true;
         }

         if(CxxChar::Attrs[c].validOp)
         {
            if(tokenize) return true;
            id = NextOperator(pos);
            pos = NextPos(pos + id.size());
            continue;
         }

         if(CxxChar::Attrs[c].validInt)
         {
            if(SkipNum(pos))
            {
               if(tokenize) return true;
               continue;
            }
         }

         pos = NextPos(pos + 1);
      }
   }

   return false;
}

//------------------------------------------------------------------------------

size_t Lexer::FindLineEnd(size_t pos) const
{
   Debug::ft("Lexer.FindLineEnd");

   auto bs = false;

   for(auto size = code_.size(); pos < size; ++pos)
   {
      switch(code_[pos])
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

void Lexer::FindLines()
{
   Debug::ft("Lexer.FindLines");

   lines_.clear();
   if(code_.empty()) return;

   for(size_t pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      lines_.push_back(LineInfo(pos));
   }
}

//------------------------------------------------------------------------------

size_t Lexer::FindNonBlank(size_t pos) const
{
   Debug::ft("Lexer.FindNonBlank");

   for(pos = NextPos(pos); pos != string::npos; pos = NextPos(pos + 1))
   {
      if(!IsBlank(code_[pos]))
      {
         return pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Lexer::FindWord(size_t pos, const string& id) const
{
   Debug::ft("Lexer.FindWord");

   pos = NextPos(pos);
   string name;

   while(FindIdentifier(pos, name, false))
   {
      if(name == id) return pos;
      pos = NextPos(pos + name.size());
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::GetAccess(Cxx::Access& access)
{
   Debug::ft("Lexer.GetAccess");

   //  <Access> = ("public" | "protected" | "private")
   //
   auto str = NextIdentifier(curr_);

   if(str.size() < strlen(PUBLIC_STR)) return false;
   else if(str == PUBLIC_STR) access = Cxx::Public;
   else if(str == PROTECTED_STR) access = Cxx::Protected;
   else if(str == PRIVATE_STR) access = Cxx::Private;
   else return false;

   return Advance(str.size());
}

//------------------------------------------------------------------------------

string Lexer::GetAttribute()
{
   Debug::ft("Lexer.GetAttribute");

   //  An attribute is enclosed in double brackets: [[<attribute>]].
   //
   string attr;

   if(NextStringIs("[["))
   {
      attr = NextIdentifier(curr_);
      Advance(attr.size());
      if(!NextStringIs("]]")) attr.clear();
   }

   return attr;
}

//------------------------------------------------------------------------------

bool Lexer::GetChar(uint32_t& c)
{
   Debug::ft("Lexer.GetChar");

   auto size = code_.size();
   if(curr_ >= size) return false;

   c = code_[curr_];
   ++curr_;

   if(c == BACKSLASH)
   {
      //  This is an escape sequence.  The next character is
      //  taken verbatim unless it has a special meaning.
      //
      int64_t n;

      if(curr_ >= size) return false;
      c = code_[curr_];

      switch(c)
      {
      case '0':
      case '1':  // character's octal value
         GetOct(n);
         c = n;
         break;
      case 'x':  // character's 2-byte hex value
         ++curr_;
         if(curr_ >= size) return false;
         GetHexNum(n, 2);
         c = n;
         break;
      case 'u':  // character's 4-byte hex value
         ++curr_;
         if(curr_ >= size) return false;
         GetHexNum(n, 4);
         c = n;
         break;
      case 'U':  // character's 8-byte hex value
         ++curr_;
         if(curr_ >= size) return false;
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
   auto str = NextIdentifier(curr_);

   if(str.size() < strlen(CLASS_STR)) return false;
   else if(str == CLASS_STR) tag = Cxx::ClassType;
   else if(str == STRUCT_STR) tag = Cxx::StructType;
   else if(str == UNION_STR) tag = Cxx::UnionType;
   else if(type && (str == TYPENAME_STR)) tag = Cxx::Typename;
   else return false;

   return Advance(str.size());
}

//------------------------------------------------------------------------------

string Lexer::GetCode(size_t pos, bool crlf) const
{
   Debug::ft("Lexer.GetCode");

   auto begin = CurrBegin(pos);
   if(begin == string::npos) return EMPTY_STR;
   auto end = CurrEnd(pos);
   auto code = code_.substr(begin, end - begin + 1);
   if(!crlf && !code.empty() && (code.back() == CRLF)) code.pop_back();
   return code;
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
      {
         auto result = tags.insert(kwd);
         if(!result.second && (kwd == Cxx::CONST))
         {
            if(file_ != nullptr)
               file_->LogPos(curr_, RedundantConst);
         }

         Reposition(curr_ + str.size());
         continue;
      }

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
   auto token = NextOperator(curr_);

   while(!token.empty())
   {
      auto match = Cxx::CxxOps->find(token);

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
      case Cxx::CONST:
      case Cxx::CONSTEXPR:
      case Cxx::EXTERN:
      case Cxx::STATIC:
      case Cxx::INLINE:
      case Cxx::MUTABLE:
      case Cxx::THREAD_LOCAL:
      case Cxx::VOLATILE:
         //
         //  "const" and "volatile" go with the type, not the data,
         //  but can still appear before the other keywords.  Barf.
         //
         tags.insert(kwd);
         Reposition(curr_ + str.size());
         continue;

      default:
         return;
      }
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

   for(auto size = code_.size(); (curr_ < size) && (max > 0); ++curr_)
   {
      auto c = code_[curr_];
      auto value = CxxChar::Attrs[c].hexValue;
      if(value < 0) return count;
      ++count;
      num <<= 4;
      num += value;
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
   auto stop = code_.find(CRLF, pos);
   pos = NextPos(pos);
   if(pos >= stop) return false;
   if(code_.find(HASH_INCLUDE_STR, pos) != pos) return false;
   pos = NextPos(pos + strlen(HASH_INCLUDE_STR));
   if(pos >= stop) return false;

   char delimiter;

   switch(code_[pos])
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
   auto end = code_.find(delimiter, pos);
   if(end >= stop) return false;
   file = code_.substr(pos, end - pos);
   return true;
}

//------------------------------------------------------------------------------

TagCount Lexer::GetIndirectionLevel(char c, bool& space)
{
   Debug::ft("Lexer.GetIndirectionLevel");

   space = false;
   if(curr_ >= code_.size()) return 0;
   auto start = curr_;
   TagCount count = 0;
   while(NextCharIs(c)) ++count;
   space = ((count > 0) && (code_[start - 1] == SPACE));
   return count;
}

//------------------------------------------------------------------------------

size_t Lexer::GetInt(int64_t& num)
{
   Debug::ft("Lexer.GetInt");

   size_t count = 0;
   num = 0;

   for(auto size = code_.size(); curr_ < size; ++curr_)
   {
      auto c = code_[curr_];
      auto value = CxxChar::Attrs[c].intValue;
      if(value < 0) return count;
      ++count;
      num *= 10;
      num += value;
   }

   return count;
}

//------------------------------------------------------------------------------

const LineInfo* Lexer::GetLineInfo(size_t pos) const
{
   auto i = GetLineNum(pos);
   if(i == SIZE_MAX) return nullptr;
   return &lines_[i];
}

//------------------------------------------------------------------------------

LineInfo* Lexer::GetLineInfo(size_t pos)
{
   auto i = GetLineNum(pos);
   if(i == SIZE_MAX) return nullptr;
   return &lines_[i];
}

//------------------------------------------------------------------------------

size_t Lexer::GetLineNum(size_t pos) const
{
   if(pos >= code_.size()) return SIZE_MAX;

   //  Do a binary search over the lines' starting positions.
   //
   size_t min = 0;
   size_t max = lines_.size() - 1;

   while(min < max)
   {
      auto mid = (min + max + 1) >> 1;

      if(lines_[mid].begin > pos)
         max = mid - 1;
      else
         min = mid;
   }

   return min;
}

//------------------------------------------------------------------------------

size_t Lexer::GetLineStart(size_t line) const
{
   if(line < lines_.size()) return lines_[line].begin;
   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::GetName(string& name, Constraint constraint)
{
   Debug::ft("Lexer.GetName");

   auto id = NextIdentifier(curr_);
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
      if(Cxx::Types->find(id) != Cxx::Types->cend()) return false;

      if(Cxx::Keywords->find(id) != Cxx::Keywords->cend())
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
         if(Cxx::Keywords->find(id) != Cxx::Keywords->cend())
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
   else if((Cxx::Types->find(name) == Cxx::Types->cend()) &&
         (Cxx::Keywords->find(name) == Cxx::Keywords->cend()))
   {
      return true;
   }

   Reposition(prev_);
   return false;
}

//------------------------------------------------------------------------------

bool Lexer::GetNthLine(size_t n, string& s) const
{
   s.clear();
   auto last = lines_.size() - 1;
   if(n > last) return false;
   auto begin = lines_[n].begin;
   auto end = (n < last ? lines_[n + 1].begin - 1 : code_.size() - 1);
   s = code_.substr(begin, end - begin + 1);
   return true;
}

//------------------------------------------------------------------------------

string Lexer::GetNthLine(size_t n) const
{
   string s;
   GetNthLine(n, s);
   if(!s.empty() && (s.back() == CRLF)) s.pop_back();
   return s;
}

//------------------------------------------------------------------------------

bool Lexer::GetNum(TokenPtr& item)
{
   Debug::ft("Lexer.GetNum");

   //  It is already known that the next character is a digit, so a lot of
   //  nonsense can be avoided by seeing if that digit appears alone.
   //
   if(curr_ > code_.size() - 1) return false;
   auto pos = curr_ + 1;
   auto c = (pos < code_.size() ? code_[pos] : NUL);

   if(!CxxChar::Attrs[c].validInt)
   {
      IntLiteral::Tags tags(IntLiteral::DEC, false, IntLiteral::SIZE_I);
      auto value = CxxChar::Attrs[CurrChar()].intValue;
      if(value < 0) return false;
      item.reset(new IntLiteral(value, tags));
      item->SetContext(curr_);
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
      item->SetContext(start);

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
   item->SetContext(start);
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

   for(auto size = code_.size(); curr_ < size; ++curr_)
   {
      auto c = code_[curr_];
      auto value = CxxChar::Attrs[c].octValue;
      if(value < 0) return count;
      ++count;
      num <<= 3;
      num += value;
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
      auto match = Cxx::CxxOps->find(token);

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
   auto token = NextOperator(curr_);

   while(!token.empty())
   {
      auto match = Cxx::PreOps->find(token);

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

bool Lexer::GetTemplateSpec(string& spec)
{
   Debug::ft("Lexer.GetTemplateSpec");

   spec.clear();
   auto end = SkipTemplateSpec(curr_);
   if(end == string::npos) return false;
   spec = code_.substr(curr_, end - curr_ + 1);
   return Advance(spec.size());
}

//------------------------------------------------------------------------------

void Lexer::Initialize(const string& source, CodeFile* file)
{
   Debug::ft("Lexer.Initialize");

   code_ = source;
   file_ = file;
   curr_ = 0;
   prev_ = 0;
   FindLines();
   Advance();
}

//------------------------------------------------------------------------------

size_t Lexer::IntroStart(size_t pos, bool funcName) const
{
   Debug::ft("Lexer.IntroStart");

   auto start = pos;
   auto found = false;

   for(auto curr = PrevBegin(pos); curr != string::npos; curr = PrevBegin(curr))
   {
      auto type = PosToType(curr);

      switch(type)
      {
      case EmptyComment:
      case TextComment:
         //
         //  These are attached to the code that follows, so include them.
         //
         start = curr;
         break;

      case BlankLine:
         //
         //  This can be included if it precedes or follows an fn_name.
         //
         if(!funcName) return start;
         if(found) start = curr;
         break;

      case FunctionName:
         if(!funcName) return start;
         found = true;
         start = curr;
         break;

      default:
         return start;
      }
   }

   return pos;
}

//------------------------------------------------------------------------------

bool Lexer::IsBlankLine(size_t pos) const
{
   Debug::ft("Lexer.IsBlankLine");

   auto begin = CurrBegin(pos);
   return (code_.find_first_not_of(WhitespaceChars, begin) > CurrEnd(pos));
}

//------------------------------------------------------------------------------

const string BraceInitPrevChars("=,{(");

bool Lexer::IsBraceInit(char prev) const
{
   Debug::ft("Lexer.IsBraceInit");

   //  If this isn't a brace initialization, it's the start of a function.
   //  It's a brace initialization if PREV is one of BraceInitPrevChars.
   //  It's a function if a semicolon occurs before the next right brace.
   //  It's a brace initialization if a right brace occurs before the next
   //  semicolon *unless* it's an empty function.
   //
   if(BraceInitPrevChars.find(prev) != string::npos) return true;

   auto closePos = FindFirstOf(";}", curr_ + 1);
   if(code_[closePos] == ';') return false;

   auto nextPos = FindNonBlank(curr_ + 1);
   return (code_[nextPos] != '}');
}

//------------------------------------------------------------------------------

bool Lexer::IsFirstNonBlank(size_t pos) const
{
   Debug::ft("Lexer.IsFirstNonBlank");

   return (LineFindFirst(CurrBegin(pos)) == pos);
}

//------------------------------------------------------------------------------

bool Lexer::IsInItemGroup(const CxxScoped* item) const
{
   Debug::ft("Lexer.IsInItemGroup");

   if(item == nullptr) return false;

   size_t begin, end;
   item->GetSpan2(begin, end);

   auto pos = NextBegin(end);
   auto after = ((pos != string::npos) && (PosToType(pos) == CodeLine));
   pos = PrevBegin(begin);
   auto before = ((pos != string::npos) && (PosToType(pos) == CodeLine));

   if(!before && !after) return false;

   for(auto n = GetLineNum(pos); n != SIZE_MAX; --n)
   {
      switch(lines_[n].type)
      {
      case CodeLine:
      case EmptyComment:
         break;
      case TextComment:
         return true;
      default:
         return false;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

size_t Lexer::LineFind(size_t pos, const string& str) const
{
   Debug::ft("Lexer.LineFind");

   auto end = CurrEnd(pos);
   if(end == string::npos) return string::npos;

   for(pos = NextPos(pos); pos <= end; pos = NextPos(pos + 1))
   {
      if(code_.compare(pos, str.size(), str) == 0)
      {
         return pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Lexer::LineFindFirst(size_t pos) const
{
   Debug::ft("Lexer.LineFindFirst");

   auto begin = CurrBegin(pos);
   if(begin == string::npos) return string::npos;
   auto loc = code_.find_first_not_of(WhitespaceChars, begin);
   return (loc < CurrEnd(pos) ? loc : string::npos);
}

//------------------------------------------------------------------------------

size_t Lexer::LineFindFirstOf(size_t pos, const std::string& chars) const
{
   Debug::ft("Lexer.LineFindFirstOf");

   auto end = CurrEnd(pos);
   if(end == string::npos) return string::npos;

   for(pos = NextPos(pos); pos <= end; pos = NextPos(pos + 1))
   {
      if(chars.find(code_[pos]) != string::npos)
      {
         return pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Lexer::LineFindNext(size_t pos) const
{
   Debug::ft("Lexer.LineFindNext");

   if(pos >= code_.size()) return string::npos;
   auto loc = code_.find_first_not_of(WhitespaceChars, pos);
   return (loc < CurrEnd(pos) ? loc : string::npos);
}

//------------------------------------------------------------------------------

size_t Lexer::LineFindNonBlank(size_t pos) const
{
   Debug::ft("Lexer.LineFindNonBlank");

   auto end = CurrEnd(pos);
   if(end == string::npos) return string::npos;

   for(pos = NextPos(pos); pos <= end; pos = NextPos(pos + 1))
   {
      if(!IsBlank(code_[pos]))
      {
         return pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::LineHasTrailingCommentAt(size_t n, size_t offset) const
{
   if(n >= lines_.size()) return false;
   const auto& info = lines_[n];
   auto first = LineFindFirst(info.begin);
   auto pos = FindComment(info.begin);
   if(first == pos) return false;
   return ((pos - info.begin) == offset);
}

//------------------------------------------------------------------------------

size_t Lexer::LineRfind(size_t pos, const string& str) const
{
   Debug::ft("Lexer.LineRfind");

   //  The lexer doesn't support reverse scanning, which would involve writing
   //  "reverse" versions of NextPos, FindFirstOf, and various other functions.
   //  So we fake it by scanning forward for the last occurrence.
   //
   auto begin = CurrBegin(pos);
   if(begin == string::npos) return string::npos;

   auto loc = string::npos;

   for(begin = NextPos(begin); begin <= pos; begin = NextPos(begin + 1))
   {
      if(code_.compare(begin, str.size(), str) == 0)
      {
         loc = begin;
      }
   }

   return loc;
}

//------------------------------------------------------------------------------

size_t Lexer::LineRfindFirstOf(size_t pos, const std::string& chars) const
{
   Debug::ft("Lexer.LineRfindFirstOf");

   auto begin = CurrBegin(pos);
   if(begin == string::npos) return string::npos;

   auto loc = string::npos;

   for(begin = NextPos(begin); begin <= pos; begin = NextPos(begin + 1))
   {
      if(chars.find(code_[begin]) != string::npos)
      {
         loc = begin;
      }
   }

   return loc;
}

//------------------------------------------------------------------------------

size_t Lexer::LineRfindNonBlank(size_t pos) const
{
   Debug::ft("Lexer.LineRfindNonBlank");

   auto begin = CurrBegin(pos);
   if(begin == string::npos) return string::npos;

   auto loc = string::npos;

   for(begin = NextPos(begin); begin <= pos; begin = NextPos(begin + 1))
   {
      if(!IsBlank(code_[begin]))
      {
         loc = begin;
      }
   }

   return loc;
}

//------------------------------------------------------------------------------

size_t Lexer::LineSize(size_t pos) const
{
   return CurrEnd(pos) - CurrBegin(pos) + 1;
}

//------------------------------------------------------------------------------

LineType Lexer::LineToType(size_t n) const
{
   Debug::ft("Lexer.LineToType");

   if(n < lines_.size()) return lines_[n].type;
   return LineType_N;
}

//------------------------------------------------------------------------------

string Lexer::MarkPos(size_t pos) const
{
   Debug::ft("Lexer.MarkPos");

   if(pos == string::npos)
   {
      auto first = code_.rfind(CRLF, code_.size() - 2);
      auto text = code_.substr(first + 1);
      if(text.back() == CRLF) text.pop_back();
      text.push_back('$');
      return text;
   }

   auto first = code_.rfind(CRLF, pos);
   if(first == string::npos)
      first = 0;
   else
      ++first;
   auto last = code_.find(CRLF, pos);
   auto text = code_.substr(first, last - first);
   text.insert(pos - first, 1, '$');
   return text;
}

//------------------------------------------------------------------------------

size_t Lexer::NextBegin(size_t pos) const
{
   auto end = CurrEnd(pos);
   return (end >= code_.size() - 1 ? string::npos : end + 1);
}

//------------------------------------------------------------------------------

bool Lexer::NextCharIs(char c)
{
   Debug::ft("Lexer.NextCharIs");

   if((curr_ >= code_.size()) || (code_[curr_] != c)) return false;
   return Advance(1);
}

//------------------------------------------------------------------------------

Cxx::Directive Lexer::NextDirective(string& str) const
{
   Debug::ft("Lexer.NextDirective");

   str = NextIdentifier(curr_);
   if(str.empty()) return Cxx::NIL_DIRECTIVE;

   auto match = Cxx::Directives->find(str);
   return
      (match != Cxx::Directives->cend() ? match->second : Cxx::NIL_DIRECTIVE);
}

//------------------------------------------------------------------------------

string Lexer::NextIdentifier(size_t pos) const
{
   Debug::ft("Lexer.NextIdentifier");

   auto size = code_.size();
   if(pos == string::npos) pos = curr_;
   if(pos >= size) return EMPTY_STR;

   string str;

   //  We assume that the code already compiles.  This means that we
   //  don't have to screen out reserved words that aren't types.
   //
   auto c = code_[pos];
   if(!CxxChar::Attrs[c].validFirst) return str;
   str += c;

   while(++pos < size)
   {
      c = code_[pos];
      if(!CxxChar::Attrs[c].validNext) return str;
      str += c;
   }

   return str;
}

//------------------------------------------------------------------------------

Cxx::Keyword Lexer::NextKeyword(string& str) const
{
   Debug::ft("Lexer.NextKeyword");

   str = NextIdentifier(curr_);
   if(str.empty()) return Cxx::NIL_KEYWORD;

   auto first = str.front();
   if(first == '#') return Cxx::HASH;
   if(first == '~') return Cxx::NVDTOR;

   auto match = Cxx::Keywords->find(str);
   return (match != Cxx::Keywords->cend() ? match->second : Cxx::NIL_KEYWORD);
}

//------------------------------------------------------------------------------

size_t Lexer::NextLineIndentation(size_t pos) const
{
   Debug::ft("Lexer.NextLineIndentation");

   for(pos = NextBegin(pos); pos != string::npos; pos = NextBegin(pos))
   {
      auto indent = LineFindFirst(pos);
      if(indent != string::npos) return ((indent - pos) / IndentSize());
   }

   return 0;
}

//------------------------------------------------------------------------------

string Lexer::NextOperator(size_t pos) const
{
   Debug::ft("Lexer.NextOperator");

   auto size = code_.size();
   if(pos == string::npos) pos = curr_;
   if(pos >= size) return EMPTY_STR;
   string token;
   auto c = code_[pos];

   while(CxxChar::Attrs[c].validOp)
   {
      token += c;
      ++pos;
      c = code_[pos];
   }

   return token;
}

//------------------------------------------------------------------------------

size_t Lexer::NextPos(size_t pos) const
{
   //  Find the next character to be parsed.
   //
   for(auto size = code_.size(); pos < size; NO_OP)
   {
      auto c = code_[pos];

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
         if(++pos >= size) return string::npos;

         switch(code_[pos])
         {
         case '/':
            //
            //  This is a // comment.  Continue on the next line.
            //
            pos = code_.find(CRLF, pos);
            if(pos == string::npos) return pos;
            ++pos;
            break;

         case '*':
            //
            //  This is a /* comment.  Continue where it ends.
            //
            if(++pos >= size) return string::npos;
            pos = code_.find(COMMENT_END_STR, pos);
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
         if(++pos >= size) return string::npos;
         if(code_[pos] != CRLF) return pos - 1;
         ++pos;
         break;

      default:
         return pos;
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::NextStringIs(c_string str, bool check)
{
   Debug::ft("Lexer.NextStringIs");

   auto size = code_.size();
   if(curr_ >= size) return false;

   auto len = strlen(str);
   if(code_.compare(curr_, len, str) != 0) return false;

   auto pos = curr_ + len;
   if(!check || (pos >= size)) return Reposition(pos);

   auto next = code_[pos];

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
      if(CxxChar::Attrs[str[len - 1]].validNext)
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

   auto token = NextIdentifier(curr_);
   if(!token.empty()) return token;
   return NextOperator(curr_);
}

//------------------------------------------------------------------------------

Cxx::Type Lexer::NextType()
{
   Debug::ft("Lexer.NextType");

   auto token = NextIdentifier(curr_);
   if(token.empty()) return Cxx::NIL_TYPE;
   auto type = Cxx::GetType(token);
   if(type != Cxx::NIL_TYPE) Advance(token.size());
   return type;
}

//------------------------------------------------------------------------------

bool Lexer::NoCodeFollows(size_t pos) const
{
   Debug::ft("Lexer.NoCodeFollows");

   auto crlf = code_.find_first_of(CRLF, pos);
   pos = code_.find_first_not_of(WhitespaceChars, pos);
   if(pos >= crlf) return true;
   if(CompareCode(pos, COMMENT_STR) == 0) return true;

   if(CompareCode(pos, COMMENT_BEGIN_STR) == 0)
   {
      pos = code_.find(COMMENT_END_STR, pos + 2);
      if(pos >= crlf) return true;
      return NoCodeFollows(pos + 2);
   }

   return false;
}

//------------------------------------------------------------------------------

bool Lexer::NoSpaceBeforeColon(size_t pos) const
{
   Debug::ft("Lexer.NoSpaceBeforeColon");

   auto size = strlen(PUBLIC_STR);
   auto begin = pos - size;
   if(code_.compare(begin, size, PUBLIC_STR) == 0) return true;

   size = strlen(PROTECTED_STR);
   begin = pos - size;
   if(code_.compare(begin, size, PROTECTED_STR) == 0) return true;

   size = strlen(PRIVATE_STR);
   begin = pos - size;
   if(code_.compare(begin, size, PRIVATE_STR) == 0) return true;

   if(code_.rfind(CASE_STR, pos) >= CurrBegin(pos)) return true;
   if(code_.rfind(DEFAULT_STR, pos) >= CurrBegin(pos)) return true;

   return false;
}

//------------------------------------------------------------------------------

bool Lexer::OnSameLine(size_t pos1, size_t pos2) const
{
   Debug::ft("Lexer.OnSameLine");

   return (CurrEnd(pos1) == CurrEnd(pos2));
}

//------------------------------------------------------------------------------

LineType Lexer::PosToType(size_t pos) const
{
   Debug::ft("Lexer.PosToType");

   auto line = GetLineNum(pos);
   if(line == SIZE_MAX) return LineType_N;
   return LineToType(line);
}

//------------------------------------------------------------------------------

void Lexer::Preprocess()
{
   Debug::ft("Lexer.Preprocess");

   //  Keep fetching identifiers, erasing any that are #defined symbols that
   //  map to empty strings.  Skip preprocessor directives.
   //
   auto syms = Singleton<CxxSymbols>::Instance();
   auto file = Context::File();
   auto scope = Singleton<CxxRoot>::Instance()->GlobalNamespace();
   auto pos = NextPos(0);
   string id;

   while(FindIdentifier(pos, id, false))
   {
      if(id.front() == '#')
      {
         pos = NextPos(FindLineEnd(pos));
         continue;
      }

      SymbolView view;
      auto item = syms->FindSymbol(file, scope, id, MACRO_MASK, view);

      if(item != nullptr)
      {
         auto def = static_cast<Define*>(item);

         if(def->Empty())
         {
            for(size_t i = 0; i < id.size(); ++i) code_[pos + i] = SPACE;
            def->WasRead();
         }
      }

      pos = NextPos(pos + id.size());
   }
}

//------------------------------------------------------------------------------

size_t Lexer::PrevBegin(size_t pos) const
{
   if(pos >= code_.size()) pos = code_.size() - 1;
   auto begin = CurrBegin(pos);
   return (begin == 0 ? string::npos : CurrBegin(begin - 1));
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

size_t Lexer::Rfind(size_t pos, const std::string& str) const
{
   Debug::ft("Lexer.Rfind");

   for(NO_OP; pos != string::npos; pos = CurrBegin(pos) - 1)
   {
      auto loc = LineRfind(pos, str);
      if(loc != string::npos) return loc;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Lexer::RfindFirstOf(size_t pos, const std::string& chars) const
{
   Debug::ft("Lexer.RfindFirstOf");

   for(NO_OP; pos != string::npos; pos = CurrBegin(pos) - 1)
   {
      auto loc = LineRfindFirstOf(pos, chars);
      if(loc != string::npos) return loc;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Lexer::RfindNonBlank(size_t pos) const
{
   Debug::ft("Lexer.RfindNonBlank");

   for(NO_OP; pos != string::npos; pos = CurrBegin(pos) - 1)
   {
      auto loc = LineRfindNonBlank(pos);
      if(loc != string::npos) return loc;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

void Lexer::SetDepth(int8_t depth1, int8_t depth2, bool merge)
{
   //  BEGIN is the last position where a line of code whose depth has not
   //  been determined started, and curr_ has finalized the depth of that
   //  code.  Each line from BEGIN to the one above the next parse position
   //  is therefore at DEPTH unless its depth has already been determined.
   //  If there is more than one line in this range, the subsequent ones
   //  are continuations of the first.
   //
   auto begin = nextLine_;
   auto mid = GetLineNum(curr_);
   auto pos = NextPos(curr_ + 1);
   auto end = GetLineNum(pos);
   if(end == SIZE_MAX) end = lines_.size();
   nextLine_ = end;
   auto first = begin;
   if(lines_[begin].depth != DEPTH_NOT_SET) ++first;

   for(auto i = begin; i <= mid; ++i)
   {
      auto& info = lines_[i];
      if(info.depth == DEPTH_NOT_SET)
      {
         info.depth = depth1;
         info.continuation = (i != first);
         if(!merge) info.mergeable = false;
      }
   }

   for(auto i = mid + 1; i < end; ++i)
   {
      auto& info = lines_[i];
      if(info.depth == DEPTH_NOT_SET)
      {
         info.depth = depth2;
         info.continuation = (i != mid + 1);
         if(!merge) info.mergeable = false;
      }
   }
}

//------------------------------------------------------------------------------

bool Lexer::Skip()
{
   Debug::ft("Lexer.Skip");

   //  Advance to whatever follows the current line.
   //
   if(curr_ >= code_.size()) return true;
   curr_ = code_.find(CRLF, curr_);
   return Advance(1);
}

//------------------------------------------------------------------------------

size_t Lexer::SkipCharLiteral(size_t pos) const
{
   Debug::ft("Lexer.SkipCharLiteral");

   //  The literal ends at the next non-escaped occurrence of an apostrophe.
   //
   for(auto size = code_.size(); ++pos < size; NO_OP)
   {
      auto c = code_[pos];
      if(c == APOSTROPHE) return pos;
      if(c == BACKSLASH) ++pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

bool Lexer::SkipNum(size_t& pos) const
{
   Debug::ft("Lexer.SkipNum");

   if(pos >= code_.size()) return false;
   auto c = code_[pos];
   if(CxxChar::Attrs[c].intValue < 0) return false;

   while(++pos < code_.size())
   {
      c = code_[pos];
      auto& attrs = CxxChar::Attrs[c];

      if(!attrs.validInt && (attrs.hexValue < 0))
      {
         pos = NextPos(pos);
         return true;
      }
   }

   return true;
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

   for(auto size = code_.size(); ++pos < size; NO_OP)
   {
      auto c = code_[pos];

      switch(c)
      {
      case QUOTE:
         next = NextPos(pos + 1);
         if(next == string::npos) return pos;
         if(code_[next] != QUOTE) return pos;
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

   auto size = code_.size();
   if(pos >= size) return string::npos;

   //  Extract the template specification, which must begin with a '<', end
   //  with a balanced '>', and contain identifiers or template punctuation.
   //
   auto c = code_[pos];
   if(c != '<') return string::npos;
   ++pos;

   size_t depth;

   for(depth = 1; ((pos < size) && (depth > 0)); ++pos)
   {
      c = code_[pos];
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

string Lexer::Substr(size_t pos, size_t count) const
{
   Debug::ft("Lexer.Substr");

   string s = code_.substr(pos, count);
   return Compress(s);
}

//------------------------------------------------------------------------------

bool Lexer::ThisCharIs(char c)
{
   Debug::ft("Lexer.ThisCharIs");

   //  If the next character is C, advance to the character that follows it.
   //
   if((curr_ >= code_.size()) || (code_[curr_] != c)) return false;
   ++curr_;
   return true;
}

//------------------------------------------------------------------------------

void Lexer::Update()
{
   Debug::ft("Lexer.Update");

   //  The code has been modified, so regenerate our LineInfo records.
   //
   FindLines();
   CalcLineTypes();
   CalcDepths();
}
}
