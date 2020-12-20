//==============================================================================
//
//  SourceCode.h
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
#ifndef SOURCECODE_H_INCLUDED
#define SOURCECODE_H_INCLUDED

#include "Base.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <list>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Information about a line of code.  This duplicates data found in CodeFile
//  (.code and .type) and Lexer (.depth and .cont, with .line being derivable).
//
struct SourceLine
{
   SourceLine(const std::string& source, size_t seqno);

   void Display(std::ostream& stream) const;

   //  The code.
   //
   std::string code;

   //  The code's line number (the first line is 0, the same as elsewhere).
   //  A line added during editing has a line number of SIZE_MAX.
   //
   const size_t line;

   //  The line's lexical level for indentation.
   //
   int8_t depth;

   //  Set if the code continued from the previous line.
   //
   bool cont;

   //  The line's type.
   //
   LineType type;
};

//------------------------------------------------------------------------------
//
//  Source code is kept in a list.
//
typedef std::list< SourceLine > SourceList;

//  For iterating over lines of code.
//
typedef std::list< SourceLine >::iterator SourceIter;

//------------------------------------------------------------------------------
//
//  For iterating over code: a line of code and a position within that line.
//
struct SourceLoc
{
   SourceIter iter;  // location in SourceList
   size_t pos;       // position in iter->code

   explicit SourceLoc(const SourceIter& i) : iter(i), pos(0) { }

   SourceLoc(const SourceIter& i, size_t p) : iter(i), pos(p) { }

   SourceLoc(const SourceLoc& that) = default;

   SourceLoc& operator=(const SourceLoc& that) = default;

   bool operator==(const SourceLoc& that) const
   {
      return ((this->iter == that.iter) && (this->pos == that.pos));
   }

   bool operator!=(const SourceLoc& that) const
   {
      return !(*this == that);
   }

   SourceLoc& NextChar()  // equivalent to operator++
   {
      if(++pos < iter->code.size()) return *this;
      ++iter;
      pos = 0;
      return *this;
   }

   SourceLoc& NextLine()
   {
      ++iter;
      pos = 0;
      return *this;
   }

   SourceLoc& PrevChar()  // equivalent to operator--
   {
      if(pos > 0)
      {
         --pos;
         return *this;
      }

      --iter;
      pos = iter->code.size() - 1;
      return *this;
   }

   //  Returns the last non-whitespace character in iter->code.  Returns
   //  NUL if there is no non-whitespace character.
   //
   char LastChar() const;
};

//------------------------------------------------------------------------------
//
//  Source code.  Unlike CodeFile and Lexer, which use a single string with
//  embedded blanks to store a file's code, this breaks the code into lines for
//  ease of editing.  The original idea was replace Lexer with this class, but
//  it quickly became clear that this would significantly degrade performance
//  by replacing the single string's size_t iterator with SourceLoc.
//
//  To support Editor, this class provides all the functions of Lexer except
//  Lexer.FindCode.  It also provides CodeFile's functions for determining the
//  LineType for a line of code.  The goal is to keep it synched with changes
//  to Lexer and CodeFile's LineType code.
//
//  Only the following functions in this class have been tested:
//  o Initialize
//  o CalcDepths
//  o ClassifyLines
//  o functions invoked by Editor
//  o functions invoked transitively by any of the above
//
//  The code was imported from Lexer and then modified, primarily to replace
//  string's size_t iterator with SourceLoc.  Editor was also modified to use
//  this class to store and edit its code: this made various Editor functions
//  static, and so they were moved to this class, where their declarations
//  follow those that correspond to Lexer functions.
//
class SourceCode : public NodeBase::Base
{
public:
   //  Initializes all fields to default values.
   //
   SourceCode();

   //  Not subclassed.
   //
   ~SourceCode() = default;

   //  Classifies all lines of code.
   //
   void ClassifyLines();

   //  Classifies the Nth line of code and looks for some warnings.
   //  Sets CONT if a line of code continues on the next line.
   //
   LineType ClassifyLine(size_t n, bool& cont);

   //  Classifies a line of code (S) and updates WARNINGS with any warnings
   //  that were found.  Sets CONT for a line of code that does not end in
   //  a semicolon.
   //
   static LineType ClassifyLine
      (std::string s, bool& cont, std::set< Warning >& warnings);

   //  Returns a reference to the start of the first line of code.
   //
   SourceLoc FirstLine() { return SourceLoc(source_.begin()); }

   //  Reads FILE's code and invokes Advance() to position curr_ at the first
   //  valid parse position.  Returns false if the code could not be read.
   //
   bool Initialize(const CodeFile& file);

   //  Returns the character at POS.
   //
   char At(const SourceLoc& loc) const { return loc.iter->code[loc.pos]; }

   //  Returns the number of lines in source_.
   //
   size_t LineCount() const { return source_.size(); }

