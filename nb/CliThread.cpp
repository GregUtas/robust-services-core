//==============================================================================
//
//  CliThread.cpp
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
#include "CliThread.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ios>
#include <istream>
#include <sstream>
#include <utility>
#include "CinThread.h"
#include "CliBuffer.h"
#include "CliCommand.h"
#include "CliRegistry.h"
#include "CliStack.h"
#include "CoutThread.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "FileThread.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "NbDaemons.h"
#include "NbIncrement.h"
#include "PosixSignal.h"
#include "PosixSignalRegistry.h"
#include "Restart.h"
#include "Singleton.h"
#include "Symbol.h"
#include "SymbolRegistry.h"
#include "SysFile.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
constexpr size_t MaxInputDepth = 8;
constexpr size_t MaxOutputDepth = 8;

const char CliThread::CliPrompt = '>';

//------------------------------------------------------------------------------

fn_name CliThread_ctor = "CliThread.ctor";

CliThread::CliThread() :
   Thread(OperationsFaction, Singleton< CliDaemon >::Instance()),
   skip_(false),
   command_(nullptr),
   result_(0)
{
   Debug::ft(CliThread_ctor);

   AllocResources();
   SetInitialized();
}

//------------------------------------------------------------------------------

fn_name CliThread_dtor = "CliThread.dtor";

CliThread::~CliThread()
{
   Debug::ftnt(CliThread_dtor);

   auto thread = Singleton< CinThread >::Extant();
   if(thread != nullptr) thread->ClearClient(this);
}

//------------------------------------------------------------------------------

c_string CliThread::AbbrName() const
{
   return CliDaemonName;
}

//------------------------------------------------------------------------------

fn_name CliThread_AllocResources = "CliThread.AllocResources";

void CliThread::AllocResources()
{
   Debug::ft(CliThread_AllocResources);

   ibuf.reset(new CliBuffer);
   obuf.reset(new std::ostringstream);
   *obuf << std::boolalpha << std::nouppercase;
   stack_.reset(new CliStack);
   prompt_.clear();
   skip_ = false;
   outFiles_.clear();
   inFiles_.clear();
}

//------------------------------------------------------------------------------

fixed_string YesNoChars = "yn";
fixed_string YesNoHelp = "Enter y(yes) or n(no): ";

fn_name CliThread_BoolPrompt = "CliThread.BoolPrompt";

bool CliThread::BoolPrompt(const string& prompt)
{
   Debug::ft(CliThread_BoolPrompt);

   return (CharPrompt(prompt, YesNoChars, YesNoHelp) == 'y');
}

//------------------------------------------------------------------------------

fn_name CliThread_CharPrompt = "CliThread.CharPrompt";

char CliThread::CharPrompt
   (const string& prompt, const string& chars, const string& help, bool upper)
{
   Debug::ft(CliThread_CharPrompt);

   //  If input is being taken from a file rather than the console,
   //  return the first character.
   //
   if(chars.empty()) return NUL;
   if(!inFiles_.empty()) return chars.front();

   auto first = true;
   string text;

   //  Output the query until the user enters a character in CHARS.
   //  Echo the user's input to the console transcript file.
   //
   while(true)
   {
      ostringstreamPtr stream(new std::ostringstream);

      if(first)
         *stream << prompt << " [" << chars << "]: ";
      else
         *stream << help;

      Flush();
      CoutThread::Spool(stream);
      first = false;

      auto count = CinThread::GetLine(text);

      if(count < 0) return NUL;

      FileThread::Record(text, true);

      if(count == 1)
      {
         auto c = (upper ? text[0] : tolower(text[0]));
         if(chars.find(c) != string::npos) return c;
      }
   }
}

//------------------------------------------------------------------------------

fn_name CliThread_Destroy = "CliThread.Destroy";

void CliThread::Destroy()
{
   Debug::ft(CliThread_Destroy);

   Singleton< CliThread >::Destroy();
}

//------------------------------------------------------------------------------

void CliThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   stream << prefix << "ibuf : " << CRLF;
   ibuf->Display(stream, lead1, options);
   stream << prefix << "obuf : " << obuf.get() << CRLF;
   stream << prefix << "stack : " << CRLF;
   stack_->Display(stream, lead1, options);
   stream << prefix << "prompt   : " << prompt_ << CRLF;
   stream << prefix << "skip     : " << skip_ << CRLF;
   stream << prefix << "command  : " << strObj(command_) << CRLF;
   stream << prefix << "cookie   : " << CRLF;
   cookie_.Display(stream, lead1, options);
   stream << prefix << "result   : " << result_ << CRLF;
   stream << prefix << "stream   : " << stream_.get() << CRLF;

   stream << prefix << "outFiles : " << CRLF;
   for(size_t i = 0; i < outFiles_.size(); ++i)
   {
      stream << lead1 << strIndex(i) << outFiles_.at(i) << CRLF;
   }

   stream << prefix << "inFiles : " << CRLF;
   for(size_t i = 0; i < inFiles_.size(); ++i)
   {
      stream << lead1 << strIndex(i) << inFiles_.at(i).get() << CRLF;
   }

   stream << prefix << "appsData : " << CRLF;
   for(auto a = appsData_.cbegin(); a != appsData_.cend(); ++a)
   {
      stream << lead1 << strIndex(a->first) << CRLF;
      a->second->Display(stream, lead2, options);
   }
}

//------------------------------------------------------------------------------

fn_name CliThread_DisplayHelp = "CliThread.DisplayHelp";

word CliThread::DisplayHelp(const string& path, const string& key) const
{
   Debug::ft(CliThread_DisplayHelp);

   //  Open the help file addressed by PATH.
   //
   auto stream = SysFile::CreateIstream(path.c_str());
   if(stream == nullptr) return -2;

   //  Find line that contains "? KEY" and display the lines that follow, up
   //  to the next line that begins with '?'.  If a line begins with '?' and
   //  ends with '*', it is a wildcard that matches KEY if KEY's begins with
   //  the same characters as those that precede the asterisk.
   //
   auto found = false;
   string line;

   while(stream->peek() != EOF)
   {
      std::getline(*stream, line);

      if(line.empty())
      {
         if(found) *obuf << CRLF;
         continue;
      }

      switch(line.front())
      {
      case '/':
         continue;

      case '?':
         if(found) return 0;
         while(line.back() == SPACE) line.pop_back();
         line.erase(0, 2);

         if(!line.empty() && line.back() == '*')
         {
            auto keyStart = key.substr(0, line.size() - 1);
            auto lineStart = line.substr(0, line.size() - 1);
            if(strCompare(lineStart, keyStart) == 0) found = true;
         }
         else
         {
            if(strCompare(line, key) == 0) found = true;
         }
         break;

      default:
         if(found) *obuf << line << CRLF;
      }
   }

   return (found ? 0 : -1);
}

//------------------------------------------------------------------------------

fn_name CliThread_EndOfInput = "CliThread.EndOfInput";

bool CliThread::EndOfInput(bool error) const
{
   Debug::ft(CliThread_EndOfInput);

   if(!ibuf->FindNextNonBlank()) return true;

   if(error)
      ibuf->ErrorAtPos(*this, "Error: extra input");
   else
      ibuf->ErrorAtPos(*this, "Extra input ignored");

   return false;
}

//------------------------------------------------------------------------------

fn_name CliThread_Enter = "CliThread.Enter";

void CliThread::Enter()
{
   Debug::ft(CliThread_Enter);

   //  Put the root increment on the stack and start reading commands.
   //
   stack_->SetRoot(*Singleton< NbIncrement >::Instance());
   while(true) ReadCommands();
}

//------------------------------------------------------------------------------

fn_name CliThread_Execute = "CliThread.Execute";

word CliThread::Execute(const string& input)
{
   Debug::ft(CliThread_Execute);

   word result = -1;

   Flush();
   auto rc = ibuf->PutLine(*this, input);

   if(rc == StreamOk)
   {
      auto comm = ParseCommand();
      if(comm != nullptr) result = InvokeCommand(*comm);
   }

   Flush();
   return result;
}

