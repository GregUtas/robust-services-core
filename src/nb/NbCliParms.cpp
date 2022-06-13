//==============================================================================
//
//  NbCliParms.cpp
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
#include "NbCliParms.h"
#include <cstddef>
#include <cstdint>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Log.h"
#include "LogBufferRegistry.h"
#include "Module.h"
#include "NbTypes.h"
#include "ObjectPool.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string AllocationError      = "Failed to allocate resources.";
fixed_string AlreadyInIncrement   = "Already in ";
fixed_string BadObjectPtrWarning  = "If the pointer is invalid, this will trap.";
fixed_string BadParameterValue    = "Rejected. Value must be ";
fixed_string CommandAbortedExpl   = "Command aborted.";
fixed_string ConsoleAutomaticExpl = "Reading from console is automatic at end of file.";
fixed_string ContinuePrompt       = "Do you wish to continue?";
fixed_string CreateStreamFailure  = "Could not create output stream.";
fixed_string DelayFailure         = "Delay failed: rc=";
fixed_string EmptySet             = "No elements in set.";
fixed_string EndOfFreeQueue       = "Failed: reached end of pool's free queue.";
fixed_string NoAlarmExpl          = "There is no alarm with that identifier.";
fixed_string NoBuffersExpl        = "There were no buffers to display.";
fixed_string NoCfgParmExpl        = "No such configuration parameter.";
fixed_string NoCommandExpl        = "No such command: ";
fixed_string NoDaemonExpl         = "There is no daemon at that index.";
fixed_string NoDiscardsExpl       = "There were no discards to display.";
fixed_string NoFileExpl           = "File does not exist or is empty.";
fixed_string NoIncrExpl           = "Nothing to quit.";
fixed_string NoLogExpl            = "There is no log with that identifier.";
fixed_string NoLogGroupExpl       = "There is no log group with that identifier.";
fixed_string NoModuleExpl         = "There is no module with that identifier.";
fixed_string NoMutexExpl          = "There is no mutex at that index.";
fixed_string NoPoolExpl           = "There is no object pool with that identifier.";
fixed_string NoPosixSignalExpl    = "There is no POSIX signal at that index.";
fixed_string NoStatsGroupExpl     = "There is no statistics group with that identifier.";
fixed_string NoSymbolExpl         = "There is no symbol with that name.";
fixed_string NoThreadExpl         = "There is no thread with that identifier.";
fixed_string NoToolExpl           = "There is no tool with that identifier.";
fixed_string NotImplementedExpl   = "This command is not yet implemented.";
fixed_string NotInFieldExpl       = "This command is not allowed in the field.";
fixed_string NullPtrInvalid       = "Invalid nullptr argument.";
fixed_string ParameterIgnored     = "Parameter ignored: ";
fixed_string ParameterInvalid     = "Parameter invalid: ";
fixed_string RestartWarning       = "This will SHUT DOWN or RESTART this element.";
fixed_string ReturnFalse          = "Result is false.";
fixed_string ReturnTrue           = "Result is true.";
fixed_string SendingToConsoleExpl = "Already sending to the console.";
fixed_string SizesHeader          = "CLASS and STRUCT sizes (in bytes):";
fixed_string StopTracingPrompt    = "Tracing is on. Stop tracing first?";
fixed_string SuccessExpl          = "OK.";
fixed_string SymbolLockedExpl     = "This symbol's value cannot be changed.";
fixed_string SymbolOverflowExpl   = "Must undefine some symbols before defining more.";
fixed_string SystemErrorExpl      = "Unexpected system error.";
fixed_string TestFailedExpl       = "Test failed";
fixed_string TooManyInputStreams  = "Exceeded nesting level of >read command.";
fixed_string TooManyOutputStreams = "Exceeded nesting level of >send command.";
fixed_string TraceReportPrompt    = "Trace generation of the trace report itself?";
fixed_string UnknownSignalExpl    = "This platform does not support that signal.";

//------------------------------------------------------------------------------

fixed_string AllActivityTextStr = "all";
fixed_string AllActivityTextExpl = "all activity";

AllActivityText::AllActivityText() :
   CliText(AllActivityTextExpl, AllActivityTextStr)
{
   BindParm(*new SetHowParm);
}

//------------------------------------------------------------------------------

fixed_string BufferTextStr = "buffer";
fixed_string BufferTextExpl = "trace buffer";

BufferText::BufferText() : CliText(BufferTextExpl, BufferTextStr) { }

//------------------------------------------------------------------------------

fixed_string DispBVStr = "bv";
fixed_string DispBVExpl = "'b'=brief 'v'=verbose (default='b')";

DispBVParm::DispBVParm() : CliCharParm(DispBVExpl, DispBVStr, true) { }

//------------------------------------------------------------------------------

fixed_string DispCSVStr = "csv";
fixed_string DispCSVExpl = "'c'=count 's'=summary 'v'=verbose (default='s')";

DispCSVParm::DispCSVParm() : CliCharParm(DispCSVExpl, DispCSVStr, true) { }

