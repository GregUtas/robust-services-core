//==============================================================================
//
//  CliBuffer.cpp
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
#include "CliBuffer.h"
#include <cctype>
#include <iosfwd>
#include <sstream>
#include <utility>
#include "CinThread.h"
#include "CliThread.h"
#include "CoutThread.h"
#include "Debug.h"
#include "Element.h"
#include "FileSystem.h"
#include "FileThread.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "NbCliParms.h"
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
//  A source from which CLI input is being taken.
//
struct CliSource
{
   CliSource() : file_(nullptr) { }

   explicit CliSource(istreamPtr& file) : file_(std::move(file)) { }

   //  The file from which input is being taken.  If nullptr, input is taken
   //  from the console.
   //
   istreamPtr file_;

   //  The current commands from the input stream.  There is usually one, but
   //  the break character allows a line to contain more than one command.
   //
   std::list<std::string> inputs_;
};

//------------------------------------------------------------------------------
//
//  The character that prevents the next one from being interpreted in a
//  special way.
//
constexpr char EscapeChar = BACKSLASH;

//  The character that precedes and follows a string that contains blanks
//  or special characters.
//
constexpr char StringChar = QUOTE;

//  The character that separates multiple commands entered on one line.
//
constexpr char BreakChar = ';';

//  The character that causes the remainder of an input line to be ignored.
//
constexpr char CommentChar = '/';

//  The character that explicitly skips an optional parameter.
//
constexpr char OptSkipChar = '~';

//  The character that precedes a symbol's name to obtain its value.
//
constexpr char SymbolChar = '&';

//  The maximum depth of nesting when >read obtains input from another file.
//
constexpr size_t MaxInputDepth = 8;

const char CliBuffer::OptTagChar = '=';
fixed_string CliBuffer::ErrorPointer = "_|";

//------------------------------------------------------------------------------

CliBuffer::CliBuffer() : pos_(0)
{
   Debug::ft("CliBuffer.ctor");

   sources_.push_front(CliSource());
}

//------------------------------------------------------------------------------

CliBuffer::~CliBuffer()
{
   Debug::ftnt("CliBuffer.dtor");
}

//------------------------------------------------------------------------------

CliBuffer::CharType CliBuffer::CalcType(bool quoted)
{
   Debug::ft("CliBuffer.CalcType");

   if(pos_ < buff_.size())
   {
      switch(buff_[pos_])
      {
      case EscapeChar:
         if(++pos_ >= buff_.size()) return EndOfLine;
         break;

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

      if(isblank(buff_[pos_]) && !quoted) return Blank;
      return Regular;
   }

   return EndOfLine;
}

//------------------------------------------------------------------------------

void CliBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   auto indent = prefix + spaces(2);

   stream << prefix << "buff : " << CRLF;
   stream << indent << buff_ << CRLF;
   stream << prefix << "pos  : " << pos_ << CRLF;
   stream << prefix << "source files : " << CRLF;

   for(auto s = sources_.cbegin(); s != sources_.cend(); ++s)
   {
      stream << indent << s->file_.get() << CRLF;
   }
}

//------------------------------------------------------------------------------

void CliBuffer::Echo()
{
   Debug::ft("CliBuffer.Echo");

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

void CliBuffer::ErrorAtPos
   (const CliThread& cli, const string& expl, std::streamsize p)
{
   Debug::ft("CliBuffer.ErrorAtPos");

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
      if(isblank(buff_[i]))
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
      for(size_t i = p; (i < buff_.size()) && isblank(buff_[i]); ++i)
      {
         *cli.obuf << buff_[i];
      }
   }

   *cli.obuf << ErrorPointer << CRLF;
   *cli.obuf << spaces(2) << expl << CRLF;

   //  Discard the rest of the input line that contained the error.
   //
   buff_.clear();
   sources_.front().inputs_.clear();
}

//------------------------------------------------------------------------------

