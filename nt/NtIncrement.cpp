//==============================================================================
//
//  NtIncrement.cpp
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
#include "NtIncrement.h"
#include "CliCommandSet.h"
#include "CliText.h"
#include "Daemon.h"
#include "NbHeap.h"
#include "Temporary.h"
#include "Thread.h"
#include <cctype>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <istream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include "Algorithms.h"
#include "CliBoolParm.h"
#include "CliPtrParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "FunctionProfiler.h"
#include "FunctionTrace.h"
#include "LeakyBucketCounter.h"
#include "NbAppIds.h"
#include "NbCliParms.h"
#include "NbSignals.h"
#include "NtTestData.h"
#include "ObjectPool.h"
#include "ObjectPoolRegistry.h"
#include "PosixSignal.h"
#include "PosixSignalRegistry.h"
#include "Q1Link.h"
#include "Q1Way.h"
#include "Q2Link.h"
#include "Q2Way.h"
#include "RegCell.h"
#include "Registry.h"
#include "Singleton.h"
#include "SysFile.h"
#include "SysMutex.h"
#include "SysTime.h"
#include "TestDatabase.h"
#include "ToolTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  The CORRUPT command.
//
class ObjectPoolText : public CliText
{
public: ObjectPoolText();
};

fixed_string FreeqOffsetExpl = "offset into free queue (0 = head)";

fixed_string ObjectPoolTextStr = "pool";
fixed_string ObjectPoolTextExpl = "object pool";

ObjectPoolText::ObjectPoolText() :
   CliText(ObjectPoolTextExpl, ObjectPoolTextStr)
{
   BindParm(*new ObjPoolIdMandParm);
   BindParm(*new CliIntParm(FreeqOffsetExpl, 0, 1024));
}

fixed_string CorruptWhatExpl = "what to corrupt...";

CorruptWhatParm::CorruptWhatParm() : CliTextParm(CorruptWhatExpl)
{
   BindText(*new ObjectPoolText, CorruptCommand::PoolIndex);
}

fixed_string CorruptStr = "corrupt";
fixed_string CorruptExpl = "Corrupts a data structure for testing purposes.";

CorruptCommand::CorruptCommand(bool bind) :
   CliCommand(CorruptStr, CorruptExpl)
{
   if(bind) BindParm(*new CorruptWhatParm);
}

word CorruptCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CorruptCommand.ProcessCommand");

   id_t corruptWhatIndex;

   if(!Element::RunningInLab()) return cli.Report(-5, NotInFieldExpl);
   if(!GetTextIndex(corruptWhatIndex, cli)) return -1;
   return ProcessSubcommand(cli, corruptWhatIndex);
}

word CorruptCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("CorruptCommand.ProcessSubcommand");

   if(index != PoolIndex) return CliCommand::ProcessSubcommand(cli, index);

   word pid, n;

   if(!GetIntParm(pid, cli)) return -1;
   if(!GetIntParm(n, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< ObjectPoolRegistry >::Instance()->Pool(pid);
   if(pool == nullptr) return cli.Report(-2, NoPoolExpl);
   if(!pool->Corrupt(n)) return cli.Report(-3, EndOfFreeQueue);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The LOGS command.
//
class LogsSortText : public CliText
{
public: LogsSortText();
};

fixed_string LogsSortTextStr = "sort";
fixed_string LogsSortTextExpl = "sorts the logs in a log file";

LogsSortText::LogsSortText() : CliText(LogsSortTextExpl, LogsSortTextStr)
{
   BindParm(*new IstreamMandParm);
   BindParm(*new OstreamMandParm);
}

fixed_string FloodCountExpl = "number of SW900 logs to generate";

class LogsFloodText : public CliText
{
public: LogsFloodText();
};

fixed_string LogsFloodTextStr = "flood";
fixed_string LogsFloodTextExpl = "enters a loop that generates SW900 logs";

LogsFloodText::LogsFloodText() : CliText(LogsFloodTextExpl, LogsFloodTextStr)
{
   BindParm(*new CliIntParm(FloodCountExpl, 1, 250));
}

class NtLogsAction : public LogsAction
{
public:
   NtLogsAction();
   virtual ~NtLogsAction() = default;
};

class NtLogsCommand : public LogsCommand
{
public:
   static const id_t SortIndex = LastNbIndex + 1;
   static const id_t FloodIndex = LastNbIndex + 2;
   static const id_t LastNtIndex = FloodIndex;

   //  Set BIND to false if binding a subclass of NtLogsAction.
   //
   explicit NtLogsCommand(bool bind = true);
   virtual ~NtLogsCommand() = default;
protected:
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
private:
   static word Sort(const string& input, const string& output, string& expl);
};

NtLogsAction::NtLogsAction()
{
   BindText(*new LogsSortText, NtLogsCommand::SortIndex);
   BindText(*new LogsFloodText, NtLogsCommand::FloodIndex);
}

NtLogsCommand::NtLogsCommand(bool bind) : LogsCommand(false)
{
   if(bind) BindParm(*new NtLogsAction);
}

fn_name NtLogsCommand_ProcessSubcommand = "NtLogsCommand.ProcessSubcommand";

word NtLogsCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft(NtLogsCommand_ProcessSubcommand);

   if((index <= LastNbIndex) || (index > LastNtIndex))
   {
      return LogsCommand::ProcessSubcommand(cli, index);
   }

   string input, output, expl;
   word count;
   word rc = 0;

   switch(index)
   {
   case SortIndex:
   {
      auto yield = cli.GenerateReportPreemptably();
      FunctionGuard guard(Guard_MakePreemptable, yield);

      if(!GetFileName(input, cli)) return -1;
      if(!GetFileName(output, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = Sort(input, output, expl);
      return cli.Report(rc, expl);
   }

   case FloodIndex:
      if(!GetIntParm(count, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      while(count-- > 0)
      {
         Debug::SwLog
            (NtLogsCommand_ProcessSubcommand, "log flood test", count + 1);
      }
      break;
   }

   return rc;
}

word NtLogsCommand::Sort
   (const string& input, const string& output, string& expl)
{
   Debug::ft("NtLogsCommand.Sort");

   FunctionGuard guard(Guard_MakePreemptable);

   //  Each log is saved as a single string with embedded CRLFs.  The log's
   //  sequence number, which appears at the end of the first line enclosed
   //  in braces, serves to sort the logs as they are inserted into the map.
   //  The first line of each log is found by looking for the string
   //  "on <element-name>", which is immediately followed by the sequence
   //  number.  A blank line follows each log (except for the last one) and
   //  causes the accumulated log to be saved, as long as NUM (the sequence
   //  number) is non-zero.  If NUM is zero, it means that a log has yet to
   //  be found.
   //
   std::map< size_t, string > logs;
   string line, log;
   size_t num = 0;
   string location = "on " + Element::Name();
   auto dir = Element::OutputPath();
   auto path = dir + PATH_SEPARATOR + input + ".txt";
   auto infile = SysFile::CreateIstream(path.c_str());
   if(infile == nullptr)
   {
      expl = "Could not open input file: " + path;
      return -2;
   }

   while(infile->peek() != EOF)
   {
      std::getline(*infile, line);

      if(line.empty())
      {
         if(!log.empty() && (num != 0))
         {
            logs.insert(std::pair< size_t, string >(num, log));
         }

         num = 0;
         log.clear();
      }
      else
      {
         log += line + CRLF;
         auto pos = line.find(location);

         if(pos != string::npos)
         {
            pos = line.find_first_of('{', pos);

            if(pos != string::npos)
            {
               num = 0;
               for(auto c = line[++pos]; isdigit(c); c = line[++pos])
               {
                  num = (10 * num) + (c - '0');
               }
            }
         }
      }
   }

   if(!log.empty() && (num != 0))
   {
      logs.insert(std::pair< size_t, string >(num, log));
   }

   infile.reset();
   path = dir + PATH_SEPARATOR + output + ".txt";
   auto outfile = SysFile::CreateOstream(path.c_str(), true);
   if(outfile == nullptr)
   {
      expl = "Could not open output file: " + path;
      return -7;
   }

   for(auto i = logs.cbegin(); i != logs.cend(); ++i)
   {
      *outfile << i->second << CRLF;
   }

   outfile.reset();
   expl = SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------
//
//  The SAVE command.
//
class FuncsSortHowParm : public CliTextParm
{
public: FuncsSortHowParm();
};

class FuncsText : public CliText
{
public: FuncsText();
};

fixed_string FuncsSortByCallsTextStr = "calls";
fixed_string FuncsSortByCallsTextExpl = "by number of invocations";

fixed_string FuncsSortByTimesTextStr = "times";
fixed_string FuncsSortByTimesTextExpl = "by net time in function";

fixed_string FuncsSortByNamesTextStr = "names";
fixed_string FuncsSortByNamesTextExpl = "by function name";

fixed_string FuncsSortHowExpl = "how to sort (default=calls)";

constexpr id_t SortByCallsIndex = 1;
constexpr id_t SortByTimesIndex = 2;
constexpr id_t SortByNamesIndex = 3;

FuncsSortHowParm::FuncsSortHowParm() : CliTextParm(FuncsSortHowExpl, true)
{
   BindText(*new CliText
      (FuncsSortByCallsTextExpl, FuncsSortByCallsTextStr), SortByCallsIndex);
   BindText(*new CliText
      (FuncsSortByTimesTextExpl, FuncsSortByTimesTextStr), SortByTimesIndex);
   BindText(*new CliText
      (FuncsSortByNamesTextExpl, FuncsSortByNamesTextStr), SortByNamesIndex);
}

fixed_string FuncsTextStr = "funcs";
fixed_string FuncsTextExpl = "function call statistics";

FuncsText::FuncsText() : CliText(FuncsTextExpl, FuncsTextStr)
{
   BindParm(*new OstreamMandParm);
   BindParm(*new FuncsSortHowParm);
}

NtSaveWhatParm::NtSaveWhatParm()
{
   BindText(*new FuncsText, NtSaveCommand::FuncsIndex);
}

NtSaveCommand::NtSaveCommand(bool bind) : SaveCommand(false)
{
   if(bind) BindParm(*new NtSaveWhatParm);
}

word NtSaveCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("NtSaveCommand.ProcessSubcommand");

   if(index != FuncsIndex) return SaveCommand::ProcessSubcommand(cli, index);

   TraceRc rc;
   string title;
   id_t sortHowIndex;
   auto sort = FunctionProfiler::ByCalls;

   auto yield = cli.GenerateReportPreemptably();
   FunctionGuard guard(Guard_MakePreemptable, yield);

   if(!GetFileName(title, cli)) return -1;
   if(GetTextIndexRc(sortHowIndex, cli) == Ok)
   {
      switch(sortHowIndex)
      {
      case SortByCallsIndex: sort = FunctionProfiler::ByCalls; break;
      case SortByTimesIndex: sort = FunctionProfiler::ByTimes; break;
      case SortByNamesIndex: sort = FunctionProfiler::ByNames; break;
      }
   }
   if(!cli.EndOfInput()) return -1;

   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   FunctionTrace::Process(EMPTY_STR);
   std::unique_ptr< FunctionProfiler > fp(new FunctionProfiler);
   rc = fp->Generate(*stream, sort);
   fp.reset();

   if(rc == TraceOk)
   {
      title += ".funcs.txt";
      cli.SendToFile(title, true);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The SET command.
//
class FuncScopeParm : public CliTextParm
{
public: FuncScopeParm();
};

class ScopeText : public CliText
{
public: ScopeText();
};

fixed_string FuncScopeFullTraceTextStr = "full";
fixed_string FuncScopeFullTraceTextExpl = "full trace of invocations";

fixed_string FuncScopeCountsOnlyTextStr = "counts";
fixed_string FuncScopeCountsOnlyTextExpl = "count invocations per function";

fixed_string FuncScopeExpl = "how to trace function invocations";

constexpr id_t FuncScopeFullTraceIndex = 1;
constexpr id_t FuncScopeCountsOnlyIndex = 2;

FuncScopeParm::FuncScopeParm() : CliTextParm(FuncScopeExpl)
{
   BindText(*new CliText(FuncScopeFullTraceTextExpl,
      FuncScopeFullTraceTextStr), FuncScopeFullTraceIndex);
   BindText(*new CliText(FuncScopeCountsOnlyTextExpl,
      FuncScopeCountsOnlyTextStr), FuncScopeCountsOnlyIndex);
}

fixed_string ScopeTextStr = "scope";
fixed_string ScopeTextExpl = "scope for function tracing";

ScopeText::ScopeText() : CliText(ScopeTextExpl, ScopeTextStr)
{
   BindParm(*new FuncScopeParm);
}

NtSetWhatParm::NtSetWhatParm()
{
   BindText(*new ScopeText, NtSetCommand::FuncTraceScope);
}

NtSetCommand::NtSetCommand(bool bind) : SetCommand(false)
{
   if(bind) BindParm(*new NtSetWhatParm);
}

word NtSetCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("NtSetCommand.ProcessSubcommand");

   if(index != FuncTraceScope) return SetCommand::ProcessSubcommand(cli, index);

   auto rc = TraceOk;
   id_t scope;

   if(!GetTextIndex(scope, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   switch(scope)
   {
   case FuncScopeFullTraceIndex:
      rc = FunctionTrace::SetScope(FunctionTrace::FullTrace);
      break;
   case FuncScopeCountsOnlyIndex:
      rc = FunctionTrace::SetScope(FunctionTrace::CountsOnly);
      break;
   default:
      return cli.Report(scope, SystemErrorExpl);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The SWFLAGS command.
//
class FlagsSetText : public CliText
{
public: FlagsSetText();
};

class FlagsAction : public CliTextParm
{
public: FlagsAction();
};

class SwFlagsCommand : public CliCommand
{
public:
   static const id_t FlagsSetIndex = 1;
   static const id_t FlagsClearIndex = 2;
   static const id_t FlagsQueryIndex = 3;

   SwFlagsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FlagIdExpl = "flag identifier";

fixed_string FlagsSetTextStr = "set";
fixed_string FlagsSetTextExpl = "modifies a flag's setting";

FlagsSetText::FlagsSetText() : CliText(FlagsSetTextExpl, FlagsSetTextStr)
{
   BindParm(*new CliIntParm(FlagIdExpl, 0, MaxFlagId));
   BindParm(*new SetHowParm);
}

fixed_string FlagsClearTextStr = "clear";
fixed_string FlagsClearTextExpl = "clears all flags";

fixed_string FlagsQueryTextStr = "query";
fixed_string FlagsQueryTextExpl = "displays flags that are on";

fixed_string FlagsActionExpl = "subcommand...";

FlagsAction::FlagsAction() : CliTextParm(FlagsActionExpl)
{
   BindText(*new FlagsSetText, SwFlagsCommand::FlagsSetIndex);
   BindText(*new CliText
      (FlagsClearTextExpl, FlagsClearTextStr), SwFlagsCommand::FlagsClearIndex);
   BindText(*new CliText
      (FlagsQueryTextExpl, FlagsQueryTextStr), SwFlagsCommand::FlagsQueryIndex);
}

fixed_string SwFlagsStr = "swflags";
fixed_string SwFlagsExpl = "Supports flags used to control branching";

SwFlagsCommand::SwFlagsCommand() : CliCommand(SwFlagsStr, SwFlagsExpl)
{
   BindParm(*new FlagsAction);
}

word SwFlagsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SwFlagsCommand.ProcessCommand");

   id_t index, setHowIndex;
   word flag;
   Flags flags;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case FlagsSetIndex:
      if(!GetIntParm(flag, cli)) return -1;
      if(!GetTextIndex(setHowIndex, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      Debug::SetSwFlag(flag, (setHowIndex == SetHowParm::On));
      break;

   case FlagsClearIndex:
      if(!cli.EndOfInput()) return -1;
      Debug::ResetSwFlags();
      break;

   case FlagsQueryIndex:
      if(!cli.EndOfInput()) return -1;
      flags = Debug::GetSwFlags();
      *cli.obuf << spaces(2) << "Flags on (bit offsets):";

      if(flags.none())
      {
         *cli.obuf << " none";
      }
      else
      {
         for(auto i = 0; i <= MaxFlagId; ++i)
         {
            if(flags.test(i)) *cli.obuf << SPACE << i;
         }
      }

      *cli.obuf << CRLF;
      return 0;

   default:
      return cli.Report(index, SystemErrorExpl);
   }

   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The TESTS command.
//
class TestPrologText : public CliText
{
public: TestPrologText();
};

class TestEpilogText : public CliText
{
public: TestEpilogText();
};

class TestRecoverText : public CliText
{
public: TestRecoverText();
};

class TestBeginText : public CliText
{
public: TestBeginText();
};

class TestFailedText : public CliText
{
public: TestFailedText();
};

class TestQueryText : public CliText
{
public: TestQueryText();
};

class TestEraseText : public CliText
{
public: TestEraseText();
};

fixed_string TestPrologExpl = "filename (none if omitted)";

fixed_string TestPrologTextStr = "prolog";
fixed_string TestPrologTextExpl = "file to read before executing a test";

TestPrologText::TestPrologText() :
   CliText(TestPrologTextExpl, TestPrologTextStr)
{
   BindParm(*new CliTextParm(TestPrologExpl, true, 0));
}

fixed_string TestEpilogExpl = "filename (none if omitted)";

fixed_string TestEpilogTextStr = "epilog";
fixed_string TestEpilogTextExpl = "file to read after a test passes";

TestEpilogText::TestEpilogText() :
   CliText(TestEpilogTextExpl, TestEpilogTextStr)
{
   BindParm(*new CliTextParm(TestEpilogExpl, true, 0));
}

fixed_string TestRecoverExpl = "filename (epilog if omitted)";

fixed_string TestRecoverTextStr = "recover";
fixed_string TestRecoverTextExpl = "file to read after a test fails";

TestRecoverText::TestRecoverText() :
   CliText(TestRecoverTextExpl, TestRecoverTextStr)
{
   BindParm(*new CliTextParm(TestRecoverExpl, true, 0));
}

fixed_string TestBeginExpl = "test filename";

fixed_string TestBeginTextStr = "begin";
fixed_string TestBeginTextExpl =
   "executes a test (and concludes any previous one)";

TestBeginText::TestBeginText() : CliText(TestBeginTextExpl, TestBeginTextStr)
{
   BindParm(*new CliTextParm(TestBeginExpl, false, 0));
}

fixed_string TestEndTextStr = "end";
fixed_string TestEndTextExpl = "concludes a test";

fixed_string TestFailCodeExpl = "failure code";

fixed_string TestFailExpl = "explanation for failure";

fixed_string TestFailedTextStr = "failed";
fixed_string TestFailedTextExpl = "records that the current test failed";

TestFailedText::TestFailedText() :
   CliText(TestFailedTextExpl, TestFailedTextStr)
{
   BindParm(*new CliIntParm(TestFailCodeExpl, WORD_MIN, WORD_MAX));
   BindParm(*new CliTextParm(TestFailExpl, true, 0));
}

fixed_string TestRetestTextStr = "retest";
fixed_string TestRetestTextExpl = "displays tests that have not passed";

fixed_string TestQueryTextStr = "query";
fixed_string TestQueryTextExpl =
   "displays pass/fail counts and (if verbose) all tests";

TestQueryText::TestQueryText() :
   CliText(TestQueryTextExpl, TestQueryTextStr)
{
   BindParm(*new DispBVParm);
}

fixed_string TestEraseExpl = "test name";

fixed_string TestEraseTextStr = "erase";
fixed_string TestEraseTextExpl = "removes a test from the database";

TestEraseText::TestEraseText() : CliText(TestEraseTextExpl, TestEraseTextStr)
{
   BindParm(*new CliTextParm(TestEraseExpl, false, 0));
}

fixed_string TestResetTextStr = "reset";
fixed_string TestResetTextExpl = "resets the testing environment";

fixed_string TestsActionExpl = "subcommand...";

TestsAction::TestsAction() : CliTextParm(TestsActionExpl)
{
   BindText(*new TestPrologText, TestsCommand::TestPrologIndex);
   BindText(*new TestEpilogText, TestsCommand::TestEpilogIndex);
   BindText(*new TestRecoverText, TestsCommand::TestRecoverIndex);
   BindText(*new TestBeginText, TestsCommand::TestBeginIndex);
   BindText(*new CliText
      (TestEndTextExpl, TestEndTextStr), TestsCommand::TestEndIndex);
   BindText(*new TestFailedText, TestsCommand::TestFailedIndex);
   BindText(*new TestQueryText, TestsCommand::TestQueryIndex);
   BindText(*new CliText
      (TestRetestTextExpl, TestRetestTextStr), TestsCommand::TestRetestIndex);
   BindText(*new TestEraseText, TestsCommand::TestEraseIndex);
   BindText(*new CliText
      (TestResetTextExpl, TestResetTextStr), TestsCommand::TestResetIndex);
}

fixed_string TestsStr = "tests";
fixed_string TestsExpl = "Configures or executes tests.";

TestsCommand::TestsCommand(bool bind) : CliCommand(TestsStr, TestsExpl)
{
   if(bind) BindParm(*new TestsAction);
}

word TestsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TestsCommand.ProcessCommand");

   id_t index;

   if(!GetTextIndex(index, cli)) return -1;

   return ProcessSubcommand(cli, index);
}

word TestsCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("TestsCommand.ProcessSubcommand");

   auto test = NtTestData::Access(cli);
   if(test == nullptr) return cli.Report(-7, AllocationError);

   word rc;
   string text, expl;
   auto v = false;

   switch(index)
   {
   case TestPrologIndex:
      if(!GetString(text, cli)) text.clear();
      if(!cli.EndOfInput()) return -1;
      test->SetProlog(text);
      break;

   case TestEpilogIndex:
      if(!GetString(text, cli)) text.clear();
      if(!cli.EndOfInput()) return -1;
      test->SetEpilog(text);
      break;

   case TestRecoverIndex:
      if(!GetString(text, cli)) text.clear();
      if(!cli.EndOfInput()) return -1;
      test->SetRecover(text);
      break;

   case TestBeginIndex:
      if(!GetString(text, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      return test->Initiate(text);

   case TestEndIndex:
      if(!cli.EndOfInput()) return -1;
      test->Conclude();
      return 0;

   case TestFailedIndex:
      if(!GetIntParm(rc, cli)) return -1;
      if(!GetString(text, cli)) text.clear();
      if(!cli.EndOfInput()) return -1;
      return test->SetFailed(rc, text);

   case TestQueryIndex:
      if(GetBV(*this, cli, v) == Error) return -1;
      if(!cli.EndOfInput()) return -1;
      test->Query(v, expl);
      return cli.Report(0, expl);

   case TestRetestIndex:
      if(!cli.EndOfInput()) return -1;
      rc = Singleton< TestDatabase >::Instance()->Retest(expl);
      return cli.Report(rc, expl);

   case TestEraseIndex:
      if(!GetString(text, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = Singleton< TestDatabase >::Instance()->Erase(text, expl);
      return cli.Report(rc, expl);

   case TestResetIndex:
      if(!cli.EndOfInput()) return -1;
      test->Reset();
      break;

   default:
      return CliCommand::ProcessSubcommand(cli, index);
   }

   return cli.Report(0, SuccessExpl);
}

//==============================================================================
//
//  Testing for NbHeap.
//
class TestHeap : public NbHeap
{
   friend class Singleton< TestHeap >;
public:
   static void SetSize(size_t size) { Size_ = size; }
   static void SetType(MemoryType type) { Type_ = type; }
private:
   TestHeap();
   ~TestHeap() = default;

   static MemoryType Type_;
   static size_t Size_;
};

MemoryType TestHeap::Type_ = MemTemporary;
size_t TestHeap::Size_ = 1 * kBs;

TestHeap::TestHeap() : NbHeap(Type_, Size_) { }

//------------------------------------------------------------------------------

fixed_string HeapSizeExpl = "heap's size";

//------------------------------------------------------------------------------

fixed_string HeapTypeExpl = "heap's memory type (temporary|dynamic|protected)";
fixed_string HeapTypeChars = "tdp";

//------------------------------------------------------------------------------

fixed_string HeapBlockAddrExpl = "block's address";

//------------------------------------------------------------------------------

fixed_string HeapBlockSizeExpl = "block's size";

//------------------------------------------------------------------------------

class HeapCreateCommand : public CliCommand
{
public:
   HeapCreateCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class HeapDestroyCommand : public CliCommand
{
public:
   HeapDestroyCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class HeapAllocCommand : public CliCommand
{
public:
   HeapAllocCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class HeapBlockToSizeCommand : public CliCommand
{
public:
   HeapBlockToSizeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class HeapDisplayCommand : public CliCommand
{
public:
   HeapDisplayCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class HeapFreeCommand : public CliCommand
{
public:
   HeapFreeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class HeapValidateCommand : public CliCommand
{
public:
   HeapValidateCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

class HeapCommands : public CliCommandSet
{
public:
   HeapCommands();
};

fixed_string HeapStr = "heap";
fixed_string HeapExpl = "Tests an NbHeap function.";

HeapCommands::HeapCommands() : CliCommandSet(HeapStr, HeapExpl)
{
   BindCommand(*new HeapCreateCommand);
   BindCommand(*new HeapDestroyCommand);
   BindCommand(*new HeapAllocCommand);
   BindCommand(*new HeapBlockToSizeCommand);
   BindCommand(*new HeapDisplayCommand);
   BindCommand(*new HeapFreeCommand);
   BindCommand(*new HeapValidateCommand);
}

//------------------------------------------------------------------------------

static word CheckHeap(bool shouldExist, const CliThread& cli, Heap*& heap)
{
   heap = Singleton< TestHeap >::Extant();

   if(heap == nullptr)
   {
      if(shouldExist)
      {
         *cli.obuf << spaces(2) << "The heap must first be created." << CRLF;
         return -1;
      }
   }
   else
   {
      if(!shouldExist)
      {
         *cli.obuf << spaces(2) << "The heap must first be destroyed." << CRLF;
         return -1;
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

fixed_string HeapCreateStr = "create";
fixed_string HeapCreateExpl = "Creates the heap.";

HeapCreateCommand::HeapCreateCommand() :
   CliCommand(HeapCreateStr, HeapCreateExpl)
{
   BindParm(*new CliCharParm(HeapTypeExpl, HeapTypeChars));
   BindParm(*new CliIntParm(HeapSizeExpl, 0, 2048));
}

word HeapCreateCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HeapCreateCommand.ProcessCommand");

   char c;
   word size;
   MemoryType type;

   if(!GetCharParm(c, cli)) return -1;
   if(!GetIntParm(size, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   switch(c)
   {
   case 't':
      type = MemTemporary;
      break;
   case 'd':
      type = MemDynamic;
      break;
   case 'p':
      type = MemProtected;
      break;
   default:
      return cli.Report(c, SystemErrorExpl);
   }

   Heap* heap = nullptr;
   auto rc = CheckHeap(false, cli, heap);
   if(rc != 0) return rc;

   TestHeap::SetType(type);
   TestHeap::SetSize(size);
   heap = Singleton< TestHeap >::Instance();
   *cli.obuf << "  Heap: " << heap << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string HeapDestroyStr = "destroy";
fixed_string HeapDestroyExpl = "Destroys the heap.";

HeapDestroyCommand::HeapDestroyCommand() :
   CliCommand(HeapDestroyStr, HeapDestroyExpl) { }

word HeapDestroyCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HeapDestroyCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;

   Heap* heap = nullptr;
   auto rc = CheckHeap(true, cli, heap);
   if(rc != 0) return rc;

   Singleton< TestHeap >::Destroy();
   heap = Singleton< TestHeap >::Extant();
   *cli.obuf << "  Heap: " << heap << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string HeapAllocStr = "alloc";
fixed_string HeapAllocExpl = "Allocates a block.";

HeapAllocCommand::HeapAllocCommand() :
   CliCommand(HeapAllocStr, HeapAllocExpl)
{
   BindParm(*new CliIntParm(HeapBlockSizeExpl, 0, 1 * kBs));
}

word HeapAllocCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HeapAllocCommand.ProcessCommand");

   word size;

   if(!GetIntParm(size, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   Heap* heap = nullptr;
   auto rc = CheckHeap(true, cli, heap);
   if(rc != 0) return rc;

   auto addr = heap->Alloc(size);
   *cli.obuf << "  Address: " << addr << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string HeapBlockToSizeStr = "blocksize";
fixed_string HeapBlockToSizeExpl = "Returns a block's size.";

HeapBlockToSizeCommand::HeapBlockToSizeCommand() :
   CliCommand(HeapBlockToSizeStr, HeapBlockToSizeExpl)
{
   BindParm(*new CliPtrParm(HeapBlockAddrExpl));
}

word HeapBlockToSizeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HeapBlockToSizeCommand.ProcessCommand");

   void* addr;

   if(!GetPtrParm(addr, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   Heap* heap = nullptr;
   auto rc = CheckHeap(true, cli, heap);
   if(rc != 0) return rc;

   auto size = heap->BlockToSize(addr);
   *cli.obuf << "  Size: " << size << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string HeapDisplayStr = "display";
fixed_string HeapDisplayExpl = "Displays the heap.";

HeapDisplayCommand::HeapDisplayCommand() :
   CliCommand(HeapDisplayStr, HeapDisplayExpl) { }

word HeapDisplayCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HeapDisplayCommand.ProcessCommand");

   Heap* heap = nullptr;
   auto rc = CheckHeap(true, cli, heap);
   if(rc != 0) return rc;

   heap->Display(*cli.obuf, spaces(2), VerboseOpt);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string HeapFreeStr = "free";
fixed_string HeapFreeExpl = "Frees a block.";

HeapFreeCommand::HeapFreeCommand() :
   CliCommand(HeapFreeStr, HeapFreeExpl)
{
   BindParm(*new CliPtrParm(HeapBlockAddrExpl));
}

word HeapFreeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HeapFreeCommand.ProcessCommand");

   void* addr;

   if(!GetPtrParm(addr, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   Heap* heap = nullptr;
   auto rc = CheckHeap(true, cli, heap);
   if(rc != 0) return rc;

   heap->Free(addr);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------

fixed_string HeapValidateStr = "validate";
fixed_string HeapValidateExpl = "Validates the heap (if 0) or a block.";

HeapValidateCommand::HeapValidateCommand() :
   CliCommand(HeapValidateStr, HeapValidateExpl)
{
   BindParm(*new CliPtrParm(HeapBlockAddrExpl));
}

word HeapValidateCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HeapValidateCommand.ProcessCommand");

   void* addr;

   if(!GetPtrParm(addr, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   Heap* heap = nullptr;
   auto rc = CheckHeap(true, cli, heap);
   if(rc != 0) return rc;

   auto result = heap->Validate(addr);
   *cli.obuf << "  Result: " << result << CRLF;
   return 0;
}

//==============================================================================
//
//  Testing for LeakyBucketCounter.
//
class LbcPool : public Temporary
{
   friend class Singleton< LbcPool >;
public:
   LbcPool(const LbcPool& that) = delete;
   LbcPool& operator=(const LbcPool& that) = delete;
   LeakyBucketCounter lbc_;
private:
   LbcPool() = default;
   ~LbcPool() = default;
};

class LeakyBucketCounterCommands : public CliCommandSet
{
public:
   LeakyBucketCounterCommands();
};

class LbcInitCommand : public CliCommand
{
public:
   LbcInitCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class LbcEventCommand : public CliCommand
{
public:
   LbcEventCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

fixed_string LbcLimitExpl = "capacity of bucket (limit)";

//------------------------------------------------------------------------------

fixed_string LbcTimeExpl = "time to empty bucket (seconds)";

//------------------------------------------------------------------------------

fixed_string LeakyBucketCounterStr = "lbc";
fixed_string LeakyBucketCounterExpl = "Tests a LeakyBucketCounter function.";

LeakyBucketCounterCommands::LeakyBucketCounterCommands() :
   CliCommandSet(LeakyBucketCounterStr, LeakyBucketCounterExpl)
{
   BindCommand(*new LbcInitCommand);
   BindCommand(*new LbcEventCommand);
}

//------------------------------------------------------------------------------

fixed_string LbcInitStr = "init";
fixed_string LbcInitExpl = "Initializes the counter.";

LbcInitCommand::LbcInitCommand() : CliCommand(LbcInitStr, LbcInitExpl)
{
   BindParm(*new CliIntParm(LbcLimitExpl, 1, 3600));
   BindParm(*new CliIntParm(LbcTimeExpl, 1, 3600));
}

word LbcInitCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("LbcInitCommand.ProcessCommand");

   word limit, secs;

   if(!GetIntParm(limit, cli)) return -1;
   if(!GetIntParm(secs, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< LbcPool >::Instance();
   pool->lbc_.Initialize(limit, secs);
   pool->lbc_.Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string LbcEventStr = "event";
fixed_string LbcEventExpl = "Updates the counter when an event occurs.";

LbcEventCommand::LbcEventCommand() : CliCommand(LbcEventStr, LbcEventExpl) { }

word LbcEventCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("LbcEventCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   *cli.obuf << spaces(2);
   auto pool = Singleton< LbcPool >::Instance();
   if(pool->lbc_.HasReachedLimit())
      *cli.obuf << "The counter overflowed.";
   else
      *cli.obuf << "The counter did not overflow.";
   *cli.obuf << CRLF;
   pool->lbc_.Output(*cli.obuf, 2, true);
   return 0;
}

//==============================================================================
//
//  Testing for Q1Way.
//
fixed_string NullPtrExpl = "id=nullptr";

class Q1WayItem : public Temporary
{
public:
   explicit Q1WayItem(word index);
   ~Q1WayItem();
   Q1WayItem(const Q1WayItem& that) = delete;
   Q1WayItem& operator=(const Q1WayItem& that) = delete;
   static ptrdiff_t LinkDiff();
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;
private:
   const id_t index_;
   Q1Link link_;
};

class Q1WayPool : public Temporary
{
   friend class Singleton< Q1WayPool >;
public:
   Q1WayPool(const Q1WayPool& that) = delete;
   Q1WayPool& operator=(const Q1WayPool& that) = delete;
   void Reallocate();
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   static const size_t MaxItems = 8;
   std::unique_ptr< Q1WayItem > items_[MaxItems + 1];
   Q1Way< Q1WayItem > itemq_;
private:
   Q1WayPool();
   ~Q1WayPool() = default;
};

class Q1WayCommands : public CliCommandSet
{
public:
   Q1WayCommands();
};

class Countq1Command : public CliCommand
{
public:
   Countq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Deq1Command : public CliCommand
{
public:
   Deq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Emptyq1Command : public CliCommand
{
public:
   Emptyq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Enq1Command : public CliCommand
{
public:
   Enq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Exq1Command : public CliCommand
{
public:
   Exq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Firstq1Command : public CliCommand
{
public:
   Firstq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Henq1Command : public CliCommand
{
public:
   Henq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Insertq1Command : public CliCommand
{
public:
   Insertq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Nextq1Command : public CliCommand
{
public:
   Nextq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Purgeq1Command : public CliCommand
{
public:
   Purgeq1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

Q1WayItem::Q1WayItem(word index) : index_(index) { }

//------------------------------------------------------------------------------

Q1WayItem::~Q1WayItem()
{
   Singleton< Q1WayPool >::Extant()->items_[index_].release();
}

//------------------------------------------------------------------------------

void Q1WayItem::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(options.test(DispVerbose))
      stream << prefix << "index=" << index_ << CRLF;
   else
      stream << index_;
}

//------------------------------------------------------------------------------

ptrdiff_t Q1WayItem::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Q1WayItem* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fixed_string Q1WayItemIndexExpl = "item number (0 = nullptr)";

//------------------------------------------------------------------------------

Q1WayPool::Q1WayPool()
{
   itemq_.Init(Q1WayItem::LinkDiff());

   items_[0] = nullptr;

   for(auto i = 1; i <= MaxItems; ++i)
   {
      items_[i].reset(new Q1WayItem(i));
   }
}

//------------------------------------------------------------------------------

void Q1WayPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "Q1Way (size=" << itemq_.Size() << "): ";

   for(auto curr = itemq_.First(); curr != nullptr; itemq_.Next(curr))
   {
      curr->Display(stream, prefix + spaces(2), NoFlags);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Q1WayPool::Reallocate()
{
   for(auto i = 1; i <= MaxItems; ++i)
   {
      if(items_[i] == nullptr)
      {
         items_[i].reset(new Q1WayItem(i));
      }
   }
}

//------------------------------------------------------------------------------

fixed_string Q1WayStr = "q1";
fixed_string Q1WayExpl = "Tests a Q1Way function.";

Q1WayCommands::Q1WayCommands() : CliCommandSet(Q1WayStr, Q1WayExpl)
{
   BindCommand(*new Enq1Command);
   BindCommand(*new Henq1Command);
   BindCommand(*new Insertq1Command);
   BindCommand(*new Deq1Command);
   BindCommand(*new Exq1Command);
   BindCommand(*new Firstq1Command);
   BindCommand(*new Nextq1Command);
   BindCommand(*new Countq1Command);
   BindCommand(*new Emptyq1Command);
   BindCommand(*new Purgeq1Command);
}

//------------------------------------------------------------------------------

fixed_string Countq1Str = "count";
fixed_string Countq1Expl = "Returns the number of items in the queue.";

Countq1Command::Countq1Command() : CliCommand(Countq1Str, Countq1Expl) { }

word Countq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Countq1Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q1WayPool >::Instance();
   *cli.obuf << "  size=" << pool->itemq_.Size() << CRLF;
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Deq1Str = "deq";
fixed_string Deq1Expl = "Removes the item at the front of the queue.";

Deq1Command::Deq1Command() : CliCommand(Deq1Str, Deq1Expl) { }

word Deq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Deq1Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q1WayPool >::Instance();
   auto item = pool->itemq_.Deq();
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Emptyq1Str = "empty";
fixed_string Emptyq1Expl = "Returns true if the queue is empty.";

Emptyq1Command::Emptyq1Command() : CliCommand(Emptyq1Str, Emptyq1Expl) { }

word Emptyq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Emptyq1Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q1WayPool >::Instance();
   auto empty = pool->itemq_.Empty();
   *cli.obuf << "  empty=" << empty << CRLF;
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Enq1Str = "enq";
fixed_string Enq1Expl = "Adds an item to the end of the queue.";

Enq1Command::Enq1Command() : CliCommand(Enq1Str, Enq1Expl)
{
   BindParm(*new CliIntParm(Q1WayItemIndexExpl, 0, Q1WayPool::MaxItems));
}

word Enq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Enq1Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   if(id1 == 0)
   {
      *cli.obuf << NullPtrInvalid << CRLF;
      return -1;
   }
   auto pool = Singleton< Q1WayPool >::Instance();
   pool->itemq_.Enq(*pool->items_[id1]);
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Exq1Str = "exq";
fixed_string Exq1Expl = "Removes an item from anywhere in the queue.";

Exq1Command::Exq1Command() : CliCommand(Exq1Str, Exq1Expl)
{
   BindParm(*new CliIntParm(Q1WayItemIndexExpl, 0, Q1WayPool::MaxItems));
}

word Exq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Exq1Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   if(id1 == 0)
   {
      *cli.obuf << NullPtrInvalid << CRLF;
      return -1;
   }
   auto pool = Singleton< Q1WayPool >::Instance();
   pool->itemq_.Exq(*pool->items_[id1]);
   pool->items_[id1]->Output(*cli.obuf, 2, true);
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Firstq1Str = "first";
fixed_string Firstq1Expl = "Returns the first item in the queue.";

Firstq1Command::Firstq1Command() : CliCommand(Firstq1Str, Firstq1Expl) { }

word Firstq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Firstq1Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q1WayPool >::Instance();

   auto item = pool->itemq_.First();
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Henq1Str = "henq";
fixed_string Henq1Expl = "Adds an item to the front of the queue.";

Henq1Command::Henq1Command() : CliCommand(Henq1Str, Henq1Expl)
{
   BindParm(*new CliIntParm(Q1WayItemIndexExpl, 0, Q1WayPool::MaxItems));
}

word Henq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Henq1Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   if(id1 == 0)
   {
      *cli.obuf << NullPtrInvalid << CRLF;
      return -1;
   }
   auto pool = Singleton< Q1WayPool >::Instance();
   pool->itemq_.Henq(*pool->items_[id1]);
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Insertq1Str = "insert";
fixed_string Insertq1Expl = "Inserts item#2 after item#1.";

Insertq1Command::Insertq1Command() : CliCommand(Insertq1Str, Insertq1Expl)
{
   BindParm(*new CliIntParm(Q1WayItemIndexExpl, 0, Q1WayPool::MaxItems));
   BindParm(*new CliIntParm(Q1WayItemIndexExpl, 0, Q1WayPool::MaxItems));
}

word Insertq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Insertq1Command.ProcessCommand");

   word id1, id2;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetIntParm(id2, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   if(id2 == 0)
   {
      *cli.obuf << NullPtrInvalid << CRLF;
      return -1;
   }
   auto pool = Singleton< Q1WayPool >::Instance();
   pool->itemq_.Insert(pool->items_[id1].get(), *pool->items_[id2]);
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Nextq1Str = "next";
fixed_string Nextq1Expl = "Returns the next item in the queue.";

Nextq1Command::Nextq1Command() : CliCommand(Nextq1Str, Nextq1Expl)
{
   BindParm(*new CliIntParm(Q1WayItemIndexExpl, 0, Q1WayPool::MaxItems));
}

word Nextq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Nextq1Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< Q1WayPool >::Instance();
   auto item = pool->items_[id1].get();

   *cli.obuf << "Next(T*&): " << CRLF;
   pool->itemq_.Next(item);
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;

   *cli.obuf << "T*=Next(T&): " << CRLF;
   if(id1 != 0)
   {
      item = pool->itemq_.Next(*pool->items_[id1]);
      if(item != nullptr)
         item->Output(*cli.obuf, 2, true);
      else
         *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   }
   else
   {
      *cli.obuf << NullPtrInvalid << CRLF;
   }

   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Purgeq1Str = "purge";
fixed_string Purgeq1Expl = "Deletes all the items in the queue.";

Purgeq1Command::Purgeq1Command() : CliCommand(Purgeq1Str, Purgeq1Expl) { }

word Purgeq1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Purgeq1Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q1WayPool >::Instance();
   pool->itemq_.Purge();
   pool->Output(*cli.obuf, 2, false);
   pool->Reallocate();
   return 0;
}

//==============================================================================
//
//  Testing for Q2Way.
//
class Q2WayItem : public Temporary
{
public:
   explicit Q2WayItem(word index);
   ~Q2WayItem();
   Q2WayItem(const Q2WayItem& that) = delete;
   Q2WayItem& operator=(const Q2WayItem& that) = delete;
   static ptrdiff_t LinkDiff();
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;
private:
   const id_t index_;
   Q2Link link_;
};

class Q2WayPool : public Temporary
{
   friend class Singleton< Q2WayPool >;
public:
   Q2WayPool(const Q2WayPool& that) = delete;
   Q2WayPool& operator=(const Q2WayPool& that) = delete;
   void Reallocate();
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   static const size_t MaxItems = 8;
   std::unique_ptr< Q2WayItem > items_[MaxItems + 1];
   Q2Way< Q2WayItem > itemq_;
private:
   Q2WayPool();
   ~Q2WayPool() = default;
};

class Q2WayCommands : public CliCommandSet
{
public:
   Q2WayCommands();
};

class Countq2Command : public CliCommand
{
public:
   Countq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Deq2Command : public CliCommand
{
public:
   Deq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Emptyq2Command : public CliCommand
{
public:
   Emptyq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Enq2Command : public CliCommand
{
public:
   Enq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Exq2Command : public CliCommand
{
public:
   Exq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Firstq2Command : public CliCommand
{
public:
   Firstq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Henq2Command : public CliCommand
{
public:
   Henq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Lastq2Command : public CliCommand
{
public:
   Lastq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Nextq2Command : public CliCommand
{
public:
   Nextq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Prevq2Command : public CliCommand
{
public:
   Prevq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class Purgeq2Command : public CliCommand
{
public:
   Purgeq2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

Q2WayItem::Q2WayItem(word index) : index_(index) { }

//------------------------------------------------------------------------------

Q2WayItem::~Q2WayItem()
{
   Singleton< Q2WayPool >::Extant()->items_[index_].release();
}

//------------------------------------------------------------------------------

void Q2WayItem::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(options.test(DispVerbose))
      stream << prefix << "index=" << index_ << CRLF;
   else
      stream << index_;
}

//------------------------------------------------------------------------------

ptrdiff_t Q2WayItem::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Q2WayItem* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fixed_string Q2WayItemIndexExpl = "item number (0 = nullptr)";

//------------------------------------------------------------------------------

Q2WayPool::Q2WayPool()
{
   itemq_.Init(Q2WayItem::LinkDiff());

   items_[0] = nullptr;

   for(auto i = 1; i <= MaxItems; ++i)
   {
      items_[i].reset(new Q2WayItem(i));
   }
}

//------------------------------------------------------------------------------

void Q2WayPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "Q2Way (size=" << itemq_.Size() << "): ";

   for(auto curr = itemq_.First(); curr != nullptr; itemq_.Next(curr))
   {
      curr->Display(stream, prefix + spaces(2), NoFlags);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Q2WayPool::Reallocate()
{
   for(auto i = 1; i <= MaxItems; ++i)
   {
      if(items_[i] == nullptr)
      {
         items_[i].reset(new Q2WayItem(i));
      }
   }
}

//------------------------------------------------------------------------------

fixed_string Q2WayStr = "q2";
fixed_string Q2WayExpl = "Tests a Q2Way function.";

Q2WayCommands::Q2WayCommands() : CliCommandSet(Q2WayStr, Q2WayExpl)
{
   BindCommand(*new Enq2Command);
   BindCommand(*new Henq2Command);
   BindCommand(*new Deq2Command);
   BindCommand(*new Exq2Command);
   BindCommand(*new Firstq2Command);
   BindCommand(*new Nextq2Command);
   BindCommand(*new Lastq2Command);
   BindCommand(*new Prevq2Command);
   BindCommand(*new Countq2Command);
   BindCommand(*new Emptyq2Command);
   BindCommand(*new Purgeq2Command);
}

//------------------------------------------------------------------------------

fixed_string Countq2Str = "count";
fixed_string Countq2Expl = "Returns the number of items in the queue.";

Countq2Command::Countq2Command() : CliCommand(Countq2Str, Countq2Expl) { }

word Countq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Countq2Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q2WayPool >::Instance();
   *cli.obuf << "  size=" << pool->itemq_.Size() << CRLF;
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Deq2Str = "deq";
fixed_string Deq2Expl = "Removes the item at the front of the queue.";

Deq2Command::Deq2Command() : CliCommand(Deq2Str, Deq2Expl) { }

word Deq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Deq2Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q2WayPool >::Instance();
   auto item = pool->itemq_.Deq();
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Emptyq2Str = "empty";
fixed_string Emptyq2Expl = "Returns true if the queue is empty.";

Emptyq2Command::Emptyq2Command() : CliCommand(Emptyq2Str, Emptyq2Expl) { }

word Emptyq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Emptyq2Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q2WayPool >::Instance();
   auto empty = pool->itemq_.Empty();
   *cli.obuf << "  empty=" << empty << CRLF;
   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Enq2Str = "enq";
fixed_string Enq2Expl = "Adds an item to the end of the queue.";

Enq2Command::Enq2Command() : CliCommand(Enq2Str, Enq2Expl)
{
   BindParm(*new CliIntParm(Q2WayItemIndexExpl, 0, Q2WayPool::MaxItems));
}

word Enq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Enq2Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   if(id1 == 0)
   {
      *cli.obuf << NullPtrInvalid << CRLF;
      return -1;
   }
   auto pool = Singleton< Q2WayPool >::Instance();
   pool->itemq_.Enq(*pool->items_[id1]);
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Exq2Str = "exq";
fixed_string Exq2Expl = "Removes an item from anywhere in the queue.";

Exq2Command::Exq2Command() : CliCommand(Exq2Str, Exq2Expl)
{
   BindParm(*new CliIntParm(Q2WayItemIndexExpl, 0, Q2WayPool::MaxItems));
}

word Exq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Exq2Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   if(id1 == 0)
   {
      *cli.obuf << NullPtrInvalid << CRLF;
      return -1;
   }
   auto pool = Singleton< Q2WayPool >::Instance();
   pool->itemq_.Exq(*pool->items_[id1]);
   pool->items_[id1]->Output(*cli.obuf, 2, true);
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Firstq2Str = "first";
fixed_string Firstq2Expl = "Returns the first item in the queue.";

Firstq2Command::Firstq2Command() : CliCommand(Firstq2Str, Firstq2Expl) { }

word Firstq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Firstq2Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q2WayPool >::Instance();

   *cli.obuf << "T*=First(): " << CRLF;
   auto item = pool->itemq_.First();
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;

   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Henq2Str = "henq";
fixed_string Henq2Expl = "Adds an item to the front of the queue.";

Henq2Command::Henq2Command() : CliCommand(Henq2Str, Henq2Expl)
{
   BindParm(*new CliIntParm(Q2WayItemIndexExpl, 0, Q2WayPool::MaxItems));
}

word Henq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Henq2Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   if(id1 == 0)
   {
      *cli.obuf << NullPtrInvalid << CRLF;
      return -1;
   }
   auto pool = Singleton< Q2WayPool >::Instance();
   pool->itemq_.Henq(*pool->items_[id1]);
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Lastq2Str = "last";
fixed_string Lastq2Expl = "Returns the last item in the queue.";

Lastq2Command::Lastq2Command() : CliCommand(Lastq2Str, Lastq2Expl) { }

word Lastq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Lastq2Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q2WayPool >::Instance();

   *cli.obuf << "T*=Last(): " << CRLF;
   auto item = pool->itemq_.Last();
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;

   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Nextq2Str = "next";
fixed_string Nextq2Expl = "Returns the next item in the queue.";

Nextq2Command::Nextq2Command() : CliCommand(Nextq2Str, Nextq2Expl)
{
   BindParm(*new CliIntParm(Q2WayItemIndexExpl, 0, Q2WayPool::MaxItems));
}

word Nextq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Nextq2Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< Q2WayPool >::Instance();
   auto item = pool->items_[id1].get();

   *cli.obuf << "Next(T*&): " << CRLF;
   pool->itemq_.Next(item);
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;

   *cli.obuf << "T*=Next(T&): " << CRLF;
   if(id1 != 0)
   {
      item = pool->itemq_.Next(*pool->items_[id1]);
      if(item != nullptr)
         item->Output(*cli.obuf, 2, true);
      else
         *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   }
   else
   {
      *cli.obuf << NullPtrInvalid << CRLF;
   }

   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Prevq2Str = "prev";
fixed_string Prevq2Expl = "Returns the previous item.";

Prevq2Command::Prevq2Command() : CliCommand(Prevq2Str, Prevq2Expl)
{
   BindParm(*new CliIntParm(Q2WayItemIndexExpl, 0, Q2WayPool::MaxItems));
}

word Prevq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Prevq2Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< Q2WayPool >::Instance();
   auto item = pool->items_[id1].get();

   *cli.obuf << "Prev(T*&): " << CRLF;
   pool->itemq_.Prev(item);
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;

   *cli.obuf << "T*=Prev(T&): " << CRLF;
   if(id1 != 0)
   {
      item = pool->itemq_.Prev(*pool->items_[id1]);
      if(item != nullptr)
         item->Output(*cli.obuf, 2, true);
      else
         *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   }
   else
   {
      *cli.obuf << NullPtrInvalid << CRLF;
   }

   pool->Output(*cli.obuf, 2, false);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string Purgeq2Str = "purge";
fixed_string Purgeq2Expl = "Deletes all the items in the queue.";

Purgeq2Command::Purgeq2Command() : CliCommand(Purgeq2Str, Purgeq2Expl) { }

word Purgeq2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("Purgeq2Command.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< Q2WayPool >::Instance();
   pool->itemq_.Purge();
   pool->Output(*cli.obuf, 2, false);
   pool->Reallocate();
   return 0;
}

//==============================================================================
//
//  Testing for Registry.
//
class RegistryItem : public Temporary
{
public:
   explicit RegistryItem(word index);
   ~RegistryItem();
   RegistryItem(const RegistryItem& that) = delete;
   RegistryItem& operator=(const RegistryItem& that) = delete;
   static ptrdiff_t CellDiff();
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   RegCell rid_;
private:
   const id_t index_;
};

class RegistryPool : public Temporary
{
   friend class Singleton< RegistryPool >;
public:
   RegistryPool(const RegistryPool& that) = delete;
   RegistryPool& operator=(const RegistryPool& that) = delete;
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   static const size_t MaxItems = 8;
   std::unique_ptr< RegistryItem > items_[MaxItems + 1];
   Registry< RegistryItem > registry_;
private:
   RegistryPool();
   ~RegistryPool() = default;
};

class RegistryCommands : public CliCommandSet
{
public:
   RegistryCommands();
};

class InitCommand : public CliCommand
{
public:
   InitCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class InsertCommand : public CliCommand
{
public:
   InsertCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class RemoveCommand : public CliCommand
{
public:
   RemoveCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class AtCommand : public CliCommand
{
public:
   AtCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class FirstCommand : public CliCommand
{
public:
   FirstCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class NextCommand : public CliCommand
{
public:
   NextCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class LastCommand : public CliCommand
{
public:
   LastCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class PrevCommand : public CliCommand
{
public:
   PrevCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class CountCommand : public CliCommand
{
public:
   CountCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

RegistryItem::RegistryItem(word index) : index_(index) { }

//------------------------------------------------------------------------------

RegistryItem::~RegistryItem()
{
   Singleton< RegistryPool >::Extant()->items_[index_].release();
}

//------------------------------------------------------------------------------

ptrdiff_t RegistryItem::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const RegistryItem* >(&local);
   return ptrdiff(&fake->rid_, fake);
}

//------------------------------------------------------------------------------

void RegistryItem::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "index=" << index_ << CRLF;
}

//------------------------------------------------------------------------------

fixed_string RegistryItemIndexExpl = "item number (0 = nullptr)";

//------------------------------------------------------------------------------

RegistryPool::RegistryPool()
{
   items_[0] = nullptr;

   for(auto i = 1; i <= MaxItems; ++i)
   {
      items_[i].reset(new RegistryItem(i));
   }
}

//------------------------------------------------------------------------------

void RegistryPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "Registry:" << CRLF;
   registry_.Display(stream, prefix + spaces(2), VerboseOpt);
   stream << CRLF;
}

//------------------------------------------------------------------------------

fixed_string RegistryIdExpl = "registrant id";

//------------------------------------------------------------------------------

fixed_string RegistrySizeExpl = "maximum number of items in registry";

//------------------------------------------------------------------------------

fixed_string RegistryStr = "reg";
fixed_string RegistryExpl = "Tests a Registry function.";

RegistryCommands::RegistryCommands() :
   CliCommandSet(RegistryStr, RegistryExpl)
{
   BindCommand(*new InitCommand);
   BindCommand(*new InsertCommand);
   BindCommand(*new RemoveCommand);
   BindCommand(*new AtCommand);
   BindCommand(*new FirstCommand);
   BindCommand(*new NextCommand);
   BindCommand(*new LastCommand);
   BindCommand(*new PrevCommand);
   BindCommand(*new CountCommand);
}

//------------------------------------------------------------------------------

fixed_string InitStr = "init";
fixed_string InitExpl = "Initializes the registry.";

InitCommand::InitCommand() : CliCommand(InitStr, InitExpl)
{
   BindParm(*new CliIntParm(RegistrySizeExpl, 0, RegistryPool::MaxItems));
}

word InitCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("InitCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< RegistryPool >::Instance();
   auto result = pool->registry_.Init
      (id1, RegistryItem::CellDiff(), MemTemporary, false);
   *cli.obuf << "  rc=" << result << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string InsertStr = "insert";
fixed_string InsertExpl = "Adds an item to the registry.";

InsertCommand::InsertCommand() : CliCommand(InsertStr, InsertExpl)
{
   BindParm(*new CliIntParm(RegistryItemIndexExpl, 0, RegistryPool::MaxItems));
   BindParm(*new CliIntParm(RegistryIdExpl, 0, 31, true));
}

word InsertCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("InsertCommand.ProcessCommand");

   word id1, id2;
   bool fixed;

   if(!GetIntParm(id1, cli)) return -1;

   switch(GetIntParmRc(id2, cli))
   {
   case None: fixed = false; break;
   case Ok: fixed = true; break;
   default: return -1;
   }

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< RegistryPool >::Instance();
   if(id1 > 0)
   {
      if(fixed)
         pool->items_[id1]->rid_.SetId(id2);
      else
         pool->items_[id1]->rid_.SetId(0);
   }
   auto result = pool->registry_.Insert(*pool->items_[id1]);
   *cli.obuf << "  rc=" << result << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string RemoveStr = "remove";
fixed_string RemoveExpl = "Removes an item from the registry.";

RemoveCommand::RemoveCommand() : CliCommand(RemoveStr, RemoveExpl)
{
   BindParm(*new CliIntParm(RegistryItemIndexExpl, 0, RegistryPool::MaxItems));
   BindParm(*new CliIntParm(RegistryIdExpl, 0, 31, true));
}

word RemoveCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("RemoveCommand.ProcessCommand");

   word id1, id2;
   bool fixed, result;

   if(!GetIntParm(id1, cli)) return -1;

   switch(GetIntParmRc(id2, cli))
   {
   case None: fixed = false; break;
   case Ok: fixed = true; break;
   default: return -1;
   }

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< RegistryPool >::Instance();
   if(fixed)
      result = pool->registry_.Erase(*pool->items_[id1], id2);
   else
      result = pool->registry_.Erase(*pool->items_[id1]);
   *cli.obuf << "  rc=" << result << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string AtStr = "at";
fixed_string AtExpl = "Accesses an item in the registry.";

AtCommand::AtCommand() : CliCommand(AtStr, AtExpl)
{
   BindParm(*new CliIntParm(RegistryIdExpl, 0, 31));
}

word AtCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("AtCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< RegistryPool >::Instance();
   auto item = pool->registry_.At(id1);
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string FirstStr = "first";
fixed_string FirstExpl = "Returns the first item in the registry.";

FirstCommand::FirstCommand() : CliCommand(FirstStr, FirstExpl)
{
   BindParm(*new CliIntParm(RegistryIdExpl, 0, 31, true));
}

word FirstCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("FirstCommand.ProcessCommand");

   word id1;
   bool start;
   id_t rid;
   RegistryItem* item;

   switch(GetIntParmRc(id1, cli))
   {
   case None: start = false; break;
   case Ok: start = true; break;
   default: return -1;
   }

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< RegistryPool >::Instance();
   if(start)
   {
      rid = id1;
      item = pool->registry_.First(rid);
   }
   else
   {
      item = pool->registry_.First();
   }
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string NextStr = "next";
fixed_string NextExpl = "Returns the next item in the registry.";

NextCommand::NextCommand() : CliCommand(NextStr, NextExpl)
{
   BindParm(*new CliIntParm(RegistryItemIndexExpl, 0, RegistryPool::MaxItems));
}

word NextCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("NextCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< RegistryPool >::Instance();
   auto item = pool->items_[id1].get();

   *cli.obuf << "Next(T*&): " << CRLF;
   pool->registry_.Next(item);
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;

   *cli.obuf << "T*=Next(T&): " << CRLF;
   if(id1 != 0)
   {
      item = pool->registry_.Next(*pool->items_[id1]);
      if(item != nullptr)
         item->Output(*cli.obuf, 2, true);
      else
         *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   }
   else
   {
      *cli.obuf << NullPtrInvalid << CRLF;
   }

   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string LastStr = "last";
fixed_string LastExpl = "Returns the last item in the registry.";

LastCommand::LastCommand() : CliCommand(LastStr, LastExpl) { }

word LastCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("LastCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< RegistryPool >::Instance();
   auto item = pool->registry_.Last();
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string PrevStr = "prev";
fixed_string PrevExpl = "Returns the previous item in the registry.";

PrevCommand::PrevCommand() : CliCommand(PrevStr, PrevExpl)
{
   BindParm(*new CliIntParm(RegistryItemIndexExpl, 0, RegistryPool::MaxItems));
}

word PrevCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("PrevCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< RegistryPool >::Instance();
   auto item = pool->items_[id1].get();

   *cli.obuf << "Prev(T*&): " << CRLF;
   pool->registry_.Prev(item);
   if(item != nullptr)
      item->Output(*cli.obuf, 2, true);
   else
      *cli.obuf << spaces(2) << NullPtrExpl << CRLF;

   *cli.obuf << "T*=Prev(T&): " << CRLF;
   if(id1 != 0)
   {
      item = pool->registry_.Prev(*pool->items_[id1]);
      if(item != nullptr)
         item->Output(*cli.obuf, 2, true);
      else
         *cli.obuf << spaces(2) << NullPtrExpl << CRLF;
   }
   else
   {
      *cli.obuf << NullPtrInvalid << CRLF;
   }

   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string CountStr = "count";
fixed_string CountExpl = "Returns the number of items in the registry.";

CountCommand::CountCommand() : CliCommand(CountStr, CountExpl) { }

word CountCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CountCommand.ProcessCommand[>nt]");

   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< RegistryPool >::Instance();
   *cli.obuf << "  size=" << pool->registry_.Size() << CRLF;
   pool->Output(*cli.obuf, 2, true);
   return 0;
}

//==============================================================================
//
//  Testing for SysTime.
//
class SysTimePool : public Temporary
{
   friend class Singleton< SysTimePool >;
public:
   SysTimePool(const SysTimePool& that) = delete;
   SysTimePool& operator=(const SysTimePool& that) = delete;

   static const size_t MaxIndex = 3;
   SysTime time_[MaxIndex + 1];
private:
   SysTimePool() = default;
   ~SysTimePool() = default;
};

class SysTimeCommands : public CliCommandSet
{
public:
   SysTimeCommands();
};

class TimeCtor1Command : public CliCommand
{
public:
   TimeCtor1Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class TimeCtor2Command : public CliCommand
{
public:
   TimeCtor2Command();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class DayOfWeekCommand : public CliCommand
{
public:
   DayOfWeekCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class DayOfYearCommand : public CliCommand
{
public:
   DayOfYearCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class IsLeapYearCommand : public CliCommand
{
public:
   IsLeapYearCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class TruncateCommand : public CliCommand
{
public:
   TruncateCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class RoundCommand : public CliCommand
{
public:
   RoundCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class AddMsecsCommand : public CliCommand
{
public:
   AddMsecsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class SubMsecsCommand : public CliCommand
{
public:
   SubMsecsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class MsecsFromNowCommand : public CliCommand
{
public:
   MsecsFromNowCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class MsecsUntilCommand : public CliCommand
{
public:
   MsecsUntilCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class AddDaysCommand : public CliCommand
{
public:
   AddDaysCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class SubDaysCommand : public CliCommand
{
public:
   SubDaysCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

class StrTimeCommand : public CliCommand
{
public:
   StrTimeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

fixed_string SysTimeIndexExpl = "item number";

//------------------------------------------------------------------------------

fixed_string SysTimeIntervalExpl =
   "interval (must evenly divide the field's range)";

//------------------------------------------------------------------------------

fixed_string SysTimeMsecsExpl = "number of milliseconds";

//------------------------------------------------------------------------------

fixed_string SysTimeDaysExpl = "number of days";

//------------------------------------------------------------------------------

fixed_string SysTimeStr = "time";
fixed_string SysTimeExpl = "Tests a SysTime function.";

SysTimeCommands::SysTimeCommands() : CliCommandSet(SysTimeStr, SysTimeExpl)
{
   BindCommand(*new TimeCtor1Command);
   BindCommand(*new TimeCtor2Command);
   BindCommand(*new DayOfWeekCommand);
   BindCommand(*new DayOfYearCommand);
   BindCommand(*new IsLeapYearCommand);
   BindCommand(*new TruncateCommand);
   BindCommand(*new RoundCommand);
   BindCommand(*new AddMsecsCommand);
   BindCommand(*new SubMsecsCommand);
   BindCommand(*new MsecsFromNowCommand);
   BindCommand(*new MsecsUntilCommand);
   BindCommand(*new AddDaysCommand);
   BindCommand(*new SubDaysCommand);
   BindCommand(*new StrTimeCommand);
}

//------------------------------------------------------------------------------

fixed_string TimeCtor1Str = "ctor1";
fixed_string TimeCtor1Expl = "Constructs the current time.";

TimeCtor1Command::TimeCtor1Command() : CliCommand(TimeCtor1Str, TimeCtor1Expl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
}

word TimeCtor1Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TimeCtor1Command.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1] = SysTime();
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string TimeCtor2Str = "ctor2";
fixed_string TimeCtor2Expl = "Constructs a specified time.";

TimeCtor2Command::TimeCtor2Command() : CliCommand(TimeCtor2Str, TimeCtor2Expl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new SysTimeYearParm);
   BindParm(*new SysTimeMonthParm);
   BindParm(*new SysTimeDayParm);
   BindParm(*new SysTimeHourParm);
   BindParm(*new SysTimeMinuteParm);
   BindParm(*new SysTimeSecondParm);
   BindParm(*new SysTimeMsecondParm);
}

word TimeCtor2Command::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TimeCtor2Command.ProcessCommand");

   word id1, year, month, day, hour, min, sec, msec;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetIntParm(year, cli)) return -1;
   if(!GetIntParm(month, cli)) return -1;
   if(!GetIntParm(day, cli)) return -1;
   if(!GetIntParm(hour, cli)) return -1;
   if(!GetIntParm(min, cli)) return -1;
   if(!GetIntParm(sec, cli)) return -1;
   if(!GetIntParm(msec, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1] = SysTime(year, month - 1, day, hour, min, sec, msec);
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string DayOfWeekStr = "dayofweek";
fixed_string DayOfWeekExpl = "Returns the time's day of the week.";

DayOfWeekCommand::DayOfWeekCommand() : CliCommand(DayOfWeekStr, DayOfWeekExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
}

word DayOfWeekCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DayOfWeekCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   *cli.obuf << "  day=" << pool->time_[id1].strWeekDay() << CRLF;
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string DayOfYearStr = "dayofyear";
fixed_string DayOfYearExpl = "Returns the time's day of the year.";

DayOfYearCommand::DayOfYearCommand() : CliCommand(DayOfYearStr, DayOfYearExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
}

word DayOfYearCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DayOfYearCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   *cli.obuf << "  day=" << pool->time_[id1].DayOfYear() + 1 << CRLF;
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string IsLeapYearStr = "isleapyear";
fixed_string IsLeapYearExpl = "Returns true if a year is a leap year.";

IsLeapYearCommand::IsLeapYearCommand() :
   CliCommand(IsLeapYearStr, IsLeapYearExpl)
{
   BindParm(*new SysTimeYearParm);
}

word IsLeapYearCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("IsLeapYearCommand.ProcessCommand");

   word year;

   if(!GetIntParm(year, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   *cli.obuf << "  leap year=" << SysTime::IsLeapYear(year) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string TruncateStr = "truncate";
fixed_string TruncateExpl = "Truncates the time at a specified field.";

TruncateCommand::TruncateCommand() : CliCommand(TruncateStr, TruncateExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new SysTimeFieldParm);
}

word TruncateCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TruncateCommand.ProcessCommand");

   id_t field;
   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetTextIndex(field, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1].Truncate(TimeField(field - 1));
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string RoundStr = "round";
fixed_string RoundExpl = "Rounds off the time at a specified field.";

RoundCommand::RoundCommand() : CliCommand(RoundStr, RoundExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new SysTimeFieldParm);
   BindParm(*new CliIntParm(SysTimeIntervalExpl, 1, 500));
}

word RoundCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("RoundCommand.ProcessCommand");

   word id1, interval;
   id_t field;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetTextIndex(field, cli)) return -1;
   if(!GetIntParm(interval, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1].Round(TimeField(field - 1), interval);
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string AddMsecsStr = "addmsecs";
fixed_string AddMsecsExpl = "Adds milliseconds to the time.";

AddMsecsCommand::AddMsecsCommand() : CliCommand(AddMsecsStr, AddMsecsExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new CliIntParm(SysTimeMsecsExpl, WORD_MIN, WORD_MAX));
}

word AddMsecsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("AddMsecsCommand.ProcessCommand");

   word id1, msecs;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetIntParm(msecs, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1].AddMsecs(msecs);
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string SubMsecsStr = "submsecs";
fixed_string SubMsecsExpl = "Subtracts milliseconds from the time.";

SubMsecsCommand::SubMsecsCommand() : CliCommand(SubMsecsStr, SubMsecsExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new CliIntParm(SysTimeMsecsExpl, WORD_MIN, WORD_MAX));
}

word SubMsecsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SubMsecsCommand.ProcessCommand");

   word id1, msecs;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetIntParm(msecs, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1].SubMsecs(msecs);
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string MsecsFromNowStr = "msecsfromnow";
fixed_string MsecsFromNowExpl = "Returns the milliseconds from now to a time.";

MsecsFromNowCommand::MsecsFromNowCommand() :
   CliCommand(MsecsFromNowStr, MsecsFromNowExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
}

word MsecsFromNowCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MsecsFromNowCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   *cli.obuf << "  msecs=" << pool->time_[id1].MsecsFromNow() << CRLF;
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string MsecsUntilStr = "msecsuntil";
fixed_string MsecsUntilExpl =
   "Returns the milliseconds from one time to another.";

MsecsUntilCommand::MsecsUntilCommand() :
   CliCommand(MsecsUntilStr, MsecsUntilExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
}

word MsecsUntilCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MsecsUntilCommand.ProcessCommand");

   word id1, id2;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetIntParm(id2, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   *cli.obuf << "  msecs=" << pool->time_[id1].MsecsUntil(pool->time_[id2])
      << CRLF;
   *cli.obuf << "  time1=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   *cli.obuf << "  time2=" << pool->time_[id2].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string AddDaysStr = "adddays";
fixed_string AddDaysExpl = "Adds days to the time.";

AddDaysCommand::AddDaysCommand() : CliCommand(AddDaysStr, AddDaysExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new CliIntParm(SysTimeDaysExpl, WORD_MIN, WORD_MAX));
}

word AddDaysCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("AddDaysCommand.ProcessCommand");

   word id1, days;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetIntParm(days, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1].AddDays(days);
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string SubDaysStr = "subdays";
fixed_string SubDaysExpl = "Subtracts days from the time.";

SubDaysCommand::SubDaysCommand() : CliCommand(SubDaysStr, SubDaysExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
   BindParm(*new CliIntParm(SysTimeDaysExpl, WORD_MIN, WORD_MAX));
}

word SubDaysCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SubDaysCommand.ProcessCommand");

   word id1, days;

   if(!GetIntParm(id1, cli)) return -1;
   if(!GetIntParm(days, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   pool->time_[id1].SubDays(days);
   *cli.obuf << "  time=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

fixed_string StrTimeStr = "strtime";
fixed_string StrTimeExpl = "Displays the time in various formats.";

StrTimeCommand::StrTimeCommand() : CliCommand(StrTimeStr, StrTimeExpl)
{
   BindParm(*new CliIntParm(SysTimeIndexExpl, 1, SysTimePool::MaxIndex));
}

word StrTimeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("StrTimeCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;
   auto pool = Singleton< SysTimePool >::Instance();
   *cli.obuf << "   a=" << pool->time_[id1].to_str(SysTime::Alpha) << CRLF;
   *cli.obuf << "  la=" << pool->time_[id1].to_str(SysTime::LowAlpha) << CRLF;
   *cli.obuf << "   n=" << pool->time_[id1].to_str(SysTime::Numeric) << CRLF;
   *cli.obuf << "  hn=" << pool->time_[id1].to_str(SysTime::HighNumeric)
      << CRLF;
   return 0;
}

//==============================================================================
//
//  Daemon and thread for testing the safety net.
//
class RecoveryDaemon : public Daemon
{
   friend class Singleton< RecoveryDaemon >;

   RecoveryDaemon();
   ~RecoveryDaemon();
   Thread* CreateThread() override;
   AlarmStatus GetAlarmLevel() const override;
};

//------------------------------------------------------------------------------

class RecoveryThread : public Thread
{
   friend class Singleton< RecoveryThread >;
public:
   enum Test
   {
      Sleep,
      Abort,
      Create,
      CtorTrap,
      DtorTrap,
      Delete,
      DerefenceBadPtr,
      DivideByZero,
      InfiniteLoop,
      MutexBlock,
      MutexExit,
      MutexTrap,
      OverflowStack,
      RaiseSignal,
      Return,
      SwErr,
      Terminate,
      Trap
   };

   void SetTest(Test test) { test_ = test; }
   void SetTestSignal(signal_t signal) { signal_ = signal; }
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;
private:
   RecoveryThread();
   ~RecoveryThread();
   static void AcquireMutex();
   static void DoAbort();
   static void DoDelete();
   static int DoDivide();
   static void DoSwErr();
   static void DoTerminate();
   static void LoopForever();
   static void RecurseForever(size_t depth);
   static void UseBadPointer();
   void DoRaise() const;
   void DoTrap();
   c_string AbbrName() const override;
   void Destroy() override;
   void Enter() override;
   bool Recover() override;

   Test test_;
   signal_t signal_;
};

static SysMutex RecoveryMutex_("RecoveryMutex");

//------------------------------------------------------------------------------

fixed_string RecoveryDaemonName = "recover";

RecoveryDaemon::RecoveryDaemon() : Daemon(RecoveryDaemonName, 1)
{
   Debug::ft("RecoveryDaemon.ctor");
}

RecoveryDaemon::~RecoveryDaemon()
{
   Debug::ftnt("RecoveryDaemon.dtor");
}

Thread* RecoveryDaemon::CreateThread()
{
   Debug::ft("RecoveryDaemon.CreateThread");
   return Singleton< RecoveryThread >::Instance();
}

AlarmStatus RecoveryDaemon::GetAlarmLevel() const
{
   Debug::ft("RecoveryDaemon.GetAlarmLevel");
   return MinorAlarm;
}

//------------------------------------------------------------------------------

RecoveryThread::RecoveryThread() :
   Thread(LoadTestFaction, Singleton< RecoveryDaemon >::Instance()),
   test_(Sleep),
   signal_(0)
{
   Debug::ft("RecoveryThread.ctor");

   //  Set ThreadCtorTrapFlag to cause a trap during thread creation.  This
   //  tests orphan recovery and a single daemon trap.  If ThreadCtorRetrapFlag
   //  is also set, it tests a double daemon trap, which should disable the
   //  daemon.  Reenabling the daemon will then recreate this thread.
   //
   if(Debug::SwFlagOn(ThreadCtorTrapFlag))
   {
      Debug::SetSwFlag(ThreadCtorTrapFlag, false);
      UseBadPointer();
   }

   if(Debug::SwFlagOn(ThreadCtorRetrapFlag))
   {
      Debug::SetSwFlag(ThreadCtorRetrapFlag, false);
      UseBadPointer();
   }

   SetInitialized();
}

//------------------------------------------------------------------------------

RecoveryThread::~RecoveryThread()
{
   Debug::ftnt("RecoveryThread.dtor");

   if(Debug::SwFlagOn(ThreadDtorTrapFlag))
   {
      Debug::SetSwFlag(ThreadDtorTrapFlag, false);
      UseBadPointer();
   }
}

//------------------------------------------------------------------------------

c_string RecoveryThread::AbbrName() const
{
   return RecoveryDaemonName;
}

//------------------------------------------------------------------------------

fn_name RecoveryThread_AcquireMutex = "RecoveryThread.AcquireMutex";

void RecoveryThread::AcquireMutex()
{
   Debug::ft(RecoveryThread_AcquireMutex);

   auto rc = RecoveryMutex_.Acquire(TIMEOUT_IMMED);
   if(rc != SysMutex::Acquired)
      Debug::SwLog(RecoveryThread_AcquireMutex, "acquire failed", rc);
}

//------------------------------------------------------------------------------

void RecoveryThread::Destroy()
{
   Debug::ft("RecoveryThread.Destroy");

   Singleton< RecoveryThread >::Destroy();
}

//------------------------------------------------------------------------------

void RecoveryThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "test   : " << int(test_) << CRLF;
   stream << prefix << "signal : " << signal_ << CRLF;
}

//------------------------------------------------------------------------------

void RecoveryThread::DoAbort()
{
   Debug::ft("RecoveryThread.DoAbort");

   std::abort();
}

//------------------------------------------------------------------------------

void RecoveryThread::DoDelete()
{
   Debug::ft("RecoveryThread.DoDelete");

   Singleton< RecoveryThread >::Destroy();
}

//------------------------------------------------------------------------------

int RecoveryThread::DoDivide()
{
   Debug::ft("RecoveryThread.DoDivide");

   int one = 1, zero = 0;
   return (one / zero);
}

//------------------------------------------------------------------------------

void RecoveryThread::DoRaise() const
{
   Debug::ft("RecoveryThread.DoRaise");

   raise(signal_);
}

//------------------------------------------------------------------------------

void RecoveryThread::DoSwErr()
{
   Debug::ft("RecoveryThread.DoSwErr");

   Debug::SwErr("software error test", 1);
}

//------------------------------------------------------------------------------

void RecoveryThread::DoTerminate()
{
   Debug::ft("RecoveryThread.DoTerminate");

   std::terminate();
}

//------------------------------------------------------------------------------

void RecoveryThread::DoTrap()
{
   Debug::ft("RecoveryThread.DoTrap");
   Raise(signal_);
}

//------------------------------------------------------------------------------

fn_name RecoveryThread_Enter = "RecoveryThread.Enter";

void RecoveryThread::Enter()
{
   while(true)
   {
      Debug::ft(RecoveryThread_Enter);

      //  Save and reset the test to be performed.  Otherwise, it will be
      //  immediately repeated upon reentering the thread after recovery.
      //
      auto test = test_;
      test_ = Sleep;

      //  Execute the requested test.
      //
      switch(test)
      {
      case Abort:
         DoAbort();
         break;
      case CtorTrap:
         Debug::SetSwFlag(ThreadCtorTrapFlag, true);
         return;
      case Delete:
         DoDelete();
         break;
      case DerefenceBadPtr:
         UseBadPointer();
         break;
      case DivideByZero:
         DoDivide();
         break;
      case DtorTrap:
         Debug::SetSwFlag(ThreadDtorTrapFlag, true);
         return;
      case InfiniteLoop:
         LoopForever();
         break;
      case MutexBlock:
         AcquireMutex();
         Pause(Duration(100, mSECS));
         RecoveryMutex_.Release();
         break;
      case MutexExit:
         AcquireMutex();
         return;
      case MutexTrap:
         AcquireMutex();
         UseBadPointer();
         break;
      case OverflowStack:
         RecurseForever(1);
         break;
      case RaiseSignal:
         DoRaise();
         break;
      case Return:
         return;
      case Sleep:
         break;
      case SwErr:
         DoSwErr();
         break;
      case Terminate:
         DoTerminate();
         break;
      case Trap:
         DoTrap();
         break;
      default:
         Debug::SwLog(RecoveryThread_Enter, "unexpected test", test);
      }

      //  Sleep until interrupted to perform the next test.  There is a timeout
      //  so that the thread will resume execution after it is deleted remotely
      //  (>recover delete f), after which it should exit.
      //
      Pause(Duration(5, SECS));
   }
}

//------------------------------------------------------------------------------

fn_name RecoveryThread_LoopForever = "RecoveryThread.LoopForever";

void RecoveryThread::LoopForever()
{
   Debug::ft(RecoveryThread_LoopForever);

   while(true)
   {
      for(auto i = 0; i < 0x1000; ++i)
      {
         for(auto j = 0; j < 0x1000; ++j);
      }

      Debug::ft(RecoveryThread_LoopForever);
   }
}

//------------------------------------------------------------------------------

bool RecoveryThread::Recover()
{
   Debug::ft("RecoveryThread.Recover");

   if(Debug::SwFlagOn(ThreadRecoverTrapFlag)) UseBadPointer();
   return Debug::SwFlagOn(ThreadReenterFlag);
}

//------------------------------------------------------------------------------

void RecoveryThread::RecurseForever(size_t depth)
{
   Debug::ft("RecoveryThread.RecurseForever");

   RecurseForever(depth + 1);
}

//------------------------------------------------------------------------------

void RecoveryThread::UseBadPointer()
{
   Debug::ft("RecoveryThread.UseBadPointer");

   CauseTrap();
}

//------------------------------------------------------------------------------
//
//  The RECOVER command, for testing the Thread safety net.
//
class DeleteText : public CliText
{
public: DeleteText();
};

class RaiseText : public CliText
{
public: RaiseText();
};

class TrapText : public CliText
{
public: TrapText();
};

class RecoverWhatParm : public CliTextParm
{
public: RecoverWhatParm();
};

class RecoverCommand : public CliCommand
{
public:
   RecoverCommand();
private:
   static RecoveryThread* EnsureThread(id_t subcommand);
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

fixed_string SignalParmExpl = "signal's name ('SIG...')";

//------------------------------------------------------------------------------

fixed_string AbortTextStr = "abort";
fixed_string AbortTextExpl = "call abort()";

//------------------------------------------------------------------------------

fixed_string BadPtrTextStr = "badptr";
fixed_string BadPtrTextExpl = "dereference an invalid pointer";

//------------------------------------------------------------------------------

fixed_string CtorTrapTextStr = "ctortrap";
fixed_string CtorTrapTextExpl = "trap in recovery thread constructor";

//------------------------------------------------------------------------------

fixed_string CreateTextStr = "create";
fixed_string CreateTextExpl = "create the recovery thread";

//------------------------------------------------------------------------------

fixed_string ThisParmExpl = "perform by 'this' (t) or by another thread (f)";
fixed_string DeleteTextStr = "delete";
fixed_string DeleteTextExpl = "delete the recovery thread";

DeleteText::DeleteText() : CliText(DeleteTextExpl, DeleteTextStr)
{
   BindParm(*new CliBoolParm(ThisParmExpl));
}

//------------------------------------------------------------------------------

fixed_string DivideTextStr = "divide";
fixed_string DivideTextExpl = "divide by zero";

//------------------------------------------------------------------------------

fixed_string DtorTrapTextStr = "dtortrap";
fixed_string DtorTrapTextExpl = "trap in recovery thread destructor";

//------------------------------------------------------------------------------

fixed_string LoopTextStr = "loop";
fixed_string LoopTextExpl = "enter an infinite loop";

//------------------------------------------------------------------------------

fixed_string MutexBlockStr = "mutexblock";
fixed_string MutexBlockExpl = "block while holding a mutex";

//------------------------------------------------------------------------------

fixed_string MutexExitStr = "mutexexit";
fixed_string MutexExitExpl = "exit while holding a mutex";

//------------------------------------------------------------------------------

fixed_string MutexTrapStr = "mutextrap";
fixed_string MutexTrapExpl = "trap while holding a mutex";

//------------------------------------------------------------------------------

fixed_string RaiseTextStr = "raise";
fixed_string RaiseTextExpl = "raise a signal";

RaiseText::RaiseText() : CliText(RaiseTextExpl, RaiseTextStr)
{
   BindParm(*new CliTextParm(SignalParmExpl, false, 0));
}

//------------------------------------------------------------------------------

fixed_string ReturnTextStr = "return";
fixed_string ReturnTextExpl = "return from the recovery thread";

//------------------------------------------------------------------------------

fixed_string StackTextStr = "stack";
fixed_string StackTextExpl = "cause a stack overflow";

//------------------------------------------------------------------------------

fixed_string SwErrTextStr = "swerr";
fixed_string SwErrTextExpl = "cause a software exception";

//------------------------------------------------------------------------------

fixed_string TerminateTextStr = "terminate";
fixed_string TerminateTextExpl = "call terminate()";

//------------------------------------------------------------------------------

fixed_string TrapTextStr = "trap";
fixed_string TrapTextExpl = "cause a trap";

TrapText::TrapText() : CliText(TrapTextExpl, TrapTextStr)
{
   BindParm(*new CliBoolParm(ThisParmExpl));
   BindParm(*new CliTextParm(SignalParmExpl, false, 0));
}

//------------------------------------------------------------------------------

fixed_string RecoverWhatExpl = "what to recover from...";

RecoverWhatParm::RecoverWhatParm() : CliTextParm(RecoverWhatExpl)
{
   BindText(*new CliText
      (CreateTextExpl, CreateTextStr), RecoveryThread::Create);
   BindText(*new CliText
      (ReturnTextExpl, ReturnTextStr), RecoveryThread::Return);
   BindText(*new CliText
      (AbortTextExpl, AbortTextStr), RecoveryThread::Abort);
   BindText(*new CliText
      (BadPtrTextExpl, BadPtrTextStr), RecoveryThread::DerefenceBadPtr);
   BindText(*new CliText
      (CtorTrapTextExpl, CtorTrapTextStr), RecoveryThread::CtorTrap);
   BindText(*new CliText
      (DivideTextExpl, DivideTextStr), RecoveryThread::DivideByZero);
   BindText(*new CliText
      (LoopTextExpl, LoopTextStr), RecoveryThread::InfiniteLoop);
   BindText(*new CliText
      (MutexBlockExpl, MutexBlockStr), RecoveryThread::MutexBlock);
   BindText(*new CliText
      (MutexExitExpl, MutexExitStr), RecoveryThread::MutexExit);
   BindText(*new CliText
      (MutexTrapExpl, MutexTrapStr), RecoveryThread::MutexTrap);
   BindText(*new RaiseText, RecoveryThread::RaiseSignal);
   BindText(*new CliText
      (SwErrTextExpl, SwErrTextStr), RecoveryThread::SwErr);
   BindText(*new CliText
      (TerminateTextExpl, TerminateTextStr), RecoveryThread::Terminate);
   BindText(*new TrapText, RecoveryThread::Trap);
   BindText(*new CliText
      (StackTextExpl, StackTextStr), RecoveryThread::OverflowStack);
   BindText(*new DeleteText, RecoveryThread::Delete);
   BindText(*new CliText
      (DtorTrapTextExpl, DtorTrapTextStr), RecoveryThread::DtorTrap);
}

//------------------------------------------------------------------------------

fixed_string RecoverStr = "recover";
fixed_string RecoverExpl = "Tests thread recovery.";

RecoverCommand::RecoverCommand() : CliCommand(RecoverStr, RecoverExpl)
{
   BindParm(*new RecoverWhatParm);
}

fn_name RecoverCommand_EnsureThread = "RecoverCommand.EnsureThread";

RecoveryThread* RecoverCommand::EnsureThread(id_t subcommand)
{
   Debug::ft(RecoverCommand_EnsureThread);

   auto thr = Singleton< RecoveryThread >::Extant();
   if(thr != nullptr) return thr;

   thr = Singleton< RecoveryThread >::Instance();
   if(subcommand == RecoveryThread::Create) return thr;

   Debug::SwLog(RecoverCommand_EnsureThread, "recovery thread created", 0);
   return thr;
}

fn_name RecoverCommand_ProcessCommand = "RecoverCommand.ProcessCommand";

word RecoverCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(RecoverCommand_ProcessCommand);

   id_t index;
   bool flag;
   string signame;
   signal_t signal;
   PosixSignal* ps;

   if(!Element::RunningInLab()) return cli.Report(-5, NotInFieldExpl);
   if(!GetTextIndex(index, cli)) return -1;

   auto thr = EnsureThread(index);
   auto test = RecoveryThread::Test(index);
   auto reg = Singleton< PosixSignalRegistry >::Instance();

   switch(index)
   {
   case RecoveryThread::Create:
      break;

   case RecoveryThread::Abort:
   case RecoveryThread::CtorTrap:
   case RecoveryThread::DtorTrap:
   case RecoveryThread::DerefenceBadPtr:
   case RecoveryThread::DivideByZero:
   case RecoveryThread::InfiniteLoop:
   case RecoveryThread::MutexBlock:
   case RecoveryThread::MutexExit:
   case RecoveryThread::MutexTrap:
   case RecoveryThread::OverflowStack:
   case RecoveryThread::Return:
   case RecoveryThread::SwErr:
   case RecoveryThread::Terminate:
      if(!cli.EndOfInput()) return -1;
      thr->SetTest(test);
      thr->Interrupt();
      break;

   case RecoveryThread::Delete:
      if(!GetBoolParm(flag, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      if(flag)
      {
         thr->SetTest(test);
         thr->Interrupt();
      }
      else
      {
         Singleton< RecoveryThread >::Destroy();
      }
      break;

   case RecoveryThread::RaiseSignal:
      if(!GetString(signame, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      signal = reg->Value(signame);
      if(signal == SIGNIL) return cli.Report(-3, UnknownSignalExpl);
      thr->SetTest(test);
      thr->SetTestSignal(signal);
      thr->Interrupt();
      break;

   case RecoveryThread::Trap:
      if(!GetBoolParm(flag, cli)) return -1;
      if(!GetString(signame, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      ps = reg->Find(signame);
      if(ps == nullptr) return cli.Report(-3, UnknownSignalExpl);
      if(flag)
      {
         thr->SetTest(test);
         thr->SetTestSignal(ps->Value());
         thr->Interrupt();
      }
      else
      {
         thr->Raise(ps->Value());
      }
      break;

   default:
      Debug::SwLog(RecoverCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return cli.Report(0, SuccessExpl);
}

//==============================================================================
//
//  The NodeBase tools and test increment.
//
fixed_string NtStr = "nt";
fixed_string NtExpl = "NodeBase Tools and Tests";

NtIncrement::NtIncrement() : CliIncrement(NtStr, NtExpl)
{
   Debug::ft("NtIncrement.ctor");

   BindCommand(*new NtLogsCommand);
   BindCommand(*new NtSetCommand);
   BindCommand(*new NtSaveCommand);
   BindCommand(*new TestsCommand);
   BindCommand(*new SwFlagsCommand);
   BindCommand(*new CorruptCommand);
   BindCommand(*new LeakyBucketCounterCommands);
   BindCommand(*new Q1WayCommands);
   BindCommand(*new Q2WayCommands);
   BindCommand(*new RegistryCommands);
   BindCommand(*new SysTimeCommands);
   BindCommand(*new HeapCommands);
   BindCommand(*new RecoverCommand);
}

//------------------------------------------------------------------------------

NtIncrement::~NtIncrement()
{
   Debug::ftnt("NtIncrement.dtor");
}
}