//------------------------------------------------------------------------------

fixed_string DispCSBVStr = "csbv";
fixed_string DispCSBVExpl =
   "'c'=count 's'=summary 'b'=brief 'v'=verbose (default='s')";

DispCSBVParm::DispCSBVParm() : CliCharParm(DispCSBVExpl, DispCSBVStr, true) { }

//------------------------------------------------------------------------------

word ExplainTraceRc(const CliThread& cli, TraceRc rc)
{
   auto result = (rc == TraceOk ? 0 : -1);
   return cli.Report(result, strTraceRc(rc));
}

//------------------------------------------------------------------------------

bool GetIdDispS(const CliCommand& comm, CliThread& cli, word& id, char& disp)
{
   Debug::ft("NodeBase.GetIdDispS");

   switch(comm.GetIntParmRc(id, cli))
   {
   case CliParm::None:
      id = NIL_ID;
      break;
   case CliParm::Ok:
      disp = 's';
      break;
   default:
      return false;
   }

   switch(comm.GetCharParmRc(disp, cli))
   {
   case CliParm::None:
      disp = 's';
      break;
   case CliParm::Ok:
      break;
   default:
      return false;
   }

   return true;
}

//------------------------------------------------------------------------------

bool GetIdDispV(const CliCommand& comm, CliThread& cli, word& id, char& disp)
{
   Debug::ft("NodeBase.GetIdDispV");

   switch(comm.GetIntParmRc(id, cli))
   {
   case CliParm::None:
      id = NIL_ID;
      break;
   case CliParm::Ok:
      disp = 'v';
      break;
   default:
      return false;
   }

   switch(comm.GetCharParmRc(disp, cli))
   {
   case CliParm::None:
      if(id == NIL_ID) disp = 's';  // 'v' if identifier specified above
      break;
   case CliParm::Ok:
      break;
   default:
      return false;
   }

   return true;
}

//------------------------------------------------------------------------------

bool ValidateOptions(const string& opts, const string& valid, string& expl)
{
   Debug::ft("NodeBase.ValidateOptions");

   string invalid;

   for(size_t i = 0; i < opts.size(); ++i)
   {
      if(valid.find(opts[i]) == string::npos)
      {
         invalid.push_back(opts[i]);
      }
   }

   if(invalid.empty()) return true;

   expl = "Invalid options: ";
   expl += invalid;
   return false;
}

//------------------------------------------------------------------------------

fixed_string FactionMandExpl = "faction";

FactionMandParm::FactionMandParm() :
   CliIntParm(FactionMandExpl, 0, Faction_N - 1) { }

//------------------------------------------------------------------------------

fixed_string FactionOptExpl = "faction (default=all)";

FactionOptParm::FactionOptParm() :
   CliIntParm(FactionOptExpl, 0, Faction_N - 1, true) { }

//------------------------------------------------------------------------------

fixed_string FactionTextStr = "faction";
fixed_string FactionTextExpl = "threads in a specific faction";

FactionText::FactionText() : CliText(FactionTextExpl, FactionTextStr)
{
   BindParm(*new FactionMandParm);
}

//------------------------------------------------------------------------------

fixed_string FactionsTextStr = "factions";
fixed_string FactionsTextExpl = "all included/excluded factions";

FactionsText::FactionsText() : CliText(FactionsTextExpl, FactionsTextStr) { }

//------------------------------------------------------------------------------

fixed_string IdExpl = "index (location in registry)";

IdMandParm::IdMandParm() : CliIntParm(IdExpl, 0, UINT16_MAX) { }

IdOptParm::IdOptParm() : CliIntParm(IdExpl, 0, UINT16_MAX, true) { }

//------------------------------------------------------------------------------

fixed_string IstreamMandExpl = "filename for input (in OutputPath directory)";

IstreamMandParm::IstreamMandParm() : CliTextParm(IstreamMandExpl, false, 0) { }

//------------------------------------------------------------------------------

fixed_string LogBufferIdExpl = "log buffer index";

LogBufferIdParm::LogBufferIdParm() :
   CliIntParm(LogBufferIdExpl, 0, LogBufferRegistry::MaxBuffers - 1) { }

//------------------------------------------------------------------------------

fixed_string LogGroupMandExpl = "log group name";

LogGroupMandParm::LogGroupMandParm() :
   CliTextParm(LogGroupMandExpl, false, 0) { }

fixed_string LogGroupOptExpl = "log group name (default=all)";

LogGroupOptParm::LogGroupOptParm() : CliTextParm(LogGroupOptExpl, true, 0) { }

//------------------------------------------------------------------------------

fixed_string LogIdMandExpl = "log number";

LogIdMandParm::LogIdMandParm() :
   CliIntParm(LogIdMandExpl, TroubleLog, Log::MaxId) { }

//------------------------------------------------------------------------------

fixed_string MemoryTypeMandExpl = "memory type (see mem.* symbols)";

