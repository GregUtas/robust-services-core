//==============================================================================
//
//  CliBuffer.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLIBUFFER_H_INCLUDED
#define CLIBUFFER_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <ios>
#include <memory>
#include <string>
#include "CliParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For reading and parsing user input.
//
class CliBuffer : public Temporary
{
   friend std::unique_ptr< CliBuffer >::deleter_type;
   friend class CliThread;
public:
   //> The character that prevents the next one from being interpreted in a
   //  special way.
   //
   static const char EscapeChar;

   //> The character that precedes and follows a string that contains blanks
   //  or special characters.
   //
   static const char StringChar;

   //> The character that causes the remainder of an input line to be ignored.
   //
   static const char CommentChar;

   //> The character that explicitly skips an optional parameter.
   //
   static const char OptSkipChar;

   //> The character that explicitly tags an optional parameter.
   //
   static const char OptTagChar;

   //> The character that precedes a symbol's name to obtain its value.
   //
   static const char SymbolChar;

   //> Highlights faulty user input during command parsing.
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
   static CliParm::Rc GetInt(std::string& s, word& n, bool hex);

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
   //  the line and column where the error occurred.
   //
   void ErrorAtPos
      (CliThread& cli, const std::string& expl, std::streamsize p = -1) const;

   //  Returns the rest (unread portion) of the input line in S.
   //
   void Read(std::string& s);

   //  Outputs the rest (unread portion) of the input line to the console.
   //
   void Print();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
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
   std::streamsize GetLine(CliThread& cli);

   //  Verifies that the current input line contains no illegal characters.
   //  Returns StreamOk or StreamBadChar.
   //
   std::streamsize ScanLine(CliThread& cli);

   //  Returns a string that echoes the user's input.
   //
   std::string Echo() const;

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
   std::streamsize PutLine(CliThread& cli, const std::string& input);

   //> The maximum number of characters in a line of user input.
   //
   static const size_t BuffSize = 132;

   //  Buffer for user input.
   //
   char buff_[BuffSize];

   //  The number of characters in buff_.
   //
   std::streamsize size_;

   //  Index of the next character to be read from buff_.
   //
   std::streamsize pos_;
};
}
#endif