//------------------------------------------------------------------------------

fn_name CliThread_FileStream = "CliThread.FileStream";

ostream* CliThread::FileStream()
{
   Debug::ft(CliThread_FileStream);

   stream_ = FileThread::CreateStream();
   return stream_.get();
}

//------------------------------------------------------------------------------

fn_name CliThread_Flush = "CliThread.Flush";

void CliThread::Flush()
{
   Debug::ft(CliThread_Flush);

   //  Send output to either the console or a separate file.
   //
   if((obuf != nullptr) && (obuf->tellp() > 0))
   {
      if(outFiles_.empty())
         CoutThread::Spool(obuf);
      else
         FileThread::Spool(outFiles_.back(), obuf);
   }

   //  Create a new output buffer for the next command's results.
   //
   obuf.reset(new std::ostringstream);
   *obuf << std::boolalpha << std::nouppercase;
}

//------------------------------------------------------------------------------

fn_name CliThread_GenerateReportPreemptably =
   "CliThread.GenerateReportPreemptably";

bool CliThread::GenerateReportPreemptably()
{
   Debug::ft(CliThread_GenerateReportPreemptably);

   //  Generate the report preemptably unless tracing is on in the lab
   //  and the user specifically wants to trace report generation.
   //
   if(Debug::TraceOn())
   {
      if(!Element::RunningInLab()) return true;

      if(BoolPrompt(StopTracingPrompt))
      {
         Singleton< TraceBuffer >::Instance()->StopTracing();
         return true;
      }

      if(BoolPrompt(TraceReportPrompt)) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CliThread_GetAppData = "CliThread.GetAppData";

CliAppData* CliThread::GetAppData(CliAppData::Id aid) const
{
   Debug::ft(CliThread_GetAppData);

   auto a = appsData_.find(aid);
   if(a != appsData_.cend()) return a->second.get();
   return nullptr;
}

//------------------------------------------------------------------------------

std::istream* CliThread::InputFile() const
{
   return (inFiles_.empty() ? nullptr : inFiles_.back().get());
}

//------------------------------------------------------------------------------

fn_name CliThread_IntPrompt = "CliThread.IntPrompt";

word CliThread::IntPrompt(const string& prompt, word min, word max)
{
   Debug::ft(CliThread_IntPrompt);

   word result = -1;

   //  If input is being taken from a file rather than the console,
   //  return 0.
   //
   if(!inFiles_.empty()) return 0;

   auto first = true;
   string text;

   //  Output the query until the user enters an integer.  Echo the
   //  user's input to the console transcript file.
   //
   while(true)
   {
      ostringstreamPtr stream(new std::ostringstream);

      if(first)
         *stream << prompt;
      else
         *stream << "Enter " << min << " to " << max << ": ";

      Flush();
      CoutThread::Spool(stream);
      first = false;

      auto count = CinThread::GetLine(text);
      if(count < 0) return result;

      string input(text);
      FileThread::Record(input, true);
      auto rc = CliBuffer::GetInt(input, result, false);
      if(rc != CliParm::Ok) continue;
      if((result >= min) && (result <= max)) return result;
   }
}

//------------------------------------------------------------------------------

fn_name CliThread_InvokeCommand = "CliThread.InvokeCommand";

word CliThread::InvokeCommand(const CliCommand& comm)
{
   Debug::ft(CliThread_InvokeCommand);

   //  Initialize the cookie so that it will look for the command's
   //  first parameter.
   //
   cookie_.Initialize();

   //  Execute the command and output the results.
   //
   Pause();
   command_ = &comm;
   SetResult(comm.ProcessCommand(*this));
   Flush();
   command_ = nullptr;
   return result_;
}

//------------------------------------------------------------------------------

fn_name CliThread_InvokeSubcommand = "CliThread.InvokeSubcommand";

word CliThread::InvokeSubcommand(const CliCommand& comm)
{
   Debug::ft(CliThread_InvokeSubcommand);

   auto prev = command_;
   command_ = &comm;
   SetResult(comm.ProcessCommand(*this));
   command_ = prev;
   return result_;
}

//------------------------------------------------------------------------------

fn_name CliThread_Notify = "CliThread.Notify";

void CliThread::Notify(CliAppData::Event event) const
{
   Debug::ft(CliThread_Notify);

   for(auto a = appsData_.cbegin(); a != appsData_.cend(); ++a)
   {
      a->second->EventOccurred(event);
   }
}

//------------------------------------------------------------------------------

fn_name CliThread_OpenInputFile = "CliThread.OpenInputFile";

word CliThread::OpenInputFile(const string& name, string& expl)
{
   Debug::ft(CliThread_OpenInputFile);

   if(inFiles_.size() >= MaxInputDepth)
   {
      expl = TooManyInputStreams;
      return -7;
   }

   auto path = Element::InputPath() + PATH_SEPARATOR + name + ".txt";
   auto file = SysFile::CreateIstream(path.c_str());

   if(file != nullptr)
   {
      inFiles_.push_back(std::move(file));
      return 0;
   }

   expl = NoFileExpl;
   return -2;
}

//------------------------------------------------------------------------------

fn_name CliThread_ParseCommand = "CliThread.ParseCommand";

const CliCommand* CliThread::ParseCommand() const
{
   Debug::ft(CliThread_ParseCommand);

   string token1;
   string token2;
   string tag;
   bool inIncr;

   //  Record the command in any output file.  (CliBuffer.GetLine copies
   //  each input to the console and/or the console transcript file.)
   //
   if(!outFiles_.empty())
   {
      auto input = ibuf->Echo();
      FileThread::Spool(outFiles_.back(), input, true);
   }

   //  Get the first token, which must be the name of a command or
   //  increment.
   //
   if(ibuf->GetStr(tag, token1) != CliParm::Ok) return nullptr;
   if(!tag.empty()) return nullptr;

   auto comm = stack_->FindCommand(token1);

   if(comm != nullptr)
   {
      //  <command>
      //
      return comm;
   }

   auto incr = stack_->FindIncrement(token1);

   if(incr != nullptr)
   {
      inIncr = true;
   }
   else
   {
      incr = Singleton< CliRegistry >::Instance()->FindIncrement(token1);

      if(incr == nullptr)
      {
         //  <junk>
         //
         *obuf << spaces(2) << NoCommandExpl << token1 << CRLF;
         return nullptr;
      }

      inIncr = false;
      stack_->Push(*incr);
   }

   if(ibuf->GetStr(tag, token2) != CliParm::Ok)
   {
      //  <increment>
      //
      if(inIncr) *obuf << AlreadyInIncrement << token1 << '.' << CRLF;
      return nullptr;
   }

   if(!tag.empty()) return nullptr;
   comm = incr->FindCommand(token2);

   if(comm == nullptr)
   {
      //  <increment> <junk>
      //
      *obuf << spaces(2) << NoCommandExpl << token1;
      *obuf << CliCommand::CommandSeparator << token2 << CRLF;
   }

   if(!inIncr) stack_->Pop();
   return comm;
}

//------------------------------------------------------------------------------

void CliThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliThread_PopOutputFile = "CliThread.PopOutputFile";

bool CliThread::PopOutputFile(bool all)
{
   Debug::ft(CliThread_PopOutputFile);

   if(outFiles_.empty()) return false;

   SendAckToOutputFile();

   while(!outFiles_.empty())
   {
      outFiles_.pop_back();
      if(!all) return true;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CliThread_PushOutputFile = "CliThread.PushOutputFile";

bool CliThread::PushOutputFile(const string& file)
{
   Debug::ft(CliThread_PushOutputFile);

   if(outFiles_.size() >= MaxOutputDepth) return false;

   SendAckToOutputFile();
   outFiles_.push_back(file);
   return true;
}

//------------------------------------------------------------------------------

fn_name CliThread_ReadCommands = "CliThread.ReadCommands";

void CliThread::ReadCommands()
{
   Debug::ft(CliThread_ReadCommands);

   while(true)
   {
      //  Print the CLI prompt, which contains the name of the
      //  most recently entered increment (if any).
      //
      string prompt(stack_->Top()->Name());
      prompt.push_back(CliPrompt);
      prompt_ = prompt;

      if(!skip_)
      {
         CoutThread::Spool(prompt_.c_str());

         if(!outFiles_.empty())
         {
            FileThread::Spool(outFiles_.back(), prompt_, false);
         }
      }

      skip_ = false;

      //  Read the user's input and parse it.
      //
      auto rc = ibuf->GetLine(*this);

      if(rc > 0)
      {
         auto comm = ParseCommand();
         if(comm != nullptr) InvokeCommand(*comm);
         Flush();
      }
      else
      {
         switch(rc)
         {
         case StreamEmpty:
            //
            //  Display the prompt again only if reading from the console.
            //
            skip_ = !inFiles_.empty();
            break;

         case StreamBadChar:
            //
            //  CliBuffer has displayed an error string.  Just loop around and
            //  prompt for new input.
            //
            break;

         case StreamEof:
         case StreamFailure:
            if(!inFiles_.empty())
            {
               //  End of input stream.  Delete the stream and resume input
               //  from the previous stream.
               //
               inFiles_.back().reset();
               inFiles_.pop_back();
               skip_ = true;
               return;
            }

            //  StreamEof and StreamFailure are not reported when reading from
            //  the console.  Pause before continuing.
            //
            Debug::SwLog(CliThread_ReadCommands, "invalid StreamRc", rc);
            //  [fallthrough]
         case StreamInterrupt:
         case StreamRestart:
            //
            //  o StreamInterrupt occurs when we plan to exit during a restart.
            //    Pausing causes us to receive SIGCLOSE and exit.
            //  o StreamRestart can occur if InitThread traps during a restart,
            //    after we have been created.  Pausing is also appropriate in
            //    this case, as another restart should occur momentarily.
            //
            Pause(ONE_SEC);
            break;

         case StreamInUse:
         default:
            Debug::SwLog(CliThread_ReadCommands, "unexpected StreamRc", rc);
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name CliThread_Recover = "CliThread.Recover";

bool CliThread::Recover()
{
   Debug::ft(CliThread_Recover);

   auto sig = GetSignal();
   auto reg = Singleton< PosixSignalRegistry >::Instance();

   if(reg->Attrs(sig).test(PosixSignal::Break))
   {
      //  On a break signal, remain in the current increment(s) but
      //  abort whatever work was in progress.
      //
      appsData_.clear();
      inFiles_.clear();
      outFiles_.clear();
      stream_.reset();
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CliThread_Report = "CliThread.Report";

word CliThread::Report(word rc, const string& expl, col_t indent) const
{
   Debug::ft(CliThread_Report);

   //  If EXPL contains explicit endlines, output each substring
   //  that ends with an endline individually.
   //
   size_t size = expl.size();
   size_t begin = 0;

   while(begin < size)
   {
      auto end = expl.find(CRLF, begin);

      if(end == string::npos)
      {
         Report1(expl, begin, size - 1, indent);
         return rc;
      }

      if(end == begin)
         *obuf << CRLF;
      else
         Report1(expl, begin, end - 1, indent);

      begin = end + 1;
   }

   return rc;
}

//------------------------------------------------------------------------------

fn_name CliThread_Report1 = "CliThread.Report1";

void CliThread::Report1
   (const string& expl, size_t begin, size_t end, col_t indent) const
{
   Debug::ft(CliThread_Report1);

   size_t maxlen = 79 - indent;  // maximum line length

   while(begin <= end)
   {
      //  If the rest of EXPL fits on one line, output it and return.
      //
      auto size = end - begin + 1;

      if(size <= maxlen)
      {
         *obuf << spaces(indent) << expl.substr(begin, size) << CRLF;
         return;
      }

      //  Starting at the last character that would fit on a line,
      //  work backwards to find a blank, and insert an endline at
      //  that point.  Then continue with the remaining characters.
      //
      auto stop = std::min(begin + maxlen - 1, end);
      auto blank = expl.rfind(SPACE, stop);

      if(blank != string::npos)
      {
         *obuf << spaces(indent) << expl.substr(begin, blank - begin) << CRLF;
         begin = blank + 1;
      }
      else
      {
         //  There are more characters without intervening blanks than
         //  fit on one line.  Just output the entire substring and let
         //  it wrap wherever.
         //
         *obuf << spaces(indent) << expl.substr(begin, size) << CRLF;
         return;
      }
   }
}

//------------------------------------------------------------------------------

fn_name CliThread_SendAckToOutputFile = "CliThread.SendAckToOutputFile";

void CliThread::SendAckToOutputFile()
{
   Debug::ft(CliThread_SendAckToOutputFile);

   std::ostringstream ack;

   ack << spaces(2) << SuccessExpl << CRLF;

   if(outFiles_.empty())
      CoutThread::Spool(ack.str().c_str());
   else
      FileThread::Spool(outFiles_.back(), ack.str());
}

//------------------------------------------------------------------------------

fn_name CliThread_SendToFile = "CliThread.SendToFile";

void CliThread::SendToFile(const string& name, bool purge)
{
   Debug::ft(CliThread_SendToFile);

   if(stream_->tellp() > 0)
   {
      FileThread::Spool(name, stream_, purge);
   }
}

//------------------------------------------------------------------------------

fn_name CliThread_SetAppData = "CliThread.SetAppData";

void CliThread::SetAppData(CliAppData* data, CliAppData::Id aid)
{
   Debug::ft(CliThread_SetAppData);

   if(data == nullptr)
      appsData_.erase(aid);
   else
      appsData_[aid].reset(data);
}

//------------------------------------------------------------------------------

fn_name CliThread_SetResult = "CliThread.SetResult";

void CliThread::SetResult(word result)
{
   Debug::ft(CliThread_SetResult);

   result_ = result;
   auto reg = Singleton< SymbolRegistry >::Instance();
   auto sym = reg->EnsureSymbol("cli.result");
   if(sym != nullptr) sym->SetValue(std::to_string(result_), false);
}

//------------------------------------------------------------------------------

fn_name CliThread_Shutdown = "CliThread.Shutdown";

void CliThread::Shutdown(RestartLevel level)
{
   Debug::ft(CliThread_Shutdown);

   //  Nullify the resources whose heap will be deleted during a restart.
   //
   Restart::Release(ibuf);
   Restart::Release(stack_);

   for(auto a = appsData_.begin(); a != appsData_.end(); ++a)
   {
      Restart::Release(a->second);
   }

   Thread::Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name CliThread_Startup = "CliThread.Startup";

void CliThread::Startup(RestartLevel level)
{
   Debug::ft(CliThread_Startup);

   Thread::Startup(level);
   AllocResources();
}

//------------------------------------------------------------------------------

fn_name CliThread_StrPrompt = "CliThread.StrPrompt";

string CliThread::StrPrompt(const string& prompt)
{
   Debug::ft(CliThread_StrPrompt);

   //  If input is being taken from a file rather than the console,
   //  return an empty string.
   //
   if(!inFiles_.empty()) return EMPTY_STR;

   auto first = true;
   string text;

   //  Output the query, read the user's input, and echo it to the
   //  console transcript file before returning it.
   //
   while(true)
   {
      ostringstreamPtr stream(new std::ostringstream);

      if(first)
         *stream << prompt;
      else
         *stream << "Please enter a non-empty string";

      Flush();
      CoutThread::Spool(stream);
      first = false;

      auto count = CinThread::GetLine(text);
      if(count < 0) return EMPTY_STR;

      if(count > 0)
      {
         string input(text);
         FileThread::Record(input, true);
         return input;
      }
   }
}
}
