//==============================================================================
//
//  CliBuffer.cpp
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
#include "CliBuffer.h"
#include <cctype>
#include <iosfwd>
#include <sstream>
#include "CinThread.h"
#include "CliThread.h"
#include "CoutThread.h"
#include "Debug.h"
#include "FileThread.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "Singleton.h"
#include "Symbol.h"
#include "SymbolRegistry.h"
#include "ThisThread.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const char CliBuffer::EscapeChar = BACKSLASH;
const char CliBuffer::CommentChar = '/';
const char CliBuffer::StringChar = QUOTE;
const char CliBuffer::OptSkipChar = '~';
const char CliBuffer::OptTagChar = '=';
const char CliBuffer::SymbolChar = '&';
fixed_string CliBuffer::ErrorPointer = "_|";

//------------------------------------------------------------------------------

fn_name CliBuffer_ctor = "CliBuffer.ctor";

CliBuffer::CliBuffer() : pos_(0)
{
   Debug::ft(CliBuffer_ctor);
}

//------------------------------------------------------------------------------

fn_name CliBuffer_dtor = "CliBuffer.dtor";

CliBuffer::~CliBuffer()
{
   Debug::ftnt(CliBuffer_dtor);
}

//------------------------------------------------------------------------------

fn_name CliBuffer_CalcType = "CliBuffer.CalcType";

CliBuffer::CharType CliBuffer::CalcType(bool quoted)
{
   Debug::ft(CliBuffer_CalcType);

   if(pos_ < buff_.size())
   {
      switch(buff_[pos_])
      {
      case EscapeChar:
         //
         //  Erase this character and treat the next one literally.
         //
         buff_.erase(pos_, 1);
         break;

      case CommentChar:
         if(quoted) return Regular;
         pos_ = buff_.size();
         return EndOfLine;

      case StringChar:
         return String;

      case OptSkipChar:
         if(quoted) return Regular;
         return OptSkip;

      case OptTagChar:
         if(quoted) return Regular;
         return OptTag;

      case SymbolChar:
         if(quoted) return Regular;
         return Symbol;
      }

      if(isspace(buff_[pos_]) && !quoted) return Blank;
      return Regular;
   }

   return EndOfLine;
}

//------------------------------------------------------------------------------

void CliBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "buff : " << CRLF;
   stream << prefix << spaces(2) << buff_ << CRLF;
   stream << prefix << "pos  : " << pos_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name CliBuffer_Echo = "CliBuffer.Echo";

string CliBuffer::Echo() const
{
   Debug::ft(CliBuffer_Echo);

   string s = buff_;
   return s;
}

//------------------------------------------------------------------------------

fn_name CliBuffer_ErrorAtPos = "CliBuffer.ErrorAtPos";

void CliBuffer::ErrorAtPos(const CliThread& cli,
   const string& expl, std::streamsize p) const
{
   Debug::ft(CliBuffer_ErrorAtPos);

   //  Generate spaces to bypass the prompt.  ErrorPointer points one
   //  column to the right of where it starts, so subtract a space.
   //
   if(p < 0) p = pos_;
   auto n = cli.Prompt().size();
   if(n > 0) --n;
   *cli.obuf << spaces(n);

   //  Generate spaces up to P.  Space characters from the user's
   //  input are copied verbatim in order to handle tabs properly.
   //
   for(auto i = 0; i < p; ++i)
   {
      if(isspace(buff_[i]))
         *cli.obuf << buff_[i];
      else
         *cli.obuf << SPACE;
   }

   //  pos_ was at the *end* of the previous blank-terminated string,
   //  so the faulty input occurred at the end of any blank space that
   //  followed pos_.  Tabs are not accepted in the input stream and
   //  shouldn't be echoed at pos_ because this will put the error
   //  pointer at the end of the tab (the next character) rather than
   //  at the beginning.
   //
   if(buff_[p] != TAB)
   {
      for(size_t i = p; (i < buff_.size()) && isspace(buff_[i]); ++i)
      {
         *cli.obuf << buff_[i];
      }
   }

   *cli.obuf << ErrorPointer << CRLF;
   *cli.obuf << spaces(2) << expl << CRLF;
}

