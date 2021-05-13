//==============================================================================
//
//  Lexer.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "Base.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

namespace CodeTools
{
   class Switch;
}

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Information about a line of source code.
//
struct LineInfo
{
   const size_t begin;  // offset where line starts; it ends at a CRLF
   int depth;           // lexical level for indentation
   bool continuation;   // set if code continues from the previous line
   bool mergeable;      // set if code can merge with another line
   LineType type;       // line's type

   //  Constructs a line that begins at START.
   //
   explicit LineInfo(size_t start);

   //  Displays the line in STREAM, prefixed with its depth and a '+' if it
   //  is a continuation.
   //
   void Display(std::ostream& stream) const;
};

//  Indicates that the depth of a line of code has not yet been determined.
//
constexpr int DEPTH_NOT_SET = -1;

//------------------------------------------------------------------------------

class Lexer : public NodeBase::Base
{
public:
   //  Initializes all fields to default values.
   //
   Lexer();

   //  Not subclassed.
   //
   virtual ~Lexer() = default;

   //  Initializes the lexer to assist with parsing SOURCE and invokes
   //  Advance() to position curr_ at the first valid parse position.
   //  FILE is the file, if any, from which the code was taken.
   //
   void Initialize(const std::string& source, CodeFile* file = nullptr);

   //  Returns the code on POS's line, removing its endline if CRLF if FALSE.
   //  Returns EMPTY_STR if POS is out of range.
   //
   std::string GetCode(size_t pos, bool crlf = true) const;

   //  Returns the character at POS.
   //
   char At(size_t pos) const { return (*source_)[pos]; }

   //  Returns the number of lines in source_.
   //
   size_t LineCount() const { return lines_.size(); }

   //  Returns the line number on which POS occurs.  If the code has been
   //  edited, this may not match the line number in the original code.
   //
   //  NOTE: Internally, line numbers start at 0.  When a line number is to
   //  ====  be displayed, it must be incremented.  Similarly, a line number
   //        obtained externally (e.g. via the CLI) must be decremented.
   //
   size_t GetLineNum(size_t pos) const;

   //  Returns the position of the first character in LINE.  Returns
   //  string::npos if LINE is out of range.  Cannot be used on code once
   //  it has been edited.
   //
   size_t GetLineStart(size_t line) const;

   //  Sets S to the string for the Nth line of code, removing its endline
   //  if CRLF is FALSE.  Clears S and returns false if N is out range.
   //  Cannot be used on code once it has been edited.
   //
   bool GetNthLine(size_t n, std::string& s, bool crlf) const;

   //  Returns the string for the Nth line of code after removing its endline.
   //  Returns EMPTY_STR if N is out of range.  Cannot be used on code once it
   //  has been edited.
   //
   std::string GetNthLine(size_t n) const;

   //  Returns the line that includes POS, with a '$' inserted after POS.
   //
   std::string MarkPos(size_t pos) const;

   //  Returns a string containing the next COUNT characters, starting at POS.
   //  Converts endlines to blanks and compresses adjacent blanks.
   //
   std::string Substr(size_t pos, size_t count) const;

   //  Returns the current parse position.
   //
   //  NOTE: Unless stated otherwise, a non-const Lexer function advances
   //  ====  curr_ to the first parse position after what was extracted.
   //
   size_t Curr() const { return curr_; }

   //  Returns the previous parse position.  Typically used to obtain the
   //  location where the last successful parse began.
   //
   size_t Prev() const { return prev_; }

   //  Returns true if curr_ has reached the end of source_.
   //
   bool Eof() const { return curr_ >= source_->size(); }

   //  Sets prev_ to curr_, finds the first parse position starting there,
   //  and returns true.
   //
   bool Advance();

   //  Sets prev_ to curr_ + incr, finds the first parse position starting
   //  there, and returns true.
   //
   bool Advance(size_t incr);

   //  Sets prev_ to POS, finds the first parse position starting there, and
   //  returns true.
   //
   bool Reposition(size_t pos);

   //  Sets prev_ and curr_ to POS and returns false.  Used for backing up
   //  to a position that is known to be valid and trying another parse.
   //
   bool Retreat(size_t pos);

