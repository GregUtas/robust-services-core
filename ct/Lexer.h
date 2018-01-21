//==============================================================================
//
//  Lexer.h
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
#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
class Lexer
{
public:
   //  Initializes all fields to default values.
   //
   Lexer();

   //  Not subclassed.
   //
   ~Lexer() { }

   //  Initializes the lexer to assist with parsing SOURCE, which must not
   //  be nullptr.  Also invokes Advance() to position curr_ at the first
   //  valid parse position.
   //
   void Initialize(const std::string* source);

   //  Returns the character at POS.
   //
   char At(size_t pos) const { return source_->at(pos); }

   //  Returns the number of lines in source_.
   //
   size_t LineCount() const { return lines_; }

   //  Returns the line number on which POS occurs.  Returns string::npos
   //  if POS is out of range.
   //
   //  NOTE: Internally, line numbers start at 0.  When a line number is to
   //  ====  be displayed, it must be incremented.  Similarly, a line number
   //        obtained externally (e.g. via the CLI) must be decremented.
   //
   size_t GetLineNum(size_t pos) const;

   //  Returns the position of the first character in LINE.
   //
   size_t GetLineStart(size_t line) const;

   //  Returns the line that includes POS, with a '$' inserted after POS.
   //
   std::string GetLine(size_t pos) const;

   //  Sets S to the string for the Nth line of code, excluding the endline,
   //  or EMPTY_STR if N was out of range.  Returns true if N was valid.
   //
   bool GetNthLine(size_t n, std::string& s) const;

   //  Returns the string for the Nth line of code.  Returns EMPTY_STR if N
   //  is out of range.
   //
   std::string GetNthLine(size_t n) const;

   //  Returns a string containing the next LEN characters, starting at POS.
   //  Converts endlines to blanks and compresses adjacent blanks.
   //
   std::string Extract(size_t pos, size_t count) const;

   //  Returns the current parse position.
   //
   //  NOTE: Unless stated otherwise, a non-const Lexer function advances
   //  ====  curr_ to the next parse position after what was extracted.
   //
   size_t Curr() const { return curr_; }

   //  Returns the previous parse position.  Typically used to obtain the
   //  location where the last successful parse began.
   //
   size_t Prev() const { return prev_; }

   //  Returns true if curr_ has reached the end of source_.
   //
   bool Eof() const { return curr_ >= size_; }

   //  Sets prev_ to curr_, moves to the next parse position from there,
   //  and returns true.
   //
   bool Advance();

   //  Sets prev_ to curr_ + incr, moves to the next parse position from
   //  there, and returns true.
   //
   bool Advance(size_t incr);

   //  Sets prev_ to POS, moves to the next parse position from there, and
   //  returns true.
   //
   bool Reposition(size_t pos);

   //  Sets prev_ and curr_ to POS and returns false.  Used for backing up
   //  to a position that is known to be valid and trying another parse.
   //
   bool Retreat(size_t pos);

   //  Skips the current line and moves to the next parse position starting
   //  at the next line.  Returns true.
   //
   bool Skip();

   //  Returns the character at curr_.
   //
   char CurrChar() const { return source_->at(curr_); }

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
   bool NextStringIs(fixed_string str, bool check = true);

   //  Returns the position of the first occurrence of C before the current
   //  parse position.
   //
   size_t rfind(char c) const;

   //  Returns the location where the line containing POS ends.  This is the
   //  location of the next CRLF that is not preceded by a '\'.
   //
   size_t FindLineEnd(size_t pos) const;

   //  Until the next #define is reached, look for #defined symbols that map to
   //  empty strings and erase them so that they will not cause parsing errors.
   //
   void PreprocessSource() const;

   //  Returns true if ID, in its entirety, is a valid identifier.
   //
   static bool IsValidIdentifier(const std::string& id);

   //  Returns the next identifier (which could be a keyword).  The first
   //  character not allowed in an identifier finalizes the string.
   //
   std::string NextIdentifier() const;

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

   //  Returns the next operator (punctuation only).
   //
   std::string NextOperator() const;

   //  Searches for a right-hand character (RHC) that matches a left-hand one
   //  (LHC) that was just found (e.g. (), [], {}, <>) at POS.  If the default
   //  value of POS is used (string::npos), POS is set to curr_.  For each LHC
   //  that is found, an additional RHC must be found.  Returns the position
   //  where the matching RHC was found, else string::npos.
   //
   size_t FindClosing(char lhc, char rhc, size_t pos = std::string::npos) const;

   //  Searches for the next occurrence of a character in TARGS.  Returns the
   //  position of the first character that was found, else string::npos.  The
   //  character must be at the same lexical level, meaning that when any of
   //  { ( [ " ' are encountered, the parse "jumps" to the closing } ) ] " '.
   //  To keep things simple, a < is not matched with a >.  A template could
   //  therefore cause an error, in which case this function may need to be
   //  enhanced.
   //
   size_t FindFirstOf(const std::string& targs) const;

   //  Returns true and sets SPEC to the template specification that follows
   //  the name of a template instance.
   //
   bool GetTemplateSpec(std::string& spec);

   //  Returns true and creates or updates NAME on finding an identifier that
   //    Advances curr_ beyond NAME.
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

   //  Returns the operator associated with NAME.  This is something of a
   //  hack, used when the item in an expression begins with an alphabetic
   //  character.
   //
   static Cxx::Operator GetReserved(const std::string& name);

   //  Returns the built-in type associated with NAME.
   //
   static Cxx::Type GetType(const std::string& name);