MemoryTypeMandParm::MemoryTypeMandParm() :
   CliIntParm(MemoryTypeMandExpl, MemTemporary, MemImmutable) { }

//------------------------------------------------------------------------------

fixed_string MemoryTypeOptExpl = "memory type (see mem.* symbols; default=all)";

MemoryTypeOptParm::MemoryTypeOptParm() :
   CliIntParm(MemoryTypeOptExpl, MemNull, MemImmutable, true) { }

//------------------------------------------------------------------------------

fixed_string ModuleIdOptExpl = "ModuleId (default=all)";

ModuleIdOptParm::ModuleIdOptParm() :
   CliIntParm(ModuleIdOptExpl, 0, Module::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string ObjPoolIdMandExpl = "ObjectPoolId";

ObjPoolIdMandParm::ObjPoolIdMandParm() :
   CliIntParm(ObjPoolIdMandExpl, 0, ObjectPool::MaxId) { }

fixed_string ObjPoolIdOptExpl = "ObjectPoolId (default=all)";

ObjPoolIdOptParm::ObjPoolIdOptParm() :
   CliIntParm(ObjPoolIdOptExpl, 0, ObjectPool::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string OstreamMandExpl = "filename for output";

OstreamMandParm::OstreamMandParm() : CliTextParm(OstreamMandExpl, false, 0) { }

fixed_string OstreamOptExpl = "filename for output (default=console)";

OstreamOptParm::OstreamOptParm() : CliTextParm(OstreamOptExpl, true, 0) { }

//------------------------------------------------------------------------------

fixed_string SelectionsTextStr = "selections";
fixed_string SelectionsTextExpl = "all items included/excluded by trace tools";

SelectionsText::SelectionsText() :
   CliText(SelectionsTextExpl, SelectionsTextStr) { }

//------------------------------------------------------------------------------

fixed_string OnTextStr = "on";
fixed_string OnTextExpl = "on";

fixed_string OffTextStr = "off";
fixed_string OffTextExpl = "off";

fixed_string SetHowExpl = "setting...";

SetHowParm::SetHowParm() : CliTextParm(SetHowExpl)
{
   BindText(*new CliText(OnTextExpl, OnTextStr), On);
   BindText(*new CliText(OffTextExpl, OffTextStr), Off);
}

//------------------------------------------------------------------------------

fixed_string PosixSignalExpl = "signal_t (default=all)";

PosixSignalParm::PosixSignalParm() :
   CliIntParm(PosixSignalExpl, 0, 127, true) { }

//------------------------------------------------------------------------------

fixed_string EqTextStr = "==";
fixed_string EqTextExpl = "equal to";

fixed_string NEqTextStr = "!=";
fixed_string NEqTextExpl = "not equal to";

fixed_string LtTextStr = "<";
fixed_string LtTextExpl = "less than";

fixed_string LtEqTextStr = "<=";
fixed_string LtEqTextExpl = "less than or equal to";

fixed_string GtTextStr = ">";
fixed_string GtTextExpl = "greater than";

fixed_string GtEqTextStr = ">=";
fixed_string GtEqTextExpl = "greater than or equal to";

fixed_string RelationParmExpl = "relational operator...";

RelationParm::RelationParm() : CliTextParm(RelationParmExpl)
{
   BindText(*new CliText(EqTextExpl, EqTextStr), Eq);
   BindText(*new CliText(NEqTextExpl, NEqTextStr), NEq);
   BindText(*new CliText(LtTextExpl, LtTextStr), Lt);
   BindText(*new CliText(LtEqTextExpl, LtEqTextStr), LtEq);
   BindText(*new CliText(GtTextExpl, GtTextStr), Gt);
   BindText(*new CliText(GtEqTextExpl, GtEqTextStr), GtEq);
}

//------------------------------------------------------------------------------

fixed_string ThreadIdMandExpl = "ThreadId";

ThreadIdMandParm::ThreadIdMandParm() :
   CliIntParm(ThreadIdMandExpl, 0, Thread::MaxId) { }

fixed_string ThreadIdOptExpl = "ThreadId (default=all)";

ThreadIdOptParm::ThreadIdOptParm() :
   CliIntParm(ThreadIdOptExpl, 0, Thread::MaxId, true) { }

//------------------------------------------------------------------------------

fixed_string ThreadTextStr = "thread";
fixed_string ThreadTextExpl = "a specific thread's activity";

ThreadText::ThreadText() : CliText(ThreadTextExpl, ThreadTextStr)
{
   BindParm(*new ThreadIdMandParm);
}

//------------------------------------------------------------------------------

fixed_string ThreadsTextStr = "threads";
fixed_string ThreadsTextExpl = "all included/excluded threads";

ThreadsText::ThreadsText() : CliText(ThreadsTextExpl, ThreadsTextStr) { }

//------------------------------------------------------------------------------

fixed_string ToolsTextStr = "tools";
fixed_string ToolsTextExpl = "trace tools";

ToolsText::ToolsText() : CliText(ToolsTextExpl, ToolsTextStr) { }
}