   //  Skips the current line and moves to the first parse position starting
   //  with the next line.  Returns true.
   //
   bool Skip();

   //  Returns the character at curr_.
   //
   char CurrChar() const { return (*source_)[curr_]; }

   //  Returns curr_ and sets C to the character at that position.  Returns
   //  string::npos if curr_ is out of range.
   //
   size_t CurrChar(char& c) const;

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

   //  Returns the location where the line containing POS ends.  This is the
   //  location of the next CRLF that is not preceded by a '\'.
   //
   size_t FindLineEnd(size_t pos) const;

   //  Until the next #define is reached, look for #defined symbols that map to
   //  empty strings and erase them so that they will not cause parsing errors.
   //
   void Preprocess() const;

   //  Sets STR to the next preprocessor directive and returns its enum
   //  constant.  Returns NIL_DIRECTIVE if a directive wasn't found, but
   //  nonetheless sets STR to any identifier that was found.
   //
   Cxx::Directive NextDirective(std::string& str) const;

   //  Sets STR to the next keyword and returns its enum constant.  Returns
   //  NIL_KEYWORD if a keyword wasn't next, but nonetheless sets STR to any
   //  identifier that was found.
   //
   Cxx::Keyword NextKeyword(std::string& str) const;

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

   //  Starts at POS and searches for a right-hand character (RHC) that matches
   //  a left-hand one (LHC) that was just found (e.g. (), [], {}, <>).  If POS
   //  is not provided, POS is set to curr_.  For each LHC that is found, an
   //  additional RHC must be found.  Returns the position where the matching
   //  RHC was found, else string::npos.
   //
   size_t FindClosing(char lhc, char rhc, size_t pos = std::string::npos) const;

   //  Searches for the next occurrence of a character in TARGS.  Returns the
   //  position of the first character that was found, else string::npos.  If
   //  POS is not provided, POS is set to curr_.  The character must be at the
   //  same lexical level: that is, when any of { ( [ < " ' are encountered,
   //  the parse skips to the closing } ) ] > " ' (if, in the case of '<', a
   //  <...> pair enclose a template specification).
   //
   size_t FindFirstOf
      (const std::string& targs, size_t pos = std::string::npos) const;

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

   //  POS is the start of a line.  If the line is an #include directive, sets
   //  FILE to the included filename, sets ANGLE if FILE appeared within angle
   //  brackets, and returns true.
   //
   bool GetIncludeFile(size_t pos, std::string& file, bool& angle) const;

   //  Advances to where compilation should continue after OPT.  If COMPILE is
   //  set, the code following OPT is to be compiled, else it is to be skipped.
   //
   void FindCode(OptionalCode* opt, bool compile);

   //  Scans the code to determine lexical levels for indentation.
   //
   void CalcDepths();

   //  Scans the code to determine each line's LineType.  LOG is set if warnings
   //  are to be generated.
   //
   void CalcLineTypes(bool log);

   //  Returns 0 or 1 if line N and the following line can be merged and remain
   //  within the line length limit, with 1 indicating that a space needs to be
   //  inserted when merging them.  Returns -1 if the lines cannot merge.
   //
   int CheckLineMerge(size_t n) const;

   //  A type for the LineInfo associated with each line of source code.
   //
   typedef std::vector< LineInfo > LineInfoVector;

   //  Returns the information associated with each line of source code.
   //
   const LineInfoVector& GetLinesInfo() const { return lines_; }

   //  Returns the LineInfo for POS's line.
   //
   const LineInfo* GetLineInfo(size_t pos) const;

   //  Returns the start of the line that contains POS.
   //
   size_t CurrBegin(size_t pos) const;

   //  Returns the end of the line that contains POS.  It will be at an endline.
   //
   size_t CurrEnd(size_t pos) const;

   //  Returns the start of the line that precedes POS's line.
   //
   size_t PrevBegin(size_t pos) const;

   //  Returns the start of the line that follows POS's line.
   //
   size_t NextBegin(size_t pos) const;

   //  Returns the type of line for the Nth line.
   //
   LineType LineToType(size_t n) const;

   //  Returns the type of line in which POS occurs.
   //
   LineType PosToType(size_t pos) const;