   //  Returns the line number on which LOC occurs.  Returns string::npos
   //  if LOC is out of range.
   //
   //  NOTE: Internally, line numbers start at 0.  When a line number is to
   //  ====  be displayed, it must be incremented.  Similarly, a line number
   //        obtained externally (e.g. via the CLI) must be decremented.
   //
   size_t GetLineNum(const SourceLoc& loc);

   //  Returns the position of the first character in LINE.
   //
   SourceLoc GetLineStart(size_t line);

   //  Returns the line that includes LOC, with a '$' inserted after LOC.POS.
   //
   std::string GetLine(const SourceLoc& loc);

   //  Sets S to the string for the Nth line of code, excluding the endline,
   //  or EMPTY_STR if N was out of range.  Returns true if N was valid.
   //
   bool GetNthLine(size_t n, std::string& s);

   //  Returns the string for the Nth line of code.  Returns EMPTY_STR if N
   //  is out of range.
   //
   std::string GetNthLine(size_t n);

   //  Returns the LineType for line N.  Returns LineType_N if N is out
   //  of range.
   //
   LineType GetLineType(size_t n) const;

   //  Returns a string containing the next COUNT characters, starting at LOC.
   //  Converts endlines to blanks and compresses adjacent blanks.
   //
   std::string Extract(const SourceLoc& loc, size_t count);

   //  Returns a string containing the characters from BEGIN to END.
   //
   std::string Extract(const SourceLoc& begin, const SourceLoc& end);

   //  Returns the current parse position.
   //
   //  NOTE: Unless stated otherwise, a non-const Lexer function advances
   //  ====  curr_ to the first parse position after what was extracted.
   //
   SourceLoc Curr() const { return curr_; }

   //  Returns the previous parse position.  Typically used to obtain the
   //  location where the last successful parse began.
   //
   SourceLoc Prev() const { return prev_; }

   //  Returns true if curr_ has reached the end of source_.
   //
   bool Eof() const { return curr_.iter == source_.end(); }

   //  Sets prev_ to curr_, finds the first parse position starting there,
   //  and returns true.
   //
   bool Advance();

   //  Sets prev_ to curr_ + INCR, finds the first parse position starting
   //  there, and returns true.
   //
   bool Advance(size_t incr);

   //  Sets prev_ to LOC, finds the first parse position starting there,
   //  and returns true.
   //
   bool Reposition(const SourceLoc& loc);

   //  Sets prev_ to LOC + INCR, finds the first parse position starting
   //  there, and returns true.
   //
   bool Reposition(const SourceLoc& loc, size_t incr);

   //  Repositions curr_ to the beginning of the source code.
   //
   void Reset();

   //  Sets prev_ and curr_ to LOC and returns false.  Used for backing up
   //  to a position that is known to be valid and trying another parse.
   //
   bool Retreat(const SourceLoc& loc);

   //  Skips the current line and moves to the first parse position starting
   //  with the next line.  Returns true.
   //
   bool Skip();

   //  Returns the character at curr_.
   //
   char CurrChar() const { return (*curr_.iter).code[curr_.pos]; }

   //  Returns curr_ and sets C to the character at that position.  Returns
   //  End() if curr_ is out of range.
   //
   SourceLoc CurrChar(char& c);

   //  Returns true and advances curr_ if the character at curr_ matches C.
   //
   bool NextCharIs(char c);

   //  The same as NextCharIs, but only advances curr_ to the character that
   //  immediately follows C.  Used to parse literals, as it does not skip
   //  over blanks and comments.
   //
   bool ThisCharIs(char c);

   //  Returns true if STR starts at curr_, advancing curr_ beyond STR.  Used
   //  when looking for a specific keyword or operator.  If CHECK isn't forced
   //  to false, then either
   //  o a space or endline must follow STR, or
   //  o if STR ends in a punctuation character, the next character must not
   //    be punctuation (and vice versa).
   //
   bool NextStringIs(NodeBase::fixed_string str, bool check = true);

   //  Returns the location where the line containing LOC ends.  This is the
   //  location of the next CRLF that is not preceded by a '\'.
   //
   SourceLoc FindLineEnd(SourceLoc loc);

   //  Until the next #define is reached, look for #defined symbols that map to
   //  empty strings and erase them so that they will not cause parsing errors.
   //
   void PreprocessSource();

   //  Returns the next identifier (which could be a keyword).  The first
   //  character not allowed in an identifier finalizes the string.
   //
   std::string NextIdentifier();

   //  Sets STR to the next preprocessor directive and returns its enum
   //  constant.  Returns NIL_DIRECTIVE if a directive wasn't found, but
   //  nonetheless sets STR to any identifier that was found.
   //
   Cxx::Directive NextDirective(std::string& str);

