//==============================================================================
//
//  CliBuffer.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef CLIBUFFER_H_INCLUDED
#define CLIBUFFER_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <ios>
#include <list>
#include <memory>
#include <string>
#include "CliParm.h"
#include "SysTypes.h"

namespace NodeBase
{
   struct CliSource;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For reading and parsing user input.
//
class CliBuffer : public Temporary
{
   friend std::unique_ptr<CliBuffer>::deleter_type;
   friend class CliThread;
public:
   //  The character that explicitly tags an optional parameter.
   //
   static const char OptTagChar;

   //  Highlights faulty user input during command parsing.
   //
   static fixed_string ErrorPointer;

   //  Looks for a string in the input stream, supplying it in S and
   //  returning Ok on success.  If the string had a tag prefix, it
   //  is supplied in T.
   //
   CliParm::Rc GetStr(std::string& t, std::string& s);

   //  Converts S to an integer, supplying it in N and returning Ok
   //  on success.  HEX is true if the integer is in hex.
   //
   static CliParm::Rc GetInt(const std::string& s, word& n, bool hex);

   //  Returns the current parse location in the input stream.
   //
   std::streamsize Pos() const { return pos_; }

   //  Sets the current location in the input stream.
   //
   void SetPos(std::streamsize p) { pos_ = p; }

   //  An error was detected at offset P in the buffer.  (If P is left as
   //  -1, the current offset, pos_, is used.)  If input is being taken
   //  from the console, generate a string that contains as many leading
   //  blanks as the length of the current CLI prompt, plus the user's
   //  input, up to P.  Append an error pointer to this string and output
   //  it.  Then output EXPL and, if input is being read from a file, add
   //  the line and column where the error occurred.  Clears buff_ before
   //  returning so that the rest of the input line will be ignored.
   //
   void ErrorAtPos(const CliThread& cli,
      const std::string& expl, std::streamsize p = -1);

   //  Returns the rest (unread portion) of the input line in S.
   //
   void Read(std::string& s);

   //  Outputs the rest (unread portion) of the input line to the console.
   //
   void Echo();

   //  Opens NAME.txt for reading input.  Returns 0 on success.  Returns
   //  another value on failure after updating EXPL with an explanation.
   //
   word OpenInputFile(const std::string& name, std::string& expl);

   //  Returns true if input is being taken from a file.
   //
   bool ReadingFromFile() const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Special characters.
   //
   enum CharType
   {
      Regular,    // character has no special interpretation
      Blank,      // white space
      EndOfLine,  // reached end of input stream
      String,     // string delimiter
      OptSkip,    // skip an optional parameter
      OptTag,     // advance to the optional parameter that has this tag
      Symbol      // treat the next string as a symbol and look up its value
   };

   //  Not subclassed.  Only created by CliThread.
   //
   CliBuffer();

   //  Not subclassed.  Only deleted by CliThread.
   //
   ~CliBuffer();

   //  Reads an input line into buff_.  Any results are written to cli.obuf.
   //  Returns StreamOk on success.  See StreamRc for failure codes.
   //
   std::streamsize GetLine(const CliThread& cli);

   //  Verifies that the current input line contains no illegal characters.
   //  Also handles EscapeChar, StringChar, BreakChar, and CommentChar.
   //  Returns StreamOk or StreamBadChar.
   //
   std::streamsize ScanLine(const CliThread& cli);

   //  Fetches the next command from the current input line.  Returns false
   //  if there are no more commands.
   //
   bool GetNextInput();

   //  Returns all of the current input line.
   //
   std::string GetInput() const;

   //  Returns the type of character at pos_, handling the escape and comment
   //  characters.  QUOTED is set if a string literal is being constructed.
   //
   CharType CalcType(bool quoted);

   //  Skips over spaces to find the beginning of the next string.  Returns
   //  false if the end of the input stream is reached.
   //
   bool FindNextNonBlank();

   //  Looks for a symbol ("&<name>").  Updates S to the symbol's value if
   //  the symbol is found.
   //
   CliParm::Rc GetSymbol(std::string& s);

   //  Treats INPUT as if it had been entered as a command.  Any results are
   //  written to cli.obuf.  Returns StreamOk, StreamEmpty, or StreamBadChar.
   //
   std::streamsize PutLine(const CliThread& cli, const std::string& input);

   //  Releases all input files so that input will again be taken from the
   //  console.
   //
   void Reset();

   //  Contains the input currently being processed.
   //
   std::string buff_;

   //  Index of the next character to be read from buff_.
   //
   size_t pos_;

   //  The source(s) for CLI input.  This acts as a stack, with the console
   //  at the bottom, and files from which input is being read being pushed
   //  onto the stack.
   //
   std::list<CliSource> sources_;
};
}
#endif