   //  Returns true if POS's line is empty or contains only whitespace.
   //
   bool IsBlankLine(size_t pos) const;

   //  Returns the length of POS's line, including its CRLF.
   //
   size_t LineSize(size_t pos) const;

   //  Returns true if there is no CRLF between POS1 and POS2.
   //
   bool OnSameLine(size_t pos1, size_t pos2) const;

   //  Uses string::compare to compare the code starting at POS with STR.
   //  Returns -2 if POS is out of range.
   //
   int CompareCode(size_t pos, const std::string& str) const;

   //  Returns the position of any comment that follows POS on its line.
   //
   size_t FindComment(size_t pos) const;

   //  Returns true if line N has code and then a comment at OFFSET from
   //  its start.
   //
   bool LineHasTrailingCommentAt(size_t n, size_t offset) const;

   //  Returns the position of the first non-blank character on POS's line.
   //  Does *not* skip non-code (that is, literals and comments).
   //
   size_t LineFindFirst(size_t pos) const;

   //  Returns the position of the next non-blank character on POS's line,
   //  starting at POS.  Does *not* skip non-code.
   //
   size_t LineFindNext(size_t pos) const;

   //  Returns true if POS is the location of the first non-blank on its line.
   //
   bool IsFirstNonBlank(size_t pos) const;

   //  Returns true if the rest of the line that follows POS contains no code.
   //
   bool NoCodeFollows(size_t pos) const;

   //  Looks for STR starting at POS.  If STR is found, returns its position,
   //  else returns string::npos.  Ignores non-code and does not proceed to
   //  subsequent lines.
   //
   size_t LineFind(size_t pos, const std::string& str) const;

   //  The same as LineFind(pos, str), but searches backwards.
   //
   size_t LineRfind(size_t pos, const std::string& str) const;

   //  Returns the first occurrence of a character in CHARS, starting at POS.
   //  Does not proceed to subsequent lines.  Returns string::npos if no such
   //  character was found.
   //
   size_t LineFindFirstOf(size_t pos, const std::string& chars) const;

   //  The same as LineFindFirstOf, but searches backwards.
   //
   size_t LineRfindFirstOf(size_t pos, const std::string& chars) const;

   //  The same as LineFind, but looks for the next non-blank character.
   //
   size_t LineFindNonBlank(size_t pos) const;

   //  The same as LineFindNonBlank, but searches backwards.
   //
   size_t LineRfindNonBlank(size_t pos) const;

   //  The same as LineFind, but continues to subsequent lines.
   //
   size_t Find(size_t pos, const std::string& str) const;

   //  The same as LineRfind, but continues to previous lines.
   //
   size_t Rfind(size_t pos, const std::string& str) const;

   //  The same as Find, but looks for the next non-blank character.
   //
   size_t FindNonBlank(size_t pos) const;

   //  The same as FindNonBlank, but searches backwards.
   //
   size_t RfindNonBlank(size_t pos) const;

   //  Returns the first occurrence of a character in CHARS, starting at POS
   //  and reversing.  Returns string::npos if no such character was found.
   //
   size_t RfindFirstOf(size_t pos, const std::string& chars) const;

   //  Returns the position of the next character to parse, starting at POS.
   //  The result is POS unless characters are skipped (namely whitespace,
   //  comments, and character and string literals).  Returns string::npos
   //  if the end of source_ is reached.
   //
   size_t NextPos(size_t pos) const;

   //  Advances POS to the start of the next identifier, which is supplied
   //  in ID.  The identifier could be a keyword or preprocessor directive.
   //  Returns true if an identifier was found, else false.  If TOKENIZE is
   //  set, true is returned if a numeric or punctuation is found, in which
   //  case ID is set to "$" and curr_ advances to the first position after
   //  a numeric, whereas it remains at punctuation.
   //
   bool FindIdentifier(size_t& pos, std::string& id, bool tokenize) const;

   //  Returns the location of ID, starting at POS.  Returns string::npos
   //  if STR was not found.  STR must be an identifier or keyword that is
   //  delimited by punctuation.
   //
   size_t FindWord(size_t pos, const std::string& id) const;