   //  Sets STR to the next keyword and returns its enum constant.  Returns
   //  NIL_KEYWORD if a keyword wasn't next, but nonetheless sets STR to any
   //  identifier that was found.
   //
   Cxx::Keyword NextKeyword(std::string& str);

   //  Updates TAGS with the next series of keywords that appear in a data
   //  declaration.  Stops when a non-keyword is reached.
   //
   void GetDataTags(KeywordSet& tags);

   //  Updates TAGS if "const" and/or "volatile" appear next.
   //
   void GetCVTags(KeywordSet& tags);

   //  Updates TAGS with the next series of keywords that appear before the
   //  name in a function declaration.  Stops when a non-keyword is reached.
   //
   void GetFuncFrontTags(KeywordSet& tags);

   //  Updates TAGS with the next series of keywords that appear at the end
   //  of a function declaration.  Stops when a non-keyword is reached.
   //
   void GetFuncBackTags(KeywordSet& tags);

   //  Returns the next operator (punctuation only).
   //
   std::string NextOperator();

   //  Starts at LOC and searches for a right-hand character (RHC) that matches
   //  a left-hand one (LHC) that was just found (e.g. (), [], {}, <>).  If LOC
   //  is not provided, LOC is set to curr_.  For each LHC that is found, an
   //  additional RHC must be found.  Returns the position where the matching
   //  RHC was found, else End().
   //
   SourceLoc FindClosing(char lhc, char rhc, SourceLoc loc);
   SourceLoc FindClosing(char lhc, char rhc);

   //  Searches for the next occurrence of a character in TARGS.  Returns
   //  the position of the first character that was found, else End().  The
   //  character must be at the same lexical level, meaning that when any of
   //  { ( [ < " ' are encountered, the parse skips to the closing } ) ] > " '
   //  (if, in the case of '<', a <...> pair enclose a template specification).
   //
   SourceLoc FindFirstOf(const std::string& targs);

   //  Returns true and sets SPEC to the template specification that follows
   //  the name of a template instance.
   //
   bool GetTemplateSpec(std::string& spec);

   //  Returns true and creates or updates NAME on finding an identifier that
   //  advances curr_ beyond NAME.
   //
   bool GetName(std::string& name, Constraint constraint = NonKeyword);

   //  Returns true and creates or updates NAME on finding an identifier (or
   //  keyword).  If NAME is "operator", updates OPER to the operator that
   //  follows it.  Advances curr_ beyond NAME and OPER.
   //
   bool GetName(std::string& name, Cxx::Operator& oper);

   //  Returns true and updates OPER if the next token is an operator.  Used
   //  for parsing an operator that follows the "operator" keyword.
   //
   bool GetOpOverride(Cxx::Operator& oper);

   //  Returns the next token if it is an operator.  Returns NIL_OPERATOR if
   //  an operator is not found.  Used for parsing non-alphabetic operators
   //  that appear in expressions.  GetCxxOp is for C++ code, and GetPreOp
   //  is for preprocessor directives.
   //
   Cxx::Operator GetCxxOp();
   Cxx::Operator GetPreOp();

   //  If the next token is a built-in type that can appear in a compound
   //  type, returns its Cxx::Type  and advances curr_ beyond it.  Returns
   //  Cxx::NIL_TYPE and leaves curr_ unchanged in other cases.
   //
   Cxx::Type NextType();

   //  If the next token is a numeric literal, creates a FloatLiteral or
   //  IntLiteral and returns it in ITEM.  Does *not* parse a unary + or -.
   //
   bool GetNum(TokenPtr& item);

   //  Sets C to the value of the next character within a character or string
   //  literal and returns true.  Handles escape sequences.  Returns false if
   //  the end of the source code is reached before a character is found.
   //
   bool GetChar(uint32_t& c);

   //  Returns true and updates TAG on finding a class keyword.  TYPE is
   //  set if "typename" is acceptable as a keyword.
   //
   bool GetClassTag(Cxx::ClassTag& tag, bool type = false);

   //  Returns true and updates ACCESS on finding an access control keyword.
   //
   bool GetAccess(Cxx::Access& access);

   //  Returns the number of C's that occur in a row.  Sets SPACE if at least
   //  one C was found but was preceded by a space.
   //
   TagCount GetIndirectionLevel(char c, bool& space);

   //  Looks for the next #include directive starting at LOC.  If one is found,
   //  sets FILE to the included filename, sets ANGLE if FILE appeared within
   //  angle brackets, advances LOC to the following line, and returns true.
   //  Returns false if no #include directive was found, in which LOC will be
   //  End().
   //
   bool GetIncludeFile(SourceLoc loc, std::string& file, bool& angle);

   //  Advances curr_ to the start of the next identifier, which is supplied
   //  in ID.  The identifier could be a keyword or preprocessor directive.
   //  Returns true if an identifier was found, else false.  If TOKENIZE is
   //  set, true is returned if a numeric or punctuation is found, in which
   //  case ID is set to "$" and curr_ advances to the first position after
   //  a numeric, whereas it remains at punctuation.
   //
   bool FindIdentifier(std::string& id, bool tokenize);

