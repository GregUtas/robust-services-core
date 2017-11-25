//==============================================================================
//
//  CliThread.h
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
#ifndef CLITHREAD_H_INCLUDED
#define CLITHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "CliAppData.h"
#include "CliCookie.h"
#include "NbTypes.h"
#include "SysFile.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliBuffer;
   class CliCommand;
   class CliStack;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Implements the CLI by reading commands and invoking the appropriate
//  increment.
//
//  NOTE: The CLI thread runs unpreemptably.  CLI applications must invoke
//  ====  SetPreemptable before performing time-consuming operations and
//        invoke SetUnpreemptable once finished.  All CLI output is first
//        written to OBUF before being forwarded to the console.
//
class CliThread : public Thread
{
   friend class Singleton< CliThread >;
   friend class HelpCommand;
   friend class QuitCommand;
   friend class SendCommand;
public:
   //  Called once a command has been parsed.  Returns true if the input
   //  stream contains no more non-blank characters.  Otherwise, an error
   //  or warning message (as determined by ERROR) is output, indicating
   //  where the superfluous input starts.
   //
   bool EndOfInput(bool error);

   //  Used to report the result of a command that returned RC.  EXPL is
   //  displayed as a success or failure explanation with INDENT leading
   //  blanks.  Returns RC.
   //
   word Report(word rc, const std::string& expl, col_t indent = 2);

   //  Outputs and clears the output buffer (obuf) if it contains text.
   //
   void Flush();

   //  Displays PROMPT and loops until the user enters 'y' or 'n'.  Returns
   //  true or false accordingly.  Returns true if commands are being read
   //  from a file rather than the console.
   //
   bool BoolPrompt(const std::string& prompt);

   //  Displays PROMPT, followed by CHARS, and loops until the user enters a
   //  character in CHARS, which is returned.  Converts upper to lower case
   //  unless UPPER is set.  Returns the first character in CHARS if commands
   //  are being read from a file rather than the console.  Returns '\0' on
   //  an error, such as CHARS being empty.  Displays HELP if the user enters
   //  invalid input.
   //
   char CharPrompt(const std::string& prompt,
      const std::string& chars, const std::string& help, bool upper = false);

   //  Returns the buffer where output to be passed to FileThread is placed.
   //
   std::ostream* FileStream();

   //  After output has been placed in the buffer returned by FileStream
   //  (above), invoke this to send the buffer to FileThread, which will
   //  write the output to a file identified by NAME.  If PURGE is set,
   //  an existing file with NAME is overwritten; otherwise, the output
   //  is appended to it.
   //
   void SendToFile(const std::string& name, bool purge = false);

   //  Returns the current command.
   //
   const CliCommand* Command() const { return command_; }

   //  Returns the result from the last command executed.  This value is
   //  also saved in the symbol &cli.result.
   //
   word Result() const { return result_; }

   //  Executes INPUT as if it had been entered on the command line.
   //
   word Execute(const std::string& input);

   //  Accesses application-specific data.
   //
   CliAppData* GetAppData(CliAppData::Id aid) const;

   //  Sets application-specific data.  Any existing data is first deleted.
   //
   void SetAppData(CliAppData* data, CliAppData::Id aid);

   //  Notifies applications that EVT has occurred.
   //
   void Notify(CliAppData::Event evt) const;

   //  Invoked when tracing is still on and trace tool is about to generate
   //  a report.  Reports are normally generated preemptably, but in the lab
   //  the user is given the option to trace generation of the report itself.
   //
   bool GenerateReportPreemptably();

   //  Returns the result of invoking comm.ProcessCommand.
   //
   word InvokeSubcommand(const CliCommand& comm);

   //  Returns the current prompt.
   //
   const std::string& Prompt() const { return *prompt_; }

   //  Returns the file from which input is being read.
   //
   std::istream* InputFile() const { return in_[inIndex_].get(); }

   //  Returns the parse cookie.
   //
   CliCookie& Cookie() { return cookie_; }

   //  Opens NAME.txt for reading input.  Returns 0 on success.  On failure,
   //  returns another value and updates EXPL with an explanation.
   //
   word OpenInputFile(const std::string& name, std::string& expl);

   //  Reads commands from the current input stream (in_[inIndex_).
   //  This is initially the console, but the >read command calls
   //  this to read commands from a file.
   //
   void ReadCommands();

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  The input buffer.
   //
   std::unique_ptr< CliBuffer > ibuf;

   //  The output buffer where each CliCommand writes its results.
   //
   ostringstreamPtr obuf;
private:
   //  Private because this singleton is not subclassed.
   //
   CliThread();

   //  Private because this singleton is not subclassed.
   //
   ~CliThread();

   //  Parses user input and returns the command to be processed.
   //  Returns nullptr if no command is to be invoked.
   //
   const CliCommand* ParseCommand();

   //  Initializes the parser, invokes COMM, streams its output,
   //  and returns its result.
   //
   word InvokeCommand(const CliCommand& comm);

   //  Sets the result of executing a command.
   //
   void SetResult(word result);

   //  Acquires resources when creating or recreating the thread.
   //
   void AllocResources();

   //  Releases resources when deleting or recreating the thread.
   //
   void ReleaseResources();

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to read commands from the console, invoke them, and
   //  display the results.
   //
   virtual void Enter() override;

   //  Overridden to clean up when the thread is recreated.
   //
   virtual void Recreated() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;

   //  Overridden to release resources during error recovery.
   //
   virtual void Cleanup() override;

   //> The default prompt for user input.
   //
   static const char CliPrompt;

   //  The stack of active increments.
   //
   std::unique_ptr< CliStack > stack_;

   //  The current prompt for user input.
   //
   stringPtr prompt_;

   //  Set to suppress the prompt on a one-time basis.
   //
   bool skip_;

   //  The command currently being executed.
   //
   const CliCommand* command_;

   //  The current location in the parse tree.
   //
   CliCookie cookie_;

   //  The value returned by the last command executed.
   //
   word result_;

   //  A buffer where output to be passed to FileThread is placed.
   //
   ostringstreamPtr stream_;

   //> The number of input streams that can be nested by the >send command.
   //
   static const size_t OutSize = 8;

   //  Where to send output (the default is the console transcript file).
   //
   stringPtr outName_[OutSize];

   //  The index into out_ that references the output stream (if zero, then
   //  output is sent to CoutThread and copied to a console transcript file).
   //
   size_t outIndex_;

   //> The number of input streams that can be nested by the >read command.
   //
   static const size_t InSize = 8;

   //  The stream from which input is being read (in_[0] is nullptr).
   //
   istreamPtr in_[InSize];

   //  The index into in_ that references the input stream (if zero,
   //  input is taken from CinThread).
   //
   size_t inIndex_;

   //  Application-specific data.
   //
   std::unique_ptr< CliAppData > appData_[CliAppData::MaxId + 1];
};
}
#endif