//------------------------------------------------------------------------------

fn_name CliBuffer_FindNextNonBlank = "CliBuffer.FindNextNonBlank";

bool CliBuffer::FindNextNonBlank()
{
   Debug::ft(CliBuffer_FindNextNonBlank);

   //  Return true if there is a non-blank character before the end
   //  of the line.
   //
   while(true)
   {
      switch(CalcType(false))
      {
      case Blank:
         ++pos_;
         break;
      case EndOfLine:
         return false;
      default:
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CliBuffer_GetInt = "CliBuffer.GetInt";

CliParm::Rc CliBuffer::GetInt(const string& s, word& n, bool hex)
{
   Debug::ft(CliBuffer_GetInt);

   if(s.empty()) return CliParm::None;

   std::istringstream input(s);

   if(hex)
      input >> std::hex >> n;
   else
      input >> std::dec >> n;

   return (input.fail() ? CliParm::Error : CliParm::Ok);
}

//------------------------------------------------------------------------------

fn_name CliBuffer_GetLine = "CliBuffer.GetLine";

std::streamsize CliBuffer::GetLine(const CliThread& cli)
{
   Debug::ft(CliBuffer_GetLine);

   //  If input isn't being read from a file, read from the console and copy
   //  the input to the console transcript file.  This is the only time that
   //  we directly write to this file, because CoutThread copies everything
   //  to it.  Here, we don't want to copy console input back to the console,
   //  so we must bypass CoutThread.
   //
   auto source = cli.InputFile();

   if(source == nullptr)
   {
      auto count = CinThread::GetLine(buff_);
      if(count <= 0) return count;
      FileThread::Record(buff_, true);
      return ScanLine(cli);
   }

   //  Input is being read from a file.
   //
   if(source->eof()) return StreamEof;

   ThisThread::EnterBlockingOperation(BlockedOnStream, CliBuffer_GetLine);
   {
      std::getline(*source, buff_);
   }
   ThisThread::ExitBlockingOperation(CliBuffer_GetLine);

   if(source->fail()) return StreamFailure;
   if(buff_.empty()) return StreamEmpty;

   //  Echo the input to the console.
   //
   ostringstreamPtr echo(new std::ostringstream);
   *echo << buff_ << CRLF;
   CoutThread::Spool(echo);
   return ScanLine(cli);
}

//------------------------------------------------------------------------------

fn_name CliBuffer_GetStr = "CliBuffer.GetStr";

CliParm::Rc CliBuffer::GetStr(string& t, string& s)
{
   Debug::ft(CliBuffer_GetStr);

   auto rc = CliParm::Ok;
   size_t quotes = 0;

   s.clear();
   t.clear();

   //  Skip white space.
   //
   if(!FindNextNonBlank()) return CliParm::None;

   for(auto done = false; !done; NO_OP)
   {
      bool add = false;
      auto type = CalcType(quotes == 1);

      switch(type)
      {
      case Regular:
         //
         //  Add the character to S.
         //
         add = true;
         break;

      case Blank:
      case EndOfLine:
         //
         //  We're done constructing S.
         //
         done = true;
         break;

      case String:
         //
         //  If we're assembling a delimited string, S is now complete.
         //  If not, start to assemble a delimited string.  Regardless,
         //  advance to next character.
         //
         if(++quotes == 2) done = true;
         ++pos_;
         break;

      case OptSkip:
         //
         //  If we're assembling a delimited string, include this character.
         //  Otherwise we're done: if S is empty, the intention is to skip
         //  an optional parameter, else S is now complete.
         //
         if(s.empty())
         {
            add = true;
            rc = CliParm::Skip;
         }
         done = true;
         break;

      case OptTag:
         //
         //  Include this character in S if S is empty or does not start with
         //  an alphabetic character.  If we've already found a tag, report an
         //  error.  Otherwise, set the tag to S and start to reconstruct S.
         //
         if(s.empty() || !isalpha(s.front()))
         {
            add = true;
         }
         else
         {
            if(!t.empty()) return CliParm::Error;
            t = s;
            s.clear();
            ++pos_;
         }
         break;

      case Symbol:
         //
         //  Look up the symbol that follows this character.  It's an
         //  error, however, if a string preceded this character.
         //
         if(s.empty()) return GetSymbol(s);
         return CliParm::Error;

      default:
         Debug::SwLog(CliBuffer_GetStr, pos_, type);
         return CliParm::Error;
      }

      if(add)
      {
         s += buff_[pos_++];
      }
   }

   //  If the above loop set a result, report it.  Also check for an
   //  incomplete string literal.
   //
   if(rc != CliParm::Ok) return rc;
   if(quotes == 1) return CliParm::Error;

   //  If S is empty, treat "" as a valid input.  Otherwise, we found nothing
   //  unless T isn't empty, which is an error (a tag without a value).
   //
   if(s.empty() && (quotes != 2))
   {
      if(t.empty()) return CliParm::None;
      return CliParm::Error;
   }

   return CliParm::Ok;
}

//------------------------------------------------------------------------------

fn_name CliBuffer_GetSymbol = "CliBuffer.GetSymbol";

CliParm::Rc CliBuffer::GetSymbol(string& s)
{
   Debug::ft(CliBuffer_GetSymbol);

   s.clear();

   //  Handle '&' as a special character.  Accumulate the string that
   //  follows it and look up its value in the symbol registry.
   //
   if(buff_[pos_++] == SymbolChar)
   {
      while(CalcType(false) == Regular)
      {
         s += buff_[pos_++];
      }

      if(s.empty()) return CliParm::Error;
      auto sym = Singleton< SymbolRegistry >::Instance()->FindSymbol(s);
      if(sym == nullptr) return CliParm::Error;
      s = sym->GetValue();
      return CliParm::Ok;
   }

   return CliParm::None;
}

//------------------------------------------------------------------------------

void CliBuffer::Patch(sel_t selector, void* arguments)
{
   Temporary::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliBuffer_Print = "CliBuffer.Print";

void CliBuffer::Print()
{
   Debug::ft(CliBuffer_Print);

   //  Skip white space.
   //
   if(!FindNextNonBlank()) return;

   string s;
   string t;

   //  Create a string that contains the rest of the input stream, but
   //  handle '&' as a special character for referencing a symbol.  If
   //  a symbol doesn't follow the '&', include it.
   //
   while(true)
   {
      switch(CalcType(false))
      {
      case EndOfLine:
         CoutThread::Spool(s.c_str(), true);
         return;

      case Symbol:
         if(GetSymbol(t) != CliParm::Ok) s += SymbolChar;
         s += t;
         break;

      default:
         s += buff_[pos_++];
      }
   }
}

//------------------------------------------------------------------------------

fn_name CliBuffer_PutLine = "CliBuffer.PutLine";

std::streamsize CliBuffer::PutLine(const CliThread& cli, const string& input)
{
   Debug::ft(CliBuffer_PutLine);

   //  Put INPUT in the buffer, echo it to the console, and scan it.
   //
   if(input.empty()) return StreamEmpty;
   buff_ = input;
   CoutThread::Spool(input.c_str(), true);
   return ScanLine(cli);
}

//------------------------------------------------------------------------------

fn_name CliBuffer_Read = "CliBuffer.Read";

void CliBuffer::Read(string& s)
{
   Debug::ft(CliBuffer_Read);

   s.clear();

   //  Skip white space and update S with the rest of the input stream.
   //
   if(!FindNextNonBlank()) return;

   while(pos_ < buff_.size())
   {
      s += buff_[pos_++];
   }
}

//------------------------------------------------------------------------------

fn_name CliBuffer_ScanLine = "CliBuffer.ScanLine";

std::streamsize CliBuffer::ScanLine(const CliThread& cli)
{
   Debug::ft(CliBuffer_ScanLine);

   //  Report failure if any input characters are non-printable.
   //
   for(pos_ = 0; pos_ < buff_.size(); ++pos_)
   {
      if(!isprint(buff_[pos_]))
      {
         ErrorAtPos(cli, "Illegal character encountered", pos_);
         return StreamBadChar;
      }
   }

   //  Reposition to the beginning of the buffer and report success.
   //
   pos_ = 0;
   return StreamOk;
}
}