   //  Calculates the lexical level for each line of code.
   //
   void CalcDepths();

   //  Returns the DEPTH of indentation for a line.  Sets CONT if the line
   //  continues a previous line.
   //
   void GetDepth(size_t line, int8_t& depth, bool& cont);

   /////////////////////////////////////////////////////////////////////////////
   //
   //  End of Lexer and CodeFile functions.  Start of functions used by Editor.
   //
   /////////////////////////////////////////////////////////////////////////////

   //  Provides access to the code.
   //
   SourceList& GetSource() { return source_; }

   //  Returns the end of source_.
   //
   SourceLoc End();

   //  Sets LOC to the end of source and returns it.
   //
   SourceLoc& End(SourceLoc& loc);

   //  Returns LOC after updating it to the next character.
   //
   SourceLoc& Next(SourceLoc& loc);

   //  Returns LOC after updating it to the previous character.
   //
   SourceLoc& Prev(SourceLoc& loc);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Used by PreprocessSource, which creates a clone of "this" lexer to
   //  do the work.
   //
   void Preprocess();

   //  Returns the position of the next character to parse, starting at START.
   //  The result is START unless characters are skipped (namely whitespace,
   //  comments, and character and string literals).  If SKIP is provided, the
   //  search begins at START plus SKIP characters.
   //
   SourceLoc NextPos(const SourceLoc& start);
   SourceLoc NextPos(const SourceLoc& start, size_t skip);

   //  Returns the next token.  This is a non-empty string from NextIdentifier
   //  or, failing that, a sequence of valid operator characters.  The first
   //  character not allowed in an identifier (or an operator) finalizes the
   //  string.
   //
   std::string NextToken();

   //  Advances over the literal that begins at LOC.  Updates LOC to the
   //  position of the character that closes the literal.
   //
   void SkipCharLiteral(SourceLoc& loc);

   //  Advances over the literal that begins at LOC.  Updates LOC to the
   //  position of the character that closes the literal.  Sets FRAGMENTED
   //  if the string is of the form ("<string>"<whitespace>)*"<string>".
   //
   void SkipStrLiteral(SourceLoc& loc, bool& fragmented);

   //  LOC is the location of a '<'.  If a template specification follows,
   //  returns the location of the '>' at the end of the specification, else
   //  returns the end of the source.
   //
   SourceLoc SkipTemplateSpec(SourceLoc loc);

   //  Parses an integer literal and returns it in NUM.  Returns the number
   //  of digits in NUM (zero if no literal was found).
   //
   size_t GetInt(int64_t& num);

   //  Parses a hex literal and returns it in NUM.  Returns the number of
   //  digits in NUM (zero if no literal was found).
   //
   size_t GetHex(int64_t& num);

   //  Parses a hex string (digits only) and returns it in NUM.  Returns the
   //  number of digits in NUM (zero if no hex digits were found).  MAX is
   //  the maximum number of digits allowed in the string.
   //
   size_t GetHexNum(int64_t& num, size_t max = 16);

   //  Parses an octal literal and returns it in NUM.  Returns the number of
   //  digits in NUM (zero if no literal was found).
   //
   size_t GetOct(int64_t& num);

   //  Parses the numbers after the decimal point in a floating point literal
   //  and adds them to NUM.
   //
   void GetFloat(long double& num);

   //  Advances curr_ to the start of the next preprocessor directive, which
   //  is returned.
   //
   Cxx::Directive FindDirective();

   //  Sets all lines from positions START to curr_ to DEPTH1, and all lines
   //  after curr_ to the next parse position to DEPTH2.  If either range spans
   //  multiple lines, subsequent lines are marked as continuations of the first
   //  line in the range.  Updates START to the next parse position after curr_
   //  before returning.
   //
   void SetDepth(SourceLoc& start, int8_t depth1, int8_t depth2);

   //  Returns the position of the last character in source_.
   //
   SourceLoc LastLoc();

   //  Searches for STR starting at LOC. Updates LOC to the first location
   //  after STR if found, else sets it to End().
   //
   void NextAfter(SourceLoc loc, const std::string& str);

   //  The source code.
   //
   SourceList source_;

   //  The file, if any, from which the code was taken.
   //
   const CodeFile* file_;

   //  Set if the code has been scanned.
   //
   bool scanned_;

   //  Set if a /* comment is open during a lexical scan.
   //
   bool slashAsterisk_;

   //  The current position within source_.
   //
   SourceLoc curr_;

   //  The location when Advance was invoked (that is, the character
   //  immediately after the last one that was parsed).
   //
   SourceLoc prev_;
};
}
#endif