bool CliBuffer::FindNextNonBlank()
{
   Debug::ft("CliBuffer.FindNextNonBlank");

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

string CliBuffer::GetInput() const
{
   Debug::ft("CliBuffer.GetInput");

   return buff_;
}

//------------------------------------------------------------------------------

CliParm::Rc CliBuffer::GetInt(const string& s, word& n, bool hex)
{
   Debug::ft("CliBuffer.GetInt");

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

   //  If the last line contained multiple commands, echo the next one and
   //  process it.
   //
   auto& source = sources_.front();

   if(GetNextInput())
   {
      //  The previous line contained multiple commands.  Echo each subsequent
      //  command to the console and console transcript file before processing
      //  it.
      //
      ostringstreamPtr echo(new std::ostringstream);
      *echo << buff_ << CRLF;
      CoutThread::Spool(echo);
      FileThread::Record(buff_, true);
      return StreamOk;
   }

   //  If input isn't being read from a file, read from the console and copy
   //  the input to the console transcript file.  This is the only time that
   //  we directly write to this file, because CoutThread copies everything
   //  to it.  Here, we don't want to copy console input back to the console,
   //  so we must bypass CoutThread.
   //
   if(source.file_ == nullptr)
   {
      auto count = CinThread::GetLine(buff_);
      if(count <= 0) return count;
      FileThread::Record(buff_, true);
      return ScanLine(cli);
   }

   //  Input is being read from a file.
   //
   if(source.file_->eof())
   {
      FunctionGuard guard(Guard_MakePreemptable);
      sources_.pop_front();
      return StreamEof;
   }

   ThisThread::EnterBlockingOperation(BlockedOnStream, CliBuffer_GetLine);
   {
      FileSystem::GetLine(*source.file_, buff_);
   }
   ThisThread::ExitBlockingOperation(CliBuffer_GetLine);

   if(source.file_->fail())
   {
      FunctionGuard guard(Guard_MakePreemptable);
      sources_.pop_front();
      return StreamFailure;
   }

   if(buff_.empty()) return StreamEmpty;

   //  Echo the input to the console.
   //
   ostringstreamPtr echo(new std::ostringstream);
   *echo << buff_ << CRLF;
   CoutThread::Spool(echo);
   return ScanLine(cli);
}

//------------------------------------------------------------------------------

bool CliBuffer::GetNextInput()
{
   Debug::ft("CliBuffer.GetNextInput");

   auto& source = sources_.front();

   if(source.inputs_.empty()) return false;

   buff_ = source.inputs_.front();
   source.inputs_.pop_front();
   pos_ = 0;
   return true;
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
         Debug::SwLog(CliBuffer_GetStr, "unknown type", type);
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

CliParm::Rc CliBuffer::GetSymbol(string& s)
{
   Debug::ft("CliBuffer.GetSymbol");

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
      auto sym = Singleton<SymbolRegistry>::Instance()->FindSymbol(s);
      if(sym == nullptr) return CliParm::Error;
      s = sym->GetValue();
      return CliParm::Ok;
   }

   return CliParm::None;
}

//------------------------------------------------------------------------------

word CliBuffer::OpenInputFile(const string& name, string& expl)
{
   Debug::ft("CliBuffer.OpenInputFile");

   if(sources_.size() >= MaxInputDepth)
   {
      expl = TooManyInputStreams;
      return -7;
   }

   FunctionGuard guard(Guard_MakePreemptable);

   auto path = Element::InputPath() + PATH_SEPARATOR + name + ".txt";
   auto file = FileSystem::CreateIstream(path.c_str());

   if(file != nullptr)
   {
      sources_.push_front(CliSource(file));
      return 0;
   }

   expl = NoFileExpl;
   return -2;
}

//------------------------------------------------------------------------------

void CliBuffer::Patch(sel_t selector, void* arguments)
{
   Temporary::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

std::streamsize CliBuffer::PutLine(const CliThread& cli, const string& input)
{
   Debug::ft("CliBuffer.PutLine");

   //  Put INPUT in the buffer, echo it to the console, and scan it.
   //
   if(input.empty()) return StreamEmpty;
   buff_ = input;
   CoutThread::Spool(input.c_str(), true);
   return ScanLine(cli);
}

//------------------------------------------------------------------------------

void CliBuffer::Read(string& s)
{
   Debug::ft("CliBuffer.Read");

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

bool CliBuffer::ReadingFromFile() const
{
   return (sources_.front().file_ != nullptr);
}

//------------------------------------------------------------------------------

void CliBuffer::Reset()
{
   Debug::ft("CliBuffer.Reset");

   sources_.clear();
   sources_.push_front(CliSource());
}

//------------------------------------------------------------------------------

std::streamsize CliBuffer::ScanLine(const CliThread& cli)
{
   Debug::ft("CliBuffer.ScanLine");

   //  If the input ends with a CRLF, remove it.
   //
   if(!buff_.empty() && (buff_.back() == CRLF)) buff_.pop_back();

   auto& source = sources_.front();
   auto quoted = false;
   string s;

   for(size_t i = 0; i < buff_.size(); ++i)
   {
      switch(buff_[i])
      {
      case StringChar:
         s.push_back(buff_[i]);
         quoted = !quoted;
         break;

      case CommentChar:
         //
         //  Ignore the rest of the input unless within a string.
         //
         if(quoted)
            s.push_back(buff_[i]);
         else
            i = buff_.size();
         break;

      case EscapeChar:
         //
         //  Add the next character to the string.
         //
         if(++i < buff_.size()) s.push_back(buff_[i]);
         break;

      case BreakChar:
         //
         //  Save the string before the break character and start a new one
         //  unless this appears in a string.
         //
         if(quoted)
         {
            s.push_back(buff_[i]);
         }
         else
         {
            source.inputs_.push_back(s);
            s.clear();
         }
         break;

      default:
         if(isprint(buff_[i]))
         {
            s.push_back(buff_[i]);
         }
         else
         {
            ErrorAtPos(cli, "Illegal character encountered", i);
            return StreamBadChar;
         }
      }
   }

   //  Save the last accumulated string from the input stream and
   //  prepare to process the first command.
   //
   source.inputs_.push_back(s);
   GetNextInput();
   return StreamOk;
}
}