   //  Characters in the string returned by CheckVerticalSpacing.
   //
   static const char LineOK = '-';
   static const char InsertBlank = 'b';
   static const char ChangeToEmptyComment = 'c';
   static const char DeleteLine = 'd';

   //  Check vertical spacing.  Returns a string that indicates how each line
   //  should be modified (see above).
   //
   std::string CheckVerticalSpacing() const;

   //  Checks the spacing of punctuation.
   //
   void CheckPunctuation() const;

   //  Checks the depth of line N.  Returns -1 if the depth is correct, else
   //  returns the correct number of indentation levels.
   //
   NodeBase::word CheckDepth(size_t n) const;

   //  Checks that each case label in the switch statement reference by CODE
   //  is either preceded by a break statement or a [[fallthrough]].
   //
   void CheckSwitch(const Switch& code) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
protected:
   //  Returns the LineInfo for POS's line.
   //
   LineInfo* GetLineInfo(size_t pos);

   //  Invoked to adjust LineInfo records after editing the code.
   //
   void Update();
private:
   //  Clears all LineInfo records and create new ones that contain the
   //  start position of each line.
   //
   void FindLines();

   //  Classifies the Nth line of code and looks for some warnings.  LOG
   //  is set if warnings should be generated.  Sets CONT when a line of
   //  code continues on the next line.
   //
   LineType CalcLineType(size_t n, bool log, bool& cont);

   //  Returns the next identifier (which could be a keyword), starting at POS.
   //  The first character not allowed in an identifier finalizes the string.
   //
   std::string NextIdentifier(size_t pos) const;

   //  Returns the next token.  This is a non-empty string from NextIdentifier
   //  or, failing that, a sequence of valid operator characters.  The first
   //  character not allowed in an identifier (or an operator) finalizes the
   //  string.
   //
   std::string NextToken() const;

   //  Returns the next operator (punctuation only), starting at POS.
   //
   std::string NextOperator(size_t pos) const;

   //  Advances over the literal that begins at POS.  Returns the position of
   //  the character that closes the literal.
   //
   size_t SkipCharLiteral(size_t pos) const;

   //  Advances over the literal that begins at POS.  Returns the position of
   //  the character that closes the literal.  Sets FRAGMENTED if the string
   //  is of the form ("<string>"<whitespace>)*"<string>".
   //
   size_t SkipStrLiteral(size_t pos, bool& fragmented) const;

   //  POS is the location of a '<'.  If a template specification follows,
   //  returns the location of the '>' at the end of the specification, else
   //  returns string::npos.
   //
   size_t SkipTemplateSpec(size_t pos) const;

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

   //  Skips a number that starts at POS.  Returns false if a number does not
   //  appear at POS, else updates POS to the position after the number and
   //  returns true.
   //
   bool SkipNum(size_t& pos) const;

   //  Advances curr_ to the start of the next preprocessor directive, which
   //  is returned.
   //
   Cxx::Directive FindDirective();

   //  Sets all lines from nextLine_ to curr_ to DEPTH1, and all lines after
   //  curr_ to the next parse position to DEPTH2.  If either range spans
   //  multiple lines, subsequent lines are marked as continuations of the
   //  first line in the range.  If MERGE is false, the lines are marked as
   //  not mergeable.
   //
   void SetDepth(int depth1, int depth2, bool merge = true);

   //  Returns true if the colon at POS shouldn't be preceded by a space.
   //
   bool NoSpaceBeforeColon(size_t pos) const;

   //  Returns the indentation level of the line after POS. Divides by
   //  IndentSize(), so truncation can occur.
   //
   size_t NextLineIndentation(size_t pos) const;

   //  The code being analyzed.
   //
   const std::string* source_;

   //  The file, if any, from which the code was taken.
   //
   CodeFile* file_;

   //  Set if a /* comment is open during CalcLineTypes.
   //
   bool slashAsterisk_;

   //  Information about each line.
   //
   LineInfoVector lines_;

   //  The current position within source_.
   //
   size_t curr_;

   //  The location when Advance was invoked (that is, the character
   //  immediately after the last one that was parsed).
   //
   size_t prev_;

   //  The next line whose depth needs to be set by SetDepth.
   //
   size_t nextLine_;
};
}
#endif