   //  If the next token is a built-in type that can appear in a compound
   //  type, returns its Cxx::Type  and advances curr_ beyond it.  Returns
   //  Cxx::NIL_TYPE and leaves curr_ unchanged in other cases.
   //
   Cxx::Type NextType();

   //  If the next token is a numeric literal, creates a FloatLiteral or
   //  IntLiteral and returns it in ITEM.  Does *not* parse a unary + or -.
   //
   bool GetNum(TokenPtr& item);

   //  If the next token is a character literal, sets C to its value and
   //  returns true, else returns false.
   //
   bool GetChar(char& c);

   //  If the next token is a string literal, sets S to its value and returns
   //  true, else returns false.
   //
   bool GetStr(std::string& s);

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
   //  brackets, and returns TRUE.
   //
   bool GetIncludeFile(size_t pos, std::string& file, bool& angle) const;

   //  Advances to where compilation should continue after OPT.  If COMPILE is
   //  set, the code following OPT is to be compiled, else it is to be skipped.
   //
   void FindCode(OptionalCode* opt, bool compile);
private:
   //  Used by PreprocessSource, which creates a clone of "this" lexer to
   //  do the work.
   //
   void Preprocess();

   //  Returns the next token.  This is a non-empty string from NextIdentifier
   //  or, failing that, a sequence of valid operator characters.  The first
   //  character not allowed in an identifier (or an operator) finalizes the
   //  string.
   //
   std::string NextToken() const;

   //  Advances over the literal that begins at POS.  Returns the position of
   //  the character that closes the literal.
   //
   size_t SkipCharLiteral(size_t pos) const;

   //  Advances over the literal that begins at POS.  Returns the position of
   //  the character that closes the literal.  Sets FRAGMENTED if the string
   //  is of the form ("<string>"<whitespace>)*"<string>".
   //
   size_t SkipStrLiteral(size_t pos, bool& fragmented) const;

   //  Returns the position of the next character to parse, starting from POS.
   //  Skips comments and character and string literals.  Returns string::npos
   //  if the end of source_ is reached.
   //
   size_t NextPos(size_t pos) const;

   //  Parses an integer literal and returns it in NUM.  Returns the number
   //  of digits in NUM (zero if no literal was found).
   //
   size_t GetInt(int64_t& num);

   //  Parses a hex literal and returns it in NUM.  Returns the number of
   //  digits in NUM (zero if no literal was found).
   //
   size_t GetHex(int64_t& num);

   //  Parses an octal literal and returns it in NUM.  Returns the number of
   //  digits in NUM (zero if no literal was found).
   //
   size_t GetOct(int64_t& num);

   //  Parses the numbers after the decimal point in a floating point literal
   //  and adds them to NUM.
   //
   void GetFloat(long double& num);

   //  Advances curr_ to the start of the next identifier, which is supplied
   //  in ID.  The identifier could be a keyword or preprocessor directive.
   //  Returns true if an identifier was found, else false.
   //
   bool FindIdentifier(std::string& id);

   //  Advances curr_ to the start of the next preprocessor directive, which
   //  is returned.
   //
   Cxx::Directive FindDirective();

   //  Initializes the keyword and operator hash tables.
   //
   static bool Initialize();

   //  The code being analyzed.
   //
   const std::string* source_;

   //  The size of source_.
   //
   size_t size_;

   //  The number of lines in source_.
   //
   size_t lines_;

   //  The position where each line starts; it ends at a CRLF.
   //
   std::vector< size_t > start_;

   //  The current position within source_.
   //
   size_t curr_;

   //  The location when Advance was invoked (that is, the character
   //  immediately after the last one that was parsed).
   //
   size_t prev_;

   //  Entries in the directive hash table map a string to a Cxx::Directive.
   //
   typedef std::unordered_map< std::string, Cxx::Directive > DirectiveTable;
   typedef std::pair< std::string, Cxx::Directive > DirectivePair;
   typedef std::unique_ptr< DirectiveTable > DirectiveTablePtr;
   static DirectiveTablePtr Directives;

   //  Entries in the keyword hash table map a string to a Cxx::Keyword.
   //
   typedef std::unordered_map< std::string, Cxx::Keyword > KeywordTable;
   typedef std::pair< std::string, Cxx::Keyword > KeywordPair;
   typedef std::unique_ptr< KeywordTable > KeywordTablePtr;
   static KeywordTablePtr Keywords;

   //  Entries in the operator and reserved word hash tables map a string to a
   //  Cxx::Operator.  Each operator table contains punctuation strings, while
   //  the Reserved table contains alphabetic strings.  There are two opeartor
   //  tables, one for C++ code and one for preprocessor directives.
   //
   typedef std::unordered_map< std::string, Cxx::Operator > OperatorTable;
   typedef std::pair< std::string, Cxx::Operator > OperatorPair;
   typedef std::unique_ptr< OperatorTable > OperatorTablePtr;
   static OperatorTablePtr CxxOps;
   static OperatorTablePtr PreOps;
   static OperatorTablePtr Reserved;

   //  Entries in the types hash table map the string for a built-in type to a
   //  Cxx::Type.
   //
   typedef std::unordered_map< std::string, Cxx::Type > TypesTable;
   typedef std::pair< std::string, Cxx::Type > TypePair;
   typedef std::unique_ptr< TypesTable > TypesTablePtr;
   static TypesTablePtr Types;

   //  Set by invoking Initialize().
   //
   static bool Initialized;
};
}
#endif
