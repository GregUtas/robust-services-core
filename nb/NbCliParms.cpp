//==============================================================================
//
//  NbCliParms.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "Duration.h"
#include "Log.h"
#include "LogBufferRegistry.h"
#include "Module.h"
#include "NbTypes.h"
#include "ObjectPool.h"
#include "SysTime.h"

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
fixed_string StopTracingPrompt    = "Tracing is on.  Stop tracing first?";
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

fixed_string DispCBVStr = "cbv";
fixed_string DispCBVExpl = "'c'=count 'b'=brief 'v'=verbose (default='b')";

DispCBVParm::DispCBVParm() : CliCharParm(DispCBVExpl, DispCBVStr, true) { }

//------------------------------------------------------------------------------

word ExplainTraceRc(const CliThread& cli, TraceRc rc)
{
   auto result = (rc == TraceOk ? 0 : -1);
   return cli.Report(result, strTraceRc(rc));
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

CliParm::Rc GetBV(const CliCommand& comm, CliThread& cli, bool& v)
{
   Debug::ft("NodeBase.GetBV");

   char c;
   auto rc = comm.GetCharParmRc(c, cli);
   v = (c == 'v');
   return rc;
}

//------------------------------------------------------------------------------

CliParm::Rc GetCBV(const CliCommand& comm, CliThread& cli, bool& c, bool& v)
{
   Debug::ft("NodeBase.GetCBV");

   char k;

   c = false;
   v = false;

   auto rc = comm.GetCharParmRc(k, cli);

   if(rc == CliParm::Ok)
   {
      if(k == 'c') c = true;
      if(k == 'v') v = true;
   }

   return rc;
}

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

fixed_string MemoryTypeExpl = "memory type (see mem.* symbols)";

MemoryTypeParm::MemoryTypeParm() :
   CliIntParm(MemoryTypeExpl, MemTemporary, MemImmutable) { }

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

class OnText : public CliText
{
public: OnText();
};

class OffText : public CliText
{
public: OffText();
};

fixed_string OnTextStr = "on";
fixed_string OnTextExpl = "on";

OnText::OnText() : CliText(OnTextExpl, OnTextStr) { }

fixed_string OffTextStr = "off";
fixed_string OffTextExpl = "off";

OffText::OffText() : CliText(OffTextExpl, OffTextStr) { }

fixed_string SetHowExpl = "setting...";

SetHowParm::SetHowParm() : CliTextParm(SetHowExpl)
{
   BindText(*new OnText, On);
   BindText(*new OffText, Off);
}

//------------------------------------------------------------------------------

class NEqText : public CliText
{
public: NEqText();
};

class LtText : public CliText
{
public: LtText();
};

class LtEqText : public CliText
{
public: LtEqText();
};

class EqText : public CliText
{
public: EqText();
};

class GtEqText : public CliText
{
public: GtEqText();
};

class GtText : public CliText
{
public: GtText();
};

fixed_string EqTextStr = "==";
fixed_string EqTextExpl = "equal to";

EqText::EqText() : CliText(EqTextExpl, EqTextStr) { }

fixed_string NEqTextStr = "!=";
fixed_string NEqTextExpl = "not equal to";

NEqText::NEqText() : CliText(NEqTextExpl, NEqTextStr) { }

fixed_string LtTextStr = "<";
fixed_string LtTextExpl = "less than";

LtText::LtText() : CliText(LtTextExpl, LtTextStr) { }

fixed_string LtEqTextStr = "<=";
fixed_string LtEqTextExpl = "less than or equal to";

LtEqText::LtEqText() : CliText(LtEqTextExpl, LtEqTextStr) { }

fixed_string GtTextStr = ">";
fixed_string GtTextExpl = "greater than";

GtText::GtText() : CliText(GtTextExpl, GtTextStr) { }

fixed_string GtEqTextStr = ">=";
fixed_string GtEqTextExpl = "greater than or equal to";

GtEqText::GtEqText() : CliText(GtEqTextExpl, GtEqTextStr) { }

fixed_string RelationParmExpl = "relational operator...";

RelationParm::RelationParm() : CliTextParm(RelationParmExpl)
{
   BindText(*new EqText, Eq);
   BindText(*new NEqText, NEq);
   BindText(*new LtText, Lt);
   BindText(*new LtEqText, LtEq);
   BindText(*new GtText, Gt);
   BindText(*new GtEqText, GtEq);
}

//------------------------------------------------------------------------------

fixed_string SysTimeDayExpl = "day of month";

SysTimeDayParm::SysTimeDayParm() :
   CliIntParm(SysTimeDayExpl, 1, SysTime::MaxDay) { }

//------------------------------------------------------------------------------

class SysTimeYearText : public CliText
{
public: SysTimeYearText();
};

class SysTimeMonthText : public CliText
{
public: SysTimeMonthText();
};

class SysTimeDayText : public CliText
{
public: SysTimeDayText();
};

class SysTimeHourText : public CliText
{
public: SysTimeHourText();
};

class SysTimeMinText : public CliText
{
public: SysTimeMinText();
};

class SysTimeSecText : public CliText
{
public: SysTimeSecText();
};

class SysTimeMsecText : public CliText
{
public: SysTimeMsecText();
};

fixed_string SysTimeYearTextStr = "year";
fixed_string SysTimeYearTextExpl = "year field";

SysTimeYearText::SysTimeYearText() :
   CliText(SysTimeYearTextExpl, SysTimeYearTextStr) { }

fixed_string SysTimeMonthTextStr = "month";
fixed_string SysTimeMonthTextExpl = "month field";

SysTimeMonthText::SysTimeMonthText() :
   CliText(SysTimeMonthTextExpl, SysTimeMonthTextStr) { }

fixed_string SysTimeDayTextStr = "day";
fixed_string SysTimeDayTextExpl = "day field";

SysTimeDayText::SysTimeDayText() :
   CliText(SysTimeDayTextExpl, SysTimeDayTextStr) { }

fixed_string SysTimeHourTextStr = "hour";
fixed_string SysTimeHourTextExpl = "hours field";

SysTimeHourText::SysTimeHourText() :
   CliText(SysTimeHourTextExpl, SysTimeHourTextStr) { }

fixed_string SysTimeMinTextStr = "min";
fixed_string SysTimeMinTextExpl = "minutes field";

SysTimeMinText::SysTimeMinText() :
   CliText(SysTimeMinTextExpl, SysTimeMinTextStr) { }

fixed_string SysTimeSecTextStr = "sec";
fixed_string SysTimeSecTextExpl = "seconds field";

SysTimeSecText::SysTimeSecText() :
   CliText(SysTimeSecTextExpl, SysTimeSecTextStr) { }

fixed_string SysTimeMsecTextStr = "msec";
fixed_string SysTimeMsecTextExpl = "milliseconds field";

SysTimeMsecText::SysTimeMsecText() :
   CliText(SysTimeMsecTextExpl, SysTimeMsecTextStr) { }

fixed_string SysTimeFieldExpl = "time field";

SysTimeFieldParm::SysTimeFieldParm() : CliTextParm(SysTimeFieldExpl)
{
   BindText(*new SysTimeYearText, YearsField + 1);
   BindText(*new SysTimeMonthText, MonthsField + 1);
   BindText(*new SysTimeDayText, DaysField + 1);
   BindText(*new SysTimeHourText, HoursField + 1);
   BindText(*new SysTimeMinText, MinsField + 1);
   BindText(*new SysTimeSecText, SecsField + 1);
   BindText(*new SysTimeMsecText, MsecsField + 1);
}

//------------------------------------------------------------------------------

fixed_string SysTimeHourExpl = "hours (24-hour clock)";

SysTimeHourParm::SysTimeHourParm() :
   CliIntParm(SysTimeHourExpl, 0, SysTime::MaxHour) { }

//------------------------------------------------------------------------------

fixed_string SysTimeMinuteExpl = "minutes";

SysTimeMinuteParm::SysTimeMinuteParm() :
   CliIntParm(SysTimeMinuteExpl, 0, SysTime::MaxMin) { }

//------------------------------------------------------------------------------

fixed_string SysTimeMonthExpl = "month (Jan=1, Dec=12)";

SysTimeMonthParm::SysTimeMonthParm() :
   CliIntParm(SysTimeMonthExpl, 1, SysTime::MaxMonth + 1) { }

//------------------------------------------------------------------------------

fixed_string SysTimeMsecondExpl = "milliseconds";

SysTimeMsecondParm::SysTimeMsecondParm() :
   CliIntParm(SysTimeMsecondExpl, 0, SysTime::MaxMsec) { }

//------------------------------------------------------------------------------

fixed_string SysTimeSecondExpl = "seconds";

SysTimeSecondParm::SysTimeSecondParm() :
   CliIntParm(SysTimeSecondExpl, 0, SysTime::MaxSec) { }

//------------------------------------------------------------------------------

fixed_string SysTimeYearExpl = "year";

SysTimeYearParm::SysTimeYearParm() :
   CliIntParm(SysTimeYearExpl, SysTime::MinYear, SysTime::MaxYear) { }

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
}
