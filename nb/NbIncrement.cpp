//==============================================================================
//
//  NbIncrement.cpp
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
#include "NbIncrement.h"
#include "CliText.h"
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <sstream>
#include <string>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "CallbackRequest.h"
#include "CfgParm.h"
#include "CfgParmRegistry.h"
#include "CliBoolParm.h"
#include "CliBuffer.h"
#include "CliPtrParm.h"
#include "CliRegistry.h"
#include "CliStack.h"
#include "CliThread.h"
#include "Daemon.h"
#include "DaemonRegistry.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "FileThread.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Heap.h"
#include "Log.h"
#include "LogBuffer.h"
#include "LogBufferRegistry.h"
#include "LogGroup.h"
#include "LogGroupRegistry.h"
#include "Memory.h"
#include "Module.h"
#include "ModuleRegistry.h"
#include "MutexRegistry.h"
#include "NbCliParms.h"
#include "NbPools.h"
#include "NbTracer.h"
#include "ObjectPoolAudit.h"
#include "ObjectPoolRegistry.h"
#include "PosixSignal.h"
#include "PosixSignalRegistry.h"
#include "Q1Way.h"
#include "Registry.h"
#include "Restart.h"
#include "Singleton.h"
#include "Singletons.h"
#include "Statistics.h"
#include "StatisticsGroup.h"
#include "StatisticsRegistry.h"
#include "Symbol.h"
#include "SymbolRegistry.h"
#include "SysMutex.h"
#include "ThisThread.h"
#include "ThreadRegistry.h"
#include "Tool.h"
#include "ToolRegistry.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The ALARMS command.
//
fixed_string AlarmExpl = "alarm name";

fixed_string AlarmsListTextStr = "list";
fixed_string AlarmsListTextExpl = "lists alarms";

class AlarmsExplainText : public CliText
{
public: AlarmsExplainText();
};

fixed_string AlarmsExplainTextStr = "explain";
fixed_string AlarmsExplainTextExpl = "displays documentation for an alarm";

AlarmsExplainText::AlarmsExplainText() :
   CliText(AlarmsExplainTextExpl, AlarmsExplainTextStr)
{
   BindParm(*new CliTextParm(AlarmExpl, false, 0));
}

class AlarmsClearText : public CliText
{
public: AlarmsClearText();
};

fixed_string AlarmsClearTextStr = "clear";
fixed_string AlarmsClearTextExpl = "clears an alarm";

AlarmsClearText::AlarmsClearText() :
   CliText(AlarmsClearTextExpl, AlarmsClearTextStr)
{
   BindParm(*new CliTextParm(AlarmExpl, false, 0));
}

class AlarmsAction : public CliTextParm
{
public:
   AlarmsAction();
};

constexpr id_t AlarmsListIndex = 1;
constexpr id_t AlarmsExplainIndex = 2;
constexpr id_t AlarmsClearIndex = 3;

fixed_string AlarmsActionExpl = "subcommand...";

AlarmsAction::AlarmsAction() : CliTextParm(AlarmsActionExpl)
{
   BindText(*new CliText
      (AlarmsListTextExpl, AlarmsListTextStr), AlarmsListIndex);
   BindText(*new AlarmsExplainText, AlarmsExplainIndex);
   BindText(*new AlarmsClearText, AlarmsClearIndex);
}

class AlarmsCommand : public CliCommand
{
public:
   AlarmsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string AlarmsStr = "alarms";
fixed_string AlarmsExpl = "Interface to the alarm subsystem.";

AlarmsCommand::AlarmsCommand() : CliCommand(AlarmsStr, AlarmsExpl)
{
   BindParm(*new AlarmsAction);
}

fn_name AlarmsCommand_ProcessCommand = "AlarmsCommand.ProcessCommand";

word AlarmsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(AlarmsCommand_ProcessCommand);

   word rc = 0;
   id_t index;
   string name, key, path;
   Alarm* alarm;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case AlarmsListIndex:
      if(!cli.EndOfInput()) return -1;
      Singleton< AlarmRegistry >::Instance()->Output(*cli.obuf, 2, false);
      break;

   case AlarmsExplainIndex:
      if(!GetString(name, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      alarm = Singleton< AlarmRegistry >::Instance()->Find(name);

      if(alarm == nullptr)
      {
         return cli.Report(-1, NoAlarmExpl);
      }

      key = alarm->Name();
      path = Element::HelpPath() + PATH_SEPARATOR + "alarms.txt";
      rc = cli.DisplayHelp(path, key);

      switch(rc)
      {
      case -1:
         return cli.Report(-1, "This alarm has not been documented.");
      case -2:
         return cli.Report(-2, "Failed to open file " + path);
      }

      break;

   case AlarmsClearIndex:
      if(!GetString(name, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      alarm = Singleton< AlarmRegistry >::Instance()->Find(name);

      if(alarm == nullptr)
      {
         return cli.Report(-1, NoAlarmExpl);
      }

      alarm->SetStatus(NoAlarm);
      return cli.Report(0, SuccessExpl);

   default:
      Debug::SwLog(AlarmsCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return rc;
}

//------------------------------------------------------------------------------
//
//  The AUDIT command.
//
class AuditIntervalText : public CliText
{
public: AuditIntervalText();
};

class AuditAction : public CliTextParm
{
public: AuditAction();
};

class AuditCommand : public CliCommand
{
public:
   AuditCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string AuditSecondsExpl = "seconds between audits (0 = disabled)";

fixed_string AuditIntervalStr = "interval";
fixed_string AuditIntervalExpl = "sets the audit's frequency";

AuditIntervalText::AuditIntervalText() :
   CliText(AuditIntervalExpl, AuditIntervalStr)
{
   BindParm(*new CliIntParm(AuditSecondsExpl, 0, 60));
}

fixed_string AuditForceStr = "force";
fixed_string AuditForceExpl = "forces the audit to run immediately";

constexpr id_t AuditIntervalIndex = 1;
constexpr id_t AuditForceIndex = 2;

fixed_string AuditActionExpl = "subcommand...";

AuditAction::AuditAction() : CliTextParm(AuditActionExpl)
{
   BindText(*new AuditIntervalText, AuditIntervalIndex);
   BindText(*new CliText(AuditForceExpl, AuditForceStr), AuditForceIndex);
}

fixed_string AuditStr = "audit";
fixed_string AuditExpl = "Controls the object pool audit.";

AuditCommand::AuditCommand() : CliCommand(AuditStr, AuditExpl)
{
   BindParm(*new AuditAction);
}

fn_name AuditCommand_ProcessCommand = "AuditCommand.ProcessCommand";

word AuditCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(AuditCommand_ProcessCommand);

   id_t index;
   word secs;
   Duration timeout;

   if(!GetTextIndex(index, cli)) return -1;

   auto thr = Singleton< ObjectPoolAudit >::Instance();

   switch(index)
   {
   case AuditIntervalIndex:
      //
      //  A value of zero disables the audit.  Other values must be
      //  converted to milliseconds.
      //
      if(!GetIntParm(secs, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      timeout = (secs == 0 ? TIMEOUT_NEVER : Duration(secs, SECS));
      thr->SetInterval(timeout);
      break;
   case AuditForceIndex:
      //
      //  Wake the audit without otherwise changing its interval.
      //
      if(!cli.EndOfInput()) return -1;
      thr->Interrupt();
      break;
   default:
      Debug::SwLog(AuditCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The BUFFERS command.
//
class BuffersCommand : public CliCommand
{
public:
   BuffersCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string BuffersStr = "buffers";
fixed_string BuffersExpl = "Counts or displays message buffers.";

BuffersCommand::BuffersCommand() : CliCommand(BuffersStr, BuffersExpl)
{
   BindParm(*new DispCBVParm);
}

word BuffersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("BuffersCommand.ProcessCommand");

   bool c, v;

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< MsgBufferPool >::Instance();
   auto num = pool->InUseCount();
   auto opts = (v ? VerboseOpt : NoFlags);

   if(c)
      *cli.obuf << spaces(2) << num << CRLF;
   else if(!pool->DisplayUsed(*cli.obuf, spaces(2), opts))
      return cli.Report(-2, NoBuffersExpl);

   return num;
}

//------------------------------------------------------------------------------
//
//  The CFGPARMS command.
//
class CfgParmsExplText : public CliText
{
public: CfgParmsExplText();
};

class CfgParmsGetText : public CliText
{
public: CfgParmsGetText();
};

class CfgParmsSetText : public CliText
{
public: CfgParmsSetText();
};

class CfgParmsAction : public CliTextParm
{
public: CfgParmsAction();
};

class CfgParmsCommand : public CliCommand
{
public:
   CfgParmsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CfgParmNameExpl = "name of configuration parameter";

fixed_string CfgParmValueExpl = "value of configuration parameter";

fixed_string CfgParmsListStr = "list";
fixed_string CfgParmsListExpl = "lists all configuration parameters";

fixed_string CfgParmsExplStr = "explain";
fixed_string CfgParmsExplExpl = "explains a configuration parameter";

CfgParmsExplText::CfgParmsExplText() :
   CliText(CfgParmsExplExpl, CfgParmsExplStr)
{
   BindParm(*new CliTextParm(CfgParmNameExpl, false, 0));
}

fixed_string CfgParmsGetStr = "get";
fixed_string CfgParmsGetExpl = "returns a configuration parameter's value";

CfgParmsGetText::CfgParmsGetText() : CliText(CfgParmsGetExpl, CfgParmsGetStr)
{
   BindParm(*new CliTextParm(CfgParmNameExpl, false, 0));
}

fixed_string CfgParmsSetStr = "set";
fixed_string CfgParmsSetExpl = "sets a configuration parameter's value";

CfgParmsSetText::CfgParmsSetText() : CliText(CfgParmsSetExpl, CfgParmsSetStr)
{
   BindParm(*new CliTextParm(CfgParmNameExpl, false, 0));
   BindParm(*new CliTextParm(CfgParmValueExpl, false, 0));
}

constexpr id_t CfgParmsListIndex = 1;
constexpr id_t CfgParmsExplIndex = 2;
constexpr id_t CfgParmsGetIndex = 3;
constexpr id_t CfgParmsSetIndex = 4;

fixed_string CfgParmsActionExpl = "subcommand...";

CfgParmsAction::CfgParmsAction() : CliTextParm(CfgParmsActionExpl)
{
   BindText(*new CliText(CfgParmsListExpl, CfgParmsListStr), CfgParmsListIndex);
   BindText(*new CfgParmsExplText, CfgParmsExplIndex);
   BindText(*new CfgParmsGetText, CfgParmsGetIndex);
   BindText(*new CfgParmsSetText, CfgParmsSetIndex);
}

fixed_string CfgParmsStr = "cfgparms";
fixed_string CfgParmsExpl = "Supports configuration parameters.";

CfgParmsCommand::CfgParmsCommand() : CliCommand(CfgParmsStr, CfgParmsExpl)
{
   BindParm(*new CfgParmsAction);
}

fn_name CfgParmsCommand_ProcessCommand = "CfgParmsCommand.ProcessCommand";

word CfgParmsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CfgParmsCommand_ProcessCommand);

   id_t index;
   string key, expl, value;
   auto reg = Singleton< CfgParmRegistry >::Instance();
   CfgParm* parm;
   RestartLevel level;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case CfgParmsListIndex:
      if(!cli.EndOfInput()) return -1;
      reg->ListParms(*cli.obuf, spaces(2));
      break;

   case CfgParmsExplIndex:
      if(!GetString(key, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      parm = reg->FindParm(key);
      if(parm == nullptr) return cli.Report(-2, NoCfgParmExpl);
      parm->Explain(expl);
      return cli.Report(0, expl);

   case CfgParmsGetIndex:
      if(!GetString(key, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      if(!reg->GetValue(key, value)) return cli.Report(-2, NoCfgParmExpl);
      *cli.obuf << spaces(2) << "Value: " << value << CRLF;
      break;

   case CfgParmsSetIndex:
      if(!GetString(key, cli)) return -1;
      if(!GetString(value, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      parm = reg->FindParm(key);
      if(parm == nullptr) return cli.Report(-2, NoCfgParmExpl);

      if(!parm->SetValue(value.c_str(), level))
      {
         parm->Explain(expl);
         return cli.Report(-3, BadParameterValue + expl);
      }

      if(level != RestartNone)
      {
         std::ostringstream stream;
         stream << "This change will take effect after the next ";
         stream << level << " restart.";
         return cli.Report(-4, stream.str());
      }

      return cli.Report(0, SuccessExpl);

   default:
      Debug::SwLog(CfgParmsCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The CLEAR command.
//
fixed_string ClearWhatExpl = "what to clear...";

ClearWhatParm::ClearWhatParm() : CliTextParm(ClearWhatExpl)
{
   BindText(*new BufferText, ClearCommand::BufferIndex);
   BindText(*new ToolsText, ClearCommand::ToolsIndex);
   BindText(*new SelectionsText, ClearCommand::SelectionsIndex);
   BindText(*new FactionText, ClearCommand::FactionIndex);
   BindText(*new FactionsText, ClearCommand::FactionsIndex);
   BindText(*new ThreadText, ClearCommand::ThreadIndex);
   BindText(*new ThreadsText, ClearCommand::ThreadsIndex);
}

fixed_string ClearStr = "clear";
fixed_string ClearExpl = "Clears the trace buffer, tools, or selections.";

ClearCommand::ClearCommand(bool bind) : CliCommand(ClearStr, ClearExpl)
{
   if(bind) BindParm(*new ClearWhatParm);
}

void ClearCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

word ClearCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ClearCommand.ProcessCommand");

   id_t clearWhatIndex;

   if(!GetTextIndex(clearWhatIndex, cli)) return -1;

   return ProcessSubcommand(cli, clearWhatIndex);
}

word ClearCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("ClearCommand.ProcessSubcommand");

   TraceRc rc;
   auto nbt = Singleton< NbTracer >::Instance();
   word id = 0;

   switch(index)
   {
   case BufferIndex:
      if(!cli.EndOfInput()) return -1;
      rc = Singleton< TraceBuffer >::Instance()->Clear();
      break;
   case ToolsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = Singleton< TraceBuffer >::Instance()->ClearTools();
      break;
   case SelectionsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = nbt->ClearSelections(TraceAll);
      break;
   case FactionIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nbt->SelectFaction(Faction(id), TraceDefault);
      break;
   case FactionsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = nbt->ClearSelections(TraceFaction);
      break;
   case ThreadIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = NbTracer::SelectThread(id, TraceDefault);
      break;
   case ThreadsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = nbt->ClearSelections(TraceThread);
      break;
   default:
      return CliCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The DAEMONS command.
//
class DaemonsListText : public CliText
{
public: DaemonsListText();
};

class DaemonsSetText : public CliText
{
public: DaemonsSetText();
};

class DaemonsAction : public CliTextParm
{
public: DaemonsAction();
};

class DaemonsCommand : public CliCommand
{
public:
   DaemonsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DaemonsListTextStr = "list";
fixed_string DaemonsListTextExpl =
   "shows info for all daemons or a specific daemon";

DaemonsListText::DaemonsListText() :
   CliText(DaemonsListTextExpl, DaemonsListTextStr)
{
   BindParm(*new IdOptParm);
   BindParm(*new DispBVParm);
}

fixed_string DaemonsSetTextStr = "set";
fixed_string DaemonsSetTextExpl = "disables (off) or enables (on) a daemon";

DaemonsSetText::DaemonsSetText() :
   CliText(DaemonsSetTextExpl, DaemonsSetTextStr)
{
   BindParm(*new IdMandParm);
   BindParm(*new SetHowParm);
}

constexpr id_t DaemonsListIndex = 1;
constexpr id_t DaemonsSetIndex = 2;

fixed_string DaemonsActionExpl = "subcommand...";

DaemonsAction::DaemonsAction() : CliTextParm(DaemonsActionExpl)
{
   BindText(*new DaemonsListText, DaemonsListIndex);
   BindText(*new DaemonsSetText, DaemonsSetIndex);
}

fixed_string DaemonsStr = "daemons";
fixed_string DaemonsExpl = "Displays daemons.";

DaemonsCommand::DaemonsCommand() : CliCommand(DaemonsStr, DaemonsExpl)
{
   BindParm(*new DaemonsAction);
}

fn_name DaemonsCommand_ProcessCommand = "DaemonsCommand.ProcessCommand";

word DaemonsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(DaemonsCommand_ProcessCommand);

   id_t index, setHowIndex;
   word id;
   bool all, v = false;
   Daemon* daemon = nullptr;
   auto reg = Singleton< DaemonRegistry >::Instance();

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case DaemonsListIndex:
      switch(GetIntParmRc(id, cli))
      {
      case None: all = true; break;
      case Ok: all = false; break;
      default: return -1;
      }

      if(GetBV(*this, cli, v) == Error) return -1;
      if(!cli.EndOfInput()) return -1;

      if(all)
      {
         reg->Output(*cli.obuf, 2, v);
      }
      else
      {
         daemon = reg->Daemons().At(id);
         if(daemon == nullptr) return cli.Report(-2, NoDaemonExpl);
         daemon->Output(*cli.obuf, 2, v);
      }

      return 0;

   case DaemonsSetIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!GetTextIndex(setHowIndex, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      daemon = reg->Daemons().At(id);
      if(daemon == nullptr) return cli.Report(-2, NoDaemonExpl);
      if(setHowIndex == SetHowParm::Off)
         daemon->Disable();
      else
         daemon->Enable();
      return cli.Report(0, SuccessExpl);

   default:
      Debug::SwLog(DaemonsCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }
}

//------------------------------------------------------------------------------
//
//  The DELAY command.
//
class DelayCommand : public CliCommand
{
public:
   DelayCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DelayTimeExpl = "time (secs)";

fixed_string DelayStr = "delay";
fixed_string DelayExpl = "Pauses before executing the next command.";

DelayCommand::DelayCommand() : CliCommand(DelayStr, DelayExpl)
{
   BindParm(*new CliIntParm(DelayTimeExpl, 0, 180));
}

word DelayCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DelayCommand.ProcessCommand");

   word secs;

   if(!GetIntParm(secs, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto rc = ThisThread::Pause(Duration(secs, SECS));
   if(rc != DelayCompleted) return cli.Report(-6, DelayFailure);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The DISPLAY command.
//
fixed_string ObjPtrMandText = "pointer to an object derived from Base";

class DisplayCommand : public CliCommand
{
public:
   DisplayCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DisplayStr = "display";
fixed_string DisplayExpl = "Displays an object derived from NodeBase::Base.";

DisplayCommand::DisplayCommand() : CliCommand(DisplayStr, DisplayExpl)
{
   BindParm(*new CliPtrParm(ObjPtrMandText));
   BindParm(*new DispBVParm);
}

word DisplayCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DisplayCommand.ProcessCommand");

   void* p = nullptr;
   bool v = false;
   std::ostringstream prompt;

   if(!GetPtrParm(p, cli)) return -1;
   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   prompt << BadObjectPtrWarning << CRLF << ContinuePrompt;
   if(!cli.BoolPrompt(prompt.str())) return cli.Report(0, CommandAbortedExpl);
   static_cast< const Base* >(p)->Output(*cli.obuf, 2, v);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The DUMP command.
//
class DumpCommand : public CliCommand
{
public:
   DumpCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string MemAddrText = "memory address";

fixed_string ByteCountExpl = "number of bytes to display";

fixed_string DumpStr = "dump";
fixed_string DumpExpl = "Displays memory in hex.";

DumpCommand::DumpCommand() : CliCommand(DumpStr, DumpExpl)
{
   BindParm(*new CliPtrParm(MemAddrText));
   BindParm(*new CliIntParm(ByteCountExpl, 1, 1024));
}

word DumpCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DumpCommand.ProcessCommand");

   void* p = nullptr;
   word n = 0;
   std::ostringstream prompt;

   if(!GetPtrParm(p, cli)) return -1;
   if(GetIntParmRc(n, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   prompt << BadObjectPtrWarning << CRLF << ContinuePrompt;
   if(!cli.BoolPrompt(prompt.str())) return cli.Report(0, CommandAbortedExpl);
   strBytes(*cli.obuf, spaces(2), static_cast< const byte_t* >(p), n);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The ECHO command.
//
class EchoCommand : public CliCommand
{
public:
   EchoCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string EchoParmExpl = "the string to be written to the console";

fixed_string EchoStr = "echo";
fixed_string EchoExpl = "Writes the rest of the input line to the console.";

EchoCommand::EchoCommand() : CliCommand(EchoStr, EchoExpl)
{
   BindParm(*new CliTextParm(EchoParmExpl, false, 0));
}

word EchoCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("EchoCommand.ProcessCommand");

   cli.ibuf->Echo();
   return 0;
}

//------------------------------------------------------------------------------
//
//  The EXCLUDE command.
//
fixed_string ExcludeWhatExpl = "what to exclude...";

ExcludeWhatParm::ExcludeWhatParm() : CliTextParm(ExcludeWhatExpl)
{
   BindText(*new FactionText, ExcludeCommand::ExcludeFactionIndex);
   BindText(*new ThreadText, ExcludeCommand::ExcludeThreadIndex);
}

fixed_string ExcludeStr = "exclude";
fixed_string ExcludeExpl =
   "Specifies what should not be captured by trace tools.";

ExcludeCommand::ExcludeCommand(bool bind) : CliCommand(ExcludeStr, ExcludeExpl)
{
   if(bind) BindParm(*new ExcludeWhatParm);
}

void ExcludeCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

word ExcludeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ExcludeCommand.ProcessCommand");

   id_t excludeWhatIndex;

   if(!GetTextIndex(excludeWhatIndex, cli)) return -1;

   return ProcessSubcommand(cli, excludeWhatIndex);
}

word ExcludeCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("ExcludeCommand.ProcessSubcommand");

   TraceRc rc;
   auto nbt = Singleton< NbTracer >::Instance();
   word id;

   switch(index)
   {
   case ExcludeFactionIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nbt->SelectFaction(Faction(id), TraceExcluded);
      break;
   case ExcludeThreadIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = NbTracer::SelectThread(id, TraceExcluded);
      break;
   default:
      return CliCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The HEAPS command.
//
class HeapsInUseText : public CliText
{
public: HeapsInUseText();
};

class HeapsTraceAction : public CliTextParm
{
public: HeapsTraceAction();
};

class HeapsTraceText : public CliText
{
public: HeapsTraceText();
};

class HeapsAction : public CliTextParm
{
public: HeapsAction();
};

class HeapsCommand : public CliCommand
{
public:
   HeapsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string HeapsListTextStr = "list";
fixed_string HeapsListTextExpl = "lists all heaps";

fixed_string HeapsInUseTextStr = "inuse";
fixed_string HeapsInUseTextExpl = "returns the number of bytes allocated";

HeapsInUseText::HeapsInUseText() :
   CliText(HeapsInUseTextExpl, HeapsInUseTextStr)
{
   BindParm(*new MemoryTypeParm);
}

constexpr id_t HeapsResetIndex = 1;
constexpr id_t HeapsStartIndex = 2;
constexpr id_t HeapsStopIndex = 3;
constexpr id_t HeapsDisplayIndex = 4;

fixed_string HeapsResetTextStr = "reset";
fixed_string HeapsResetTextExpl = "clears allocated blocks";

fixed_string HeapsStartTextStr = "start";
fixed_string HeapsStartTextExpl = "starts tracing of allocated blocks";

fixed_string HeapsStopTextStr = "stop";
fixed_string HeapsStopTextExpl = "stops tracing of allocated blocks";

fixed_string HeapsDisplayTextStr = "display";
fixed_string HeapsDisplayTextExpl = "displays allocated blocks";

fixed_string HeapsTraceActionExpl = "tracing subcommand...";

HeapsTraceAction::HeapsTraceAction() : CliTextParm(HeapsTraceActionExpl)
{
   BindText(*new CliText
      (HeapsResetTextExpl, HeapsResetTextStr), HeapsResetIndex);
   BindText(*new CliText
      (HeapsStartTextExpl, HeapsStartTextStr), HeapsStartIndex);
   BindText(*new CliText
      (HeapsStopTextExpl, HeapsStopTextStr), HeapsStopIndex);
   BindText(*new CliText
      (HeapsDisplayTextExpl, HeapsDisplayTextStr), HeapsDisplayIndex);
}

fixed_string HeapsTraceTextStr = "trace";
fixed_string HeapsTraceTextExpl = "controls heap trace tool";

HeapsTraceText::HeapsTraceText() :
   CliText(HeapsTraceTextExpl, HeapsTraceTextStr)
{
   BindParm(*new MemoryTypeParm);
   BindParm(*new HeapsTraceAction);
}

fixed_string HeapsValidateTextStr = "validate";
fixed_string HeapsValidateTextExpl = "validates all heaps";

constexpr id_t HeapsListIndex = 1;
constexpr id_t HeapsInUseIndex = 2;
constexpr id_t HeapsTraceIndex = 3;
constexpr id_t HeapsValidateIndex = 4;

fixed_string HeapsActionExpl = "subcommand...";

HeapsAction::HeapsAction() : CliTextParm(HeapsActionExpl)
{
   BindText(*new CliText(HeapsListTextExpl, HeapsListTextStr), HeapsListIndex);
   BindText(*new HeapsInUseText, HeapsInUseIndex);
   BindText(*new HeapsTraceText, HeapsTraceIndex);
   BindText(*new CliText
      (HeapsValidateTextExpl, HeapsValidateTextStr), HeapsValidateIndex);
}

fixed_string HeapsStr = "heaps";
fixed_string HeapsExpl = "Lists all heaps.";

HeapsCommand::HeapsCommand() : CliCommand(HeapsStr, HeapsExpl)
{
   BindParm(*new HeapsAction);
}

fn_name HeapsCommand_ProcessCommand = "HeapsCommand.ProcessCommand";

word HeapsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(HeapsCommand_ProcessCommand);

   id_t index;
   id_t trace;
   word memtype;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case HeapsListIndex:
      if(!cli.EndOfInput()) return -1;
      Memory::DisplayHeaps(*cli.obuf, spaces(2));
      return 0;

   case HeapsInUseIndex:
      if(!GetIntParm(memtype, cli)) return -1;
      if(cli.EndOfInput())
      {
         auto type = MemoryType(memtype);
         auto heap = Memory::AccessHeap(type);
         if(heap == nullptr) return cli.Report(-2, "Heap not found.");
         auto size = heap->BytesInUse();
         *cli.obuf << spaces(2) << "Bytes in use: " << size << CRLF;
         return size;
      }
      return -1;

   case HeapsTraceIndex:
      if(!GetIntParm(memtype, cli)) return -1;
      if(!GetTextIndex(trace, cli)) return -1;
      if(cli.EndOfInput())
      {
         auto type = MemoryType(memtype);
         auto heap = Memory::AccessHeap(type);
         if(heap == nullptr) return cli.Report(-2, "Heap not found.");

         switch(trace)
         {
         case HeapsResetIndex:
            heap->ResetTrace();
            return cli.Report(0, SuccessExpl);
         case HeapsStartIndex:
            heap->SetTrace(true);
            return cli.Report(0, SuccessExpl);
         case HeapsStopIndex:
            heap->SetTrace(false);
            return cli.Report(0, SuccessExpl);
         case HeapsDisplayIndex:
            heap->DisplayBlocks(*cli.obuf);
            return 0;
         default:
            return cli.Report(trace, SystemErrorExpl);
         }
      }
      return -1;

   case HeapsValidateIndex:
      if(!cli.EndOfInput()) return -1;
      *cli.obuf << spaces(2) << "Validating heaps..." << CRLF;

      for(auto m = 1; m < MemoryType_N; ++m)
      {
         auto type = MemoryType(m);
         auto result = Memory::Validate(type, nullptr);
         string status;

         if(result > 0)
            status = "true";
         else if(result == 0)
            status = "false";
         else
            status = "unallocated";

         *cli.obuf << setw(13) << MemoryType(m) << ": " << status << CRLF;
      }

      return 0;

   default:
      Debug::SwLog(HeapsCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }
}

//------------------------------------------------------------------------------
//
//  The HELP command.
//
class HelpCommand : public CliCommand
{
public:
   HelpCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string HelpIncrExpl = "name of increment";

fixed_string HelpCommExpl = "name of command ('full' = all commands)";

class HelpFullParm : public CliTextParm
{
public: HelpFullParm();
};

fixed_string HelpFullStr = "full";
fixed_string HelpFullExpl = "displays full documentation";

HelpFullParm::HelpFullParm() : CliTextParm(HelpFullExpl, true)
{
   BindText(*new CliText(HelpFullExpl, HelpFullStr), 1);
}

fixed_string HelpStr = "help";
fixed_string HelpExpl = "Provides help for an increment or command.";

HelpCommand::HelpCommand() : CliCommand(HelpStr, HelpExpl)
{
   BindParm(*new CliTextParm(HelpIncrExpl, true, 0));
   BindParm(*new CliTextParm(HelpCommExpl, true, 0));
   BindParm(*new HelpFullParm);
}

static word DisplayHelp(const CliThread& cli, const string& key)
{
   auto path = Element::HelpPath() + PATH_SEPARATOR + "cli.txt";
   auto rc = cli.DisplayHelp(path, key);

   switch(rc)
   {
   case -1:
      return cli.Report(-1, "No additional help is available.", 0);
   case -2:
      return cli.Report(-2, "Failed to open file " + path, 0);
   }

   return 0;
}

word HelpCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HelpCommand.ProcessCommand");

   //     Input:                       Result:
   //  1  >help                        overview of CLI
   //  2  >help full                   detailed help for the current increment
   //  3  >help <comm> full            detailed help for the command
   //  4  >help <comm> <junk>          summary of the command's parameters
   //  5  >help <comm>                 summary of the command's parameters
   //  6  >help <junk>                 error
   //  7  >help <incr>                 summary of the increment's commands
   //  8  >help <incr> full            detailed help for the increment
   //  9  >help <incr> <junk>          summary of the increment's commands
   //  10 >help <incr> <comm> full     detailed help for the command
   //  11 >help <incr> <comm>          summary of the command's parameters
   //  12 >help <incr> <comm> <junk>   summary of the command's parameters
   //
   string s1, s2, s3;
   const CliIncrement* incr = nullptr;

   if(!GetString(s1, cli))
   {
      if(!cli.EndOfInput()) return -1;
      return DisplayHelp(cli, EMPTY_STR);  // [1]
   }

   if(s1 == "full")
   {
      if(!cli.EndOfInput()) return -1;
      incr = cli.stack_->Top();
      incr->Explain(*cli.obuf, 2);
      return DisplayHelp(cli, incr->Name());  // [2]
   }

   auto comm = cli.stack_->FindCommand(s1, incr);

   if(comm != nullptr)
   {
      if(GetString(s2, cli) && (s2 == "full"))
      {
         if(!cli.EndOfInput()) return -1;
         comm->ExplainCommand(*cli.obuf, true);
         *cli.obuf << CRLF;
         string key(incr->Name());
         key.push_back('.');
         key.append(comm->Text());
         return DisplayHelp(cli, key);  // [3]
      }

      if(!cli.EndOfInput()) return -1;

      if(!s2.empty())
      {
         *cli.obuf << spaces(2) << ParameterIgnored << s2 << CRLF;  // [4]
      }

      return comm->ExplainCommand(*cli.obuf, true);  // [4/5]
   }

   incr = Singleton< CliRegistry >::Instance()->FindIncrement(s1);

   if(incr == nullptr)
   {
      *cli.obuf << spaces(2) << ParameterInvalid << s1 << CRLF;  // [6]
      return -2;
   }

   if(!GetString(s2, cli))
   {
      if(!cli.EndOfInput()) return -1;
      return incr->Explain(*cli.obuf, 1);  // [7]
   }

   if(s2 == "full")
   {
      if(!cli.EndOfInput()) return -1;
      incr->Explain(*cli.obuf, 2);
      return DisplayHelp(cli, incr->Name());  // [8]
   }

   comm = incr->FindCommand(s2);

   if(comm == nullptr)
   {
      if(!cli.EndOfInput()) return -1;
      *cli.obuf << spaces(2) << ParameterIgnored << s2 << CRLF;
      return incr->Explain(*cli.obuf, 1);  // [9]
   }

   if(GetString(s3, cli) && (s3 == "full"))
   {
      if(!cli.EndOfInput()) return -1;
      comm->ExplainCommand(*cli.obuf, true);
      *cli.obuf << CRLF;
      string key(incr->Name());
      key.push_back('.');
      key.append(comm->Text());
      return DisplayHelp(cli, key);  // [10]
   }

   if(!cli.EndOfInput()) return -1;
   if(!s3.empty())
   {
      *cli.obuf << spaces(2) << ParameterIgnored << s3 << CRLF;  // [11]
   }

   return comm->ExplainCommand(*cli.obuf, true);  // [11/12]
}

//------------------------------------------------------------------------------
//
//  The IF command.
//
class ElseParm : public CliTextParm
{
public: ElseParm();
};

class IfCommand : public CliCommand
{
public:
   IfCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string IfSymbolExpl = "symbol for an integer (e.g. &cli.result)";

fixed_string IfValueExpl = "value for comparison";

fixed_string CommandMandExpl = "command to execute if condition is true";

fixed_string CommandOptExpl = "command to execute if condition is false";

fixed_string ElseStr = "else";
fixed_string ElseExpl = "precedes command to execute if condition is false";

ElseParm::ElseParm() : CliTextParm(ElseExpl, true)
{
   BindText(*new CliText(ElseExpl, ElseStr), 1);
}

fixed_string IfStr = "if";
fixed_string IfExpl = "Conditionally executes a CLI command.";

IfCommand::IfCommand() : CliCommand(IfStr, IfExpl)
{
   BindParm(*new CliIntParm(IfSymbolExpl, WORD_MIN, WORD_MAX));
   BindParm(*new RelationParm);
   BindParm(*new CliIntParm(IfValueExpl, WORD_MIN, WORD_MAX));
   BindParm(*new CliTextParm(CommandMandExpl, false, 0));
   BindParm(*new ElseParm);
   BindParm(*new CliTextParm(CommandOptExpl, true, 0));
}

fn_name IfCommand_ProcessCommand = "IfCommand.ProcessCommand";

word IfCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(IfCommand_ProcessCommand);

   id_t index;
   word sym, val;
   bool result;
   string comm;

   if(!GetIntParm(sym, cli)) return -1;
   if(!GetTextIndex(index, cli)) return -1;
   if(!GetIntParm(val, cli)) return -1;

   switch(index)
   {
   case RelationParm::Lt:
      result = (sym < val);
      break;
   case RelationParm::LtEq:
      result = (sym <= val);
      break;
   case RelationParm::Eq:
      result = (sym == val);
      break;
   case RelationParm::NEq:
      result = (sym != val);
      break;
   case RelationParm::Gt:
      result = (sym > val);
      break;
   case RelationParm::GtEq:
      result = (sym >= val);
      break;
   default:
      Debug::SwLog(IfCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   cli.ibuf->Read(comm);
   if(!cli.EndOfInput()) return -1;

   //  If the result was false, report it.  If the result was true, read
   //  the rest of the input line.  If it contains anything, execute it
   //  as a command; otherwise, report that the outcome was true.
   //
   string tcomm(comm);
   string fcomm(EMPTY_STR);
   auto split = comm.find(" else ");

   if(split != string::npos)
   {
      tcomm = comm.substr(0, split);
      fcomm = comm.substr(split + 6);
   }

   if(result)
   {
      if(!tcomm.empty()) return cli.Execute(tcomm);
      return cli.Report(1, ReturnTrue);
   }

   if(!fcomm.empty()) return cli.Execute(fcomm);
   return cli.Report(1, ReturnFalse);
}

//------------------------------------------------------------------------------
//
//  The INCLUDE command.
//
fixed_string IncludeWhatExpl = "what to include...";

IncludeWhatParm::IncludeWhatParm() : CliTextParm(IncludeWhatExpl)
{
   BindText(*new AllActivityText, IncludeCommand::IncludeAllIndex);
   BindText(*new FactionText, IncludeCommand::IncludeFactionIndex);
   BindText(*new ThreadText, IncludeCommand::IncludeThreadIndex);
}

fixed_string IncludeStr = "include";
fixed_string IncludeExpl = "Specifies what should be captured by trace tools.";

IncludeCommand::IncludeCommand(bool bind) : CliCommand(IncludeStr, IncludeExpl)
{
   if(bind) BindParm(*new IncludeWhatParm);
}

void IncludeCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

word IncludeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("IncludeCommand.ProcessCommand");

   id_t includeWhatIndex;

   if(!GetTextIndex(includeWhatIndex, cli)) return -1;

   return ProcessSubcommand(cli, includeWhatIndex);
}

word IncludeCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("IncludeCommand.ProcessSubcommand");

   TraceRc rc;
   auto nbt = Singleton< NbTracer >::Instance();
   word id;
   id_t setHowIndex;
   bool flag;

   switch(index)
   {
   case IncludeAllIndex:
      if(!GetTextIndex(setHowIndex, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      flag = (setHowIndex == SetHowParm::On);
      rc = Singleton< TraceBuffer >::Instance()->SelectAll(flag);
      break;
   case IncludeFactionIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nbt->SelectFaction(Faction(id), TraceIncluded);
      break;
   case IncludeThreadIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = NbTracer::SelectThread(id, TraceIncluded);
      break;
   default:
      return CliCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The INCRS command.
//
class IncrsCommand : public CliCommand
{
public:
   IncrsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string IncrsStr = "incrs";
fixed_string IncrsExpl = "Lists all available increments.";

IncrsCommand::IncrsCommand() : CliCommand(IncrsStr, IncrsExpl) { }

word IncrsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("IncrsCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   Singleton< CliRegistry >::Instance()->ListIncrements(*cli.obuf);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The LOGS command.
//
class LogsListText : public CliText
{
public: LogsListText();
};

fixed_string LogsListTextStr = "list";
fixed_string LogsListTextExpl =
   "shows info for all logs or the logs in a specific group";

LogsListText::LogsListText() :
   CliText(LogsListTextExpl, LogsListTextStr)
{
   BindParm(*new LogGroupOptParm);
}

fixed_string LogsGroupsTextStr = "groups";
fixed_string LogsGroupsTextExpl = "lists all log groups";

class LogsExplainText : public CliText
{
public: LogsExplainText();
};

fixed_string LogsExplainTextStr = "explain";
fixed_string LogsExplainTextExpl = "displays documentation for a log";

LogsExplainText::LogsExplainText() :
   CliText(LogsExplainTextExpl, LogsExplainTextStr)
{
   BindParm(*new LogGroupMandParm);
   BindParm(*new LogIdMandParm);
}

class LogsSuppressText : public CliText
{
public: LogsSuppressText();
};

fixed_string LogsSuppressTextStr = "suppress";
fixed_string LogsSuppressTextExpl = "suppresses all the logs in a group";

LogsSuppressText::LogsSuppressText() :
   CliText(LogsSuppressTextExpl, LogsSuppressTextStr)
{
   BindParm(*new LogGroupMandParm);
   BindParm(*new SetHowParm);
}

fixed_string LogThrottleExpl = "report every Nth log (0=none, 1=all)";

class LogsThrottleText : public CliText
{
public: LogsThrottleText();
};

fixed_string LogsThrottleTextStr = "throttle";
fixed_string LogsThrottleTextExpl = "throttles or suppresses a specific log";

LogsThrottleText::LogsThrottleText() :
   CliText(LogsThrottleTextExpl, LogsThrottleTextStr)
{
   BindParm(*new LogGroupMandParm);
   BindParm(*new LogIdMandParm);
   BindParm(*new CliIntParm(LogThrottleExpl, 0, 100));
}

fixed_string LogsCountTextStr = "count";
fixed_string LogsCountTextExpl = "displays the number of logs reported so far";

class LogsBuffersText : public CliText
{
public: LogsBuffersText();
};

fixed_string LogsBuffersTextStr = "buffers";
fixed_string LogsBuffersTextExpl = "lists all log buffers";

LogsBuffersText::LogsBuffersText() :
   CliText(LogsBuffersTextExpl, LogsBuffersTextStr)
{
   BindParm(*new DispBVParm);
}

fixed_string LogCountExpl = "number of logs to send (0=all)";

class LogsWriteText : public CliText
{
public: LogsWriteText();
};

fixed_string LogsWriteTextStr = "write";
fixed_string LogsWriteTextExpl = "writes a buffer's logs to its log file";

LogsWriteText::LogsWriteText() : CliText(LogsWriteTextExpl, LogsWriteTextStr)
{
   BindParm(*new LogBufferIdParm);
   BindParm(*new CliIntParm(LogCountExpl, 0, 1000));
}

class LogsFreeText : public CliText
{
public: LogsFreeText();
};

fixed_string LogsFreeTextStr = "free";
fixed_string LogsFreeTextExpl = "deletes a log buffer";

LogsFreeText::LogsFreeText() : CliText(LogsFreeTextExpl, LogsFreeTextStr)
{
   BindParm(*new LogBufferIdParm);
}

fixed_string LogsActionExpl = "subcommand...";

LogsAction::LogsAction() : CliTextParm(LogsActionExpl)
{
   BindText(*new LogsListText, LogsCommand::ListIndex);
   BindText(*new CliText
      (LogsGroupsTextExpl, LogsGroupsTextStr), LogsCommand::GroupsIndex);
   BindText(*new LogsExplainText, LogsCommand::ExplainIndex);
   BindText(*new LogsThrottleText, LogsCommand::ThrottleIndex);
   BindText(*new LogsSuppressText, LogsCommand::SuppressIndex);
   BindText(*new CliText
      (LogsCountTextExpl, LogsCountTextStr), LogsCommand::CountIndex);
   BindText(*new LogsBuffersText, LogsCommand::BuffersIndex);
   BindText(*new LogsWriteText, LogsCommand::WriteIndex);
   BindText(*new LogsFreeText, LogsCommand::FreeIndex);
}

fixed_string LogsStr = "logs";
fixed_string LogsExpl = "Interface to the log subsystem.";

LogsCommand::LogsCommand(bool bind) : CliCommand(LogsStr, LogsExpl)
{
   if(bind) BindParm(*new LogsAction);
}

void LogsCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

//  Sets GROUP and LOG to the log group and log identified by NAME and ID.
//  Returns false if GROUP or LOG cannot be found, updating EXPL with an
//  explanation.  If GROUP is found and ID is 0, sets LOG to nullptr and
//  returns true.
//
static bool FindGroupAndLog(const string& name, word id,
   LogGroup*& group, Log*& log, string& expl)
{
   Debug::ft("NodeBase.FindGroupAndLog");

   auto reg = Singleton< LogGroupRegistry >::Instance();
   group = reg->FindGroup(name);

   if(group == nullptr)
   {
      expl = NoLogGroupExpl;
      return false;
   }

   log = nullptr;
   if(id == 0) return true;

   log = group->FindLog(id);

   if(log == nullptr)
   {
      expl = NoLogExpl;
      return false;
   }

   return true;
}

word LogsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("LogsCommand.ProcessCommand");

   id_t index;

   if(!GetTextIndex(index, cli)) return -1;

   return ProcessSubcommand(cli, index);
}

word LogsCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("LogsCommand.ProcessSubcommand");

   word rc = 0;
   string name, expl, key, path;
   word id, count, interval;
   auto v = false;
   id_t setHowIndex;
   Log* log;
   LogGroup* group;
   LogBuffer* buff;
   auto reg = Singleton< LogBufferRegistry >::Instance();

   switch(index)
   {
   case ListIndex:
      GetStringRc(name, cli);
      if(!cli.EndOfInput()) return -1;

      if(name.empty())
      {
         Singleton< LogGroupRegistry >::Instance()->Output(*cli.obuf, 2, true);
      }
      else
      {
         group = Singleton< LogGroupRegistry >::Instance()->FindGroup(name);
         if(group == nullptr) return cli.Report(-1, NoLogGroupExpl);
         group->Output(*cli.obuf, 2, true);
      }

      break;

   case GroupsIndex:
      if(!cli.EndOfInput()) return -1;
      Singleton< LogGroupRegistry >::Instance()->Output(*cli.obuf, 2, false);
      break;

   case ExplainIndex:
      if(!GetString(name, cli)) return -1;
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      if(!FindGroupAndLog(name, id, group, log, expl))
      {
         return cli.Report(-1, expl);
      }

      key = group->Name() + std::to_string(id);
      path = Element::HelpPath() + PATH_SEPARATOR + "logs.txt";
      rc = cli.DisplayHelp(path, key);

      switch(rc)
      {
      case -1:
         return cli.Report(-1, "This log has not been documented.");
      case -2:
         return cli.Report(-2, "Failed to open file " + path);
      }

      break;

   case SuppressIndex:
      if(!GetString(name, cli)) return -1;
      if(!GetTextIndex(setHowIndex, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      if(!FindGroupAndLog(name, 0, group, log, expl))
      {
         return cli.Report(-1, expl);
      }

      group->SetSuppressed(setHowIndex == SetHowParm::On);
      return cli.Report(0, SuccessExpl);

   case ThrottleIndex:
      if(!GetString(name, cli)) return -1;
      if(!GetIntParm(id, cli)) return -1;
      if(!GetIntParm(interval, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      if(!FindGroupAndLog(name, id, group, log, expl))
      {
         return cli.Report(-1, expl);
      }

      log->SetInterval(interval);
      return cli.Report(0, SuccessExpl);

   case BuffersIndex:
      if(GetBV(*this, cli, v) == Error) return -1;
      if(!cli.EndOfInput()) return -1;
      reg->Output(*cli.obuf, 2, v);
      break;

   case WriteIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!GetIntParm(count, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      buff = reg->Access(id);

      if(buff != nullptr)
      {
         auto file = buff->FileName();
         word size = buff->Count(true, true);
         size_t targ = (count < size ? size - count : 0);
         if(count == 0) targ = 0;
         buff->ResetAllToUnspooled();

         while(buff->Count(false, true) > targ)
         {
            CallbackRequestPtr callback;
            auto periodic = false;
            auto stream = buff->GetLogs(callback, periodic);
            if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
            FileThread::Spool(file, stream, callback);
         }

         return cli.Report(0, SuccessExpl);
      }

      return cli.Report(-1, "That buffer is either active or invalid.");

   case FreeIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      {
         FunctionGuard guard(Guard_ImmUnprotect);

         if(!Singleton< LogBufferRegistry >::Instance()->Free(id))
         {
            return cli.Report(-1, "That buffer is either active or invalid.");
         }
      }
      break;

   case CountIndex:
      if(!cli.EndOfInput()) return -1;
      *cli.obuf << Log::Count() << CRLF;
      return Log::Count();

   default:
      return CliCommand::ProcessSubcommand(cli, index);
   }

   return rc;
}

//------------------------------------------------------------------------------
//
//  The MODULES command.
//
class ModulesCommand : public CliCommand
{
public:
   ModulesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ModulesStr = "modules";
fixed_string ModulesExpl = "Displays modules.";

ModulesCommand::ModulesCommand() : CliCommand(ModulesStr, ModulesExpl)
{
   BindParm(*new ModuleIdOptParm);
   BindParm(*new DispBVParm);
}

word ModulesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ModulesCommand.ProcessCommand");

   word mid;
   bool all, v = false;

   switch(GetIntParmRc(mid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< ModuleRegistry >::Extant();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto mod = reg->GetModule(mid);
      if(mod == nullptr) return cli.Report(-2, NoModuleExpl);
      mod->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The MUTEXES command.
//
class MutexesCommand : public CliCommand
{
public:
   MutexesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string MutexesStr = "mutexes";
fixed_string MutexesExpl = "Displays mutexes.";

MutexesCommand::MutexesCommand() : CliCommand(MutexesStr, MutexesExpl)
{
   BindParm(*new IdOptParm);
   BindParm(*new DispBVParm);
}

word MutexesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MutexesCommand.ProcessCommand");

   word id;
   bool all, v = false;

   switch(GetIntParmRc(id, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< MutexRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto mutex = reg->Mutexes().At(id);
      if(mutex == nullptr) return cli.Report(-2, NoMutexExpl);
      mutex->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The POOLS command.
//
class PoolsCommand : public CliCommand
{
public:
   PoolsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string PoolsStr = "pools";
fixed_string PoolsExpl = "Displays object pools.";

PoolsCommand::PoolsCommand() : CliCommand(PoolsStr, PoolsExpl)
{
   BindParm(*new ObjPoolIdOptParm);
   BindParm(*new DispBVParm);
}

word PoolsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("PoolsCommand.ProcessCommand");

   word pid;
   bool all, v = false;

   switch(GetIntParmRc(pid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< ObjectPoolRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto pool = reg->Pool(pid);
      if(pool == nullptr) return cli.Report(-2, NoPoolExpl);
      pool->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The PSIGNALS command.
//
class PsignalsCommand : public CliCommand
{
public:
   PsignalsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string PsignalsStr = "psignals";
fixed_string PsignalsExpl = "Displays POSIX signals.";

PsignalsCommand::PsignalsCommand() : CliCommand(PsignalsStr, PsignalsExpl)
{
   BindParm(*new IdOptParm);
   BindParm(*new DispBVParm);
}

word PsignalsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("PsignalsCommand.ProcessCommand");

   word id;
   bool all, v = false;

   switch(GetIntParmRc(id, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< PosixSignalRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto signal = reg->Signals().At(id);
      if(signal == nullptr) return cli.Report(-2, NoPosixSignalExpl);
      signal->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The QUERY command.
//
fixed_string QueryWhatExpl = "what to query...";

QueryWhatParm::QueryWhatParm() : CliTextParm(QueryWhatExpl)
{
   BindText(*new BufferText, QueryCommand::BufferIndex);
   BindText(*new ToolsText, QueryCommand::ToolsIndex);
   BindText(*new SelectionsText, QueryCommand::SelectionsIndex);
}

fixed_string QueryStr = "query";
fixed_string QueryExpl = "Shows the status of trace tools.";

QueryCommand::QueryCommand(bool bind) : CliCommand(QueryStr, QueryExpl)
{
   if(bind) BindParm(*new QueryWhatParm);
}

void QueryCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

word QueryCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("QueryCommand.ProcessCommand");

   id_t queryWhatIndex;

   if(!GetTextIndex(queryWhatIndex, cli)) return -1;

   return ProcessSubcommand(cli, queryWhatIndex);
}

word QueryCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("QueryCommand.ProcessSubcommand");

   if(!cli.EndOfInput()) return -1;

   switch(index)
   {
   case BufferIndex:
      Singleton< TraceBuffer >::Instance()->Query(*cli.obuf);
      break;
   case ToolsIndex:
      Singleton< TraceBuffer >::Instance()->QueryTools(*cli.obuf);
      break;
   case SelectionsIndex:
      Singleton< NbTracer >::Instance()->QuerySelections(*cli.obuf);
      break;
   default:
      return CliCommand::ProcessSubcommand(cli, index);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The QUIT command.
//
class QuitParm : public CliTextParm
{
public: QuitParm();
};

class QuitCommand : public CliCommand
{
public:
   QuitCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string QuitAllStr = "all";
fixed_string QuitAllExpl = "exits all increments";

QuitParm::QuitParm() : CliTextParm(QuitAllExpl, true)
{
   BindText(*new CliText(QuitAllExpl, QuitAllStr), 1);
}

fixed_string QuitStr = "quit";
fixed_string QuitExpl = "Exits the most recent (or all) increments.";

QuitCommand::QuitCommand() : CliCommand(QuitStr, QuitExpl)
{
   BindParm(*new QuitParm);
}

word QuitCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("QuitCommand.ProcessCommand");

   id_t index;

   auto rc = GetTextIndexRc(index, cli);
   if(rc == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   if(!cli.stack_->Pop()) return cli.Report(0, NoIncrExpl);

   if(rc == Ok)
   {
      //  >quit all
      //
      while(cli.stack_->Pop());
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The READ command.
//
class ReadCommand : public CliCommand
{
public:
   ReadCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ReadWhereExpl = "read input from <str>.txt";

fixed_string ReadStr = "read";
fixed_string ReadExpl = "Reads commands from a file.";

ReadCommand::ReadCommand() : CliCommand(ReadStr, ReadExpl)
{
   BindParm(*new CliTextParm(ReadWhereExpl, false, 0));
}

word ReadCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ReadCommand.ProcessCommand");

   string name;
   string expl;

   //  Get the file's name.  If it isn't CIN, set its extension to ".txt".
   //
   if(!GetString(name, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   //  If input is to be taken from the console, there is nothing to do.
   //  Commands are read from an input file until exhausted, after which
   //  console input is automatically restored.
   //
   if(name == "cin") return cli.Report(0, ConsoleAutomaticExpl);

   auto rc = cli.ibuf->OpenInputFile(name, expl);
   if(rc != 0) return cli.Report(rc, expl);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The RESTART command.
//
class RestartType : public CliTextParm
{
public: RestartType();
};

class RestartCommand : public CliCommand
{
public:
   RestartCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string WarmTextStr = "warm";
fixed_string WarmTextExpl = "exits and recreates threads";

fixed_string ColdTextStr = "cold";
fixed_string ColdTextExpl = "deletes sessions (plus warm actions)";

fixed_string ReloadTextStr = "reload";
fixed_string ReloadTextExpl = "reloads data (plus cold and warm actions)";

fixed_string RebootTextStr = "reboot";
fixed_string RebootTextExpl = "exits and restarts the entire system";

fixed_string ExitTextStr = "exit";
fixed_string ExitTextExpl = "exits and does not restart the system";

fixed_string RestartStr = "restart";
fixed_string RestartExpl = "Shuts down the system.";

constexpr id_t WarmIndex = 1;
constexpr id_t ColdIndex = 2;
constexpr id_t ReloadIndex = 3;
constexpr id_t RebootIndex = 4;
constexpr id_t ExitIndex = 5;

fixed_string RestartTypeExpl = "type of shutdown...";

RestartType::RestartType() : CliTextParm(RestartTypeExpl)
{
   BindText(*new CliText(WarmTextExpl, WarmTextStr), WarmIndex);
   BindText(*new CliText(ColdTextExpl, ColdTextStr), ColdIndex);
   BindText(*new CliText(ReloadTextExpl, ReloadTextStr), ReloadIndex);
   BindText(*new CliText(RebootTextExpl, RebootTextStr), RebootIndex);
   BindText(*new CliText(ExitTextExpl, ExitTextStr), ExitIndex);
}

RestartCommand::RestartCommand() : CliCommand(RestartStr, RestartExpl)
{
   BindParm(*new RestartType);
}

word RestartCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("RestartCommand.ProcessCommand");

   id_t index;
   RestartLevel level;
   std::ostringstream prompt;

   if(!GetTextIndex(index, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   if(index == ExitIndex)
   {
      if(!Element::RunningInLab()) return cli.Report(-5, NotInFieldExpl);
   }

   switch(index)
   {
   case WarmIndex:
      level = RestartWarm;
      break;
   case ColdIndex:
      level = RestartCold;
      break;
   case ReloadIndex:
      level = RestartReload;
      break;
   case RebootIndex:
      level = RestartReboot;
      break;
   case ExitIndex:
      level = RestartExit;
      break;
   default:
      return cli.Report(index, SystemErrorExpl);
   }

   prompt << RestartWarning << CRLF << ContinuePrompt;
   if(!cli.BoolPrompt(prompt.str())) return cli.Report(0, CommandAbortedExpl);
   Restart::Initiate(level, ManualRestart, level);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The SAVE command.
//
fixed_string SetOptionsExpl = "options: t=suppress times; c=don't move ctors";

const string& ValidSetOptions = "tc";

class TraceText : public CliText
{
public: TraceText();
};

fixed_string TraceTextStr = "trace";
fixed_string TraceTextExpl = "events captured by tools that are currently ON";

TraceText::TraceText() : CliText(TraceTextExpl, TraceTextStr)
{
   BindParm(*new OstreamMandParm);
   BindParm(*new CliTextParm(SetOptionsExpl, true, 0));
}

fixed_string SaveWhatExpl = "what to save...";

SaveWhatParm::SaveWhatParm() : CliTextParm(SaveWhatExpl)
{
   BindText(*new TraceText, SaveCommand::TraceIndex);
}

fixed_string SaveStr = "save";
fixed_string SaveExpl = "Saves what was captured by trace tools.";

SaveCommand::SaveCommand(bool bind) : CliCommand(SaveStr, SaveExpl)
{
   if(bind) BindParm(*new SaveWhatParm);
}

void SaveCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

word SaveCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SaveCommand.ProcessCommand");

   id_t saveWhatIndex;

   if(!GetTextIndex(saveWhatIndex, cli)) return -1;

   return ProcessSubcommand(cli, saveWhatIndex);
}

word SaveCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("SaveCommand.ProcessSubcommand");

   if(index != TraceIndex) return CliCommand::ProcessSubcommand(cli, index);

   TraceRc rc;
   string title;
   string opts;
   string expl;

   auto yield = cli.GenerateReportPreemptably();
   FunctionGuard guard(Guard_MakePreemptable, yield);

   if(!GetFileName(title, cli)) return -1;
   if(GetStringRc(opts, cli) == CliParm::Error) return -1;
   if(!cli.EndOfInput()) return -1;

   if(!opts.empty() && (opts != "-"))
   {
      if(!ValidateOptions(opts, ValidSetOptions, expl))
      {
         return cli.Report(-1, expl);
      }
   }

   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   rc = Singleton< TraceBuffer >::Instance()->DisplayTrace(stream, opts);

   if(rc == TraceOk)
   {
      title += ".trace.txt";
      cli.SendToFile(title, true);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The SCHED command.
//
class SchedShowText : public CliText
{
public: SchedShowText();
};

class SchedKillText : public CliText
{
public: SchedKillText();
};

class SchedAction : public CliTextParm
{
public: SchedAction();
};

class SchedCommand : public CliCommand
{
public:
   SchedCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SchedShowTextStr = "show";
fixed_string SchedShowTextExpl = "displays thread statistics";

SchedShowText::SchedShowText() : CliText(SchedShowTextExpl, SchedShowTextStr)
{
   BindParm(*new OstreamOptParm);
}

fixed_string SchedStartTextStr = "start";
fixed_string SchedStartTextExpl = "starts tracing context switches";

fixed_string SchedStopTextStr = "stop";
fixed_string SchedStopTextExpl = "stops tracing context switches";

fixed_string SchedKillTextStr = "kill";
fixed_string SchedKillTextExpl = "kills a thread";

SchedKillText::SchedKillText() : CliText(SchedKillTextExpl, SchedKillTextStr)
{
   BindParm(*new ThreadIdMandParm);
}

constexpr id_t SchedShowIndex = 1;
constexpr id_t SchedStartIndex = 2;
constexpr id_t SchedStopIndex = 3;
constexpr id_t SchedKillIndex = 4;

fixed_string SchedActionExpl = "subcommand...";

SchedAction::SchedAction() : CliTextParm(SchedActionExpl)
{
   BindText(*new SchedShowText, SchedShowIndex);
   BindText(*new CliText
      (SchedStartTextExpl, SchedStartTextStr), SchedStartIndex);
   BindText(*new CliText
      (SchedStopTextExpl, SchedStopTextStr), SchedStopIndex);
   BindText(*new SchedKillText, SchedKillIndex);
}

fixed_string SchedStr = "sched";
fixed_string SchedExpl = "Provides scheduler information.";

SchedCommand::SchedCommand() : CliCommand(SchedStr, SchedExpl)
{
   BindParm(*new SchedAction);
}

fn_name SchedCommand_ProcessCommand = "SchedCommand.ProcessCommand";

word SchedCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(SchedCommand_ProcessCommand);

   auto rc = TraceOk;
   id_t index;
   string title;
   ostream* stream = cli.obuf.get();
   word tid;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case SchedShowIndex:
   {
      if(!GetFileName(title, cli)) title.clear();
      if(!cli.EndOfInput()) return -1;

      auto yield = (title.empty() ? false : cli.GenerateReportPreemptably());
      FunctionGuard guard(Guard_MakePreemptable, yield);

      if(!title.empty())
      {
         stream = cli.FileStream();
         if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      }

      Thread::DisplaySummaries(*stream);
      if(title.empty()) return 0;
      Thread::DisplayContextSwitches(*stream);
      title += ".sched.txt";
      cli.SendToFile(title, true);
      break;
   }

   case SchedStartIndex:
      if(!cli.EndOfInput()) return -1;
      rc = Thread::LogContextSwitches(true);
      break;

   case SchedStopIndex:
      if(!cli.EndOfInput()) return -1;
      rc = Thread::LogContextSwitches(false);
      break;

   case SchedKillIndex:
      if(!GetIntParm(tid, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      {
         auto thr = Singleton< ThreadRegistry >::Instance()->GetThread(tid);
         if(thr == nullptr) return cli.Report(-2, NoThreadExpl);

         auto daemon = thr->GetDaemon();
         if(daemon != nullptr)
         {
            std::ostringstream prompt;
            prompt << "Do you want to disable this thread's daemon" << CRLF;
            prompt << "so it will not try to recreate the thread?";

            if(cli.BoolPrompt(prompt.str()))
            {
               daemon->Disable();
            }
         }

         auto expl = thr->Kill();
         if(expl != nullptr) return cli.Report(-1, expl);
      }
      break;

   default:
      Debug::SwLog(SchedCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The SEND command.
//
class FileText : public CliText
{
public: FileText();
};

class SendWhereParm : public CliTextParm
{
public: SendWhereParm();
};

class SendCommand : public CliCommand
{
public:
   SendCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CoutTextStr = "cout";
fixed_string CoutTextExpl = "to the console";

fixed_string PrevTextStr = "prev";
fixed_string PrevTextExpl = "to the previous location";

fixed_string AppendExpl = "append if file already exists? (default=f)";

fixed_string FileTextStr = "";
fixed_string FileTextExpl = "to the file specified";

FileText::FileText() : CliText(FileTextExpl, FileTextStr)
{
   BindParm(*new CliBoolParm(AppendExpl, true));
}

constexpr id_t SendCoutIndex = 1;
constexpr id_t SendPrevIndex = 2;
constexpr id_t SendFileIndex = 3;

fixed_string SendWhereExpl = "where to send CLI output";

SendWhereParm::SendWhereParm() : CliTextParm(SendWhereExpl)
{
   BindText(*new CliText(CoutTextExpl, CoutTextStr), SendCoutIndex);
   BindText(*new CliText(PrevTextExpl, PrevTextStr), SendPrevIndex);
   BindText(*new FileText, SendFileIndex);
}

fixed_string SendStr = "send";
fixed_string SendExpl = "Sends CLI output to the console or a file.";

SendCommand::SendCommand() : CliCommand(SendStr, SendExpl)
{
   BindParm(*new SendWhereParm);
}

fn_name SendCommand_ProcessCommand = "SendCommand.ProcessCommand";

word SendCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(SendCommand_ProcessCommand);

   id_t index;
   string title;
   bool all = false;
   bool append = false;

   if(!GetTextParm(index, title, cli)) return -1;

   switch(index)
   {
   case SendCoutIndex:
   case SendPrevIndex:
      //
      //  >send cout clears the entire output stack, whereas >send prev
      //  only clears the top.
      //
      if(!cli.EndOfInput()) return -1;

      if(index == SendCoutIndex) all = true;
      if(cli.PopOutputFile(all)) return 0;
      return cli.Report(0, SendingToConsoleExpl);

   case SendFileIndex:
      if(GetBoolParmRc(append, cli) == Error) return -1;
      if(!cli.EndOfInput()) return -1;

      title += ".cli.txt";

      if(cli.PushOutputFile(title))
      {
         if(!append) FileThread::Truncate(title);
         return 0;
      }

      return cli.Report(-7, TooManyOutputStreams);

   default:
      Debug::SwLog(SendCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }
}

//------------------------------------------------------------------------------
//
//  The SET command.
//
class BuffSizeText : public CliText
{
public: BuffSizeText();
};

class BuffWrapText : public CliText
{
public: BuffWrapText();
};

class ToolListText : public CliText
{
public: ToolListText();
};

fixed_string BuffSizeExpl = "buffer size (=2^N events)";

fixed_string BuffSizeTextStr = "buffsize";
fixed_string BuffSizeTextExpl = "capacity of trace buffer";

BuffSizeText::BuffSizeText() : CliText(BuffSizeTextExpl, BuffSizeTextStr)
{
   BindParm(*new CliIntParm(BuffSizeExpl,
      TraceBuffer::MinSize, TraceBuffer::MaxSize));
}

fixed_string BuffWrapExpl = "allow trace buffer to wrap around?";

fixed_string BuffWrapTextStr = "wrap";
fixed_string BuffWrapTextExpl = "whether trace buffer can wrap around";

BuffWrapText::BuffWrapText() : CliText(BuffWrapTextExpl, BuffWrapTextStr)
{
   BindParm(*new CliBoolParm(BuffWrapExpl));
}

fixed_string ToolListExpl = "tools to set: string of tool abbreviations";

fixed_string ToolListTextStr = "tools";
fixed_string ToolListTextExpl =
   "trace tools: see >tools command for abbrevations";

ToolListText::ToolListText() : CliText(ToolListTextExpl, ToolListTextStr)
{
   BindParm(*new CliTextParm(ToolListExpl, false, 0));
   BindParm(*new SetHowParm);
}

fixed_string SetWhatExpl = "what to set...";

SetWhatParm::SetWhatParm() : CliTextParm(SetWhatExpl)
{
   BindText(*new ToolListText, SetCommand::SetToolListIndex);
   BindText(*new BuffSizeText, SetCommand::SetBuffSizeIndex);
   BindText(*new BuffWrapText, SetCommand::SetBuffWrapIndex);
}

fixed_string SetStr = "set";
fixed_string SetExpl = "Controls trace tool settings.";

SetCommand::SetCommand(bool bind) : CliCommand(SetStr, SetExpl)
{
   if(bind) BindParm(*new SetWhatParm);
}

void SetCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

word SetCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SetCommand.ProcessCommand");

   id_t setWhatIndex;

   if(!GetTextIndex(setWhatIndex, cli)) return -1;

   return ProcessSubcommand(cli, setWhatIndex);
}

word SetCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("SetCommand.ProcessSubcommand");

   auto rc = TraceOk;
   id_t setHowIndex;
   word buffSize;
   string toolList;
   string expl;
   bool flag;
   auto buff = Singleton< TraceBuffer >::Instance();
   auto reg = Singleton< ToolRegistry >::Instance();

   switch(index)
   {
   case SetToolListIndex:
      if(!GetString(toolList, cli)) return -1;
      if(!GetTextIndex(setHowIndex, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      flag = (setHowIndex == SetHowParm::On);

      if(!ValidateOptions(toolList, reg->ListToolChars(), expl))
      {
         return cli.Report(-1, expl);
      }

      for(size_t i = 0; i < toolList.size(); ++i)
      {
         auto c = toolList[i];
         auto tool = reg->FindTool(c);
         rc = buff->SetTool(tool->Tid(), flag);
         *cli.obuf << spaces(2) << c << ": " << strTraceRc(rc) << CRLF;
      }
      break;

   case SetBuffSizeIndex:
      if(!GetIntParm(buffSize, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = buff->SetSize(buffSize);
      break;

   case SetBuffWrapIndex:
      if(!GetBoolParm(flag, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = buff->SetWrap(flag);
      break;

   default:
      return CliCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The SINGLETONS command.
//
class SingletonsCommand : public CliCommand
{
public:
   SingletonsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SingletonsStr = "singletons";
fixed_string SingletonsExpl = "Displays the singletons registry.";

SingletonsCommand::SingletonsCommand() :
   CliCommand(SingletonsStr, SingletonsExpl)
{
   BindParm(*new DispBVParm);
}

word SingletonsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SingletonsCommand.ProcessCommand");

   bool v = false;

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;
   Singletons::Instance()->Output(*cli.obuf, 2, v);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The START command.
//
class StartCommand : public CliCommand
{
public:
   StartCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string StartStr = "start";
fixed_string StartExpl = "Starts tracing.";

StartCommand::StartCommand() : CliCommand(StartStr, StartExpl) { }

word StartCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("StartCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;

   auto rc = Singleton< TraceBuffer >::Instance()->StartTracing(EMPTY_STR);
   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The STATS command.
//
class StatsShowText : public CliText
{
public: StatsShowText();
};

class RolloverText : public CliText
{
public: RolloverText();
};

class StatsAction : public CliTextParm
{
public: StatsAction();
};

class StatisticsCommand : public CliCommand
{
public:
   StatisticsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string GroupsTextStr = "groups";
fixed_string GroupsTextExpl = "lists all statistics groups";

fixed_string StatisticsGroupOptExpl = "group number (default=all)";

fixed_string MemberIdOptExpl = "member number (group specific; default=all)";

fixed_string StatsShowTextStr = "show";
fixed_string StatsShowTextExpl = "displays statistics";

StatsShowText::StatsShowText() : CliText(StatsShowTextExpl, StatsShowTextStr)
{
   BindParm(*new CliIntParm(StatisticsGroupOptExpl, 0, UINT8_MAX, true));
   BindParm(*new CliIntParm(MemberIdOptExpl, 0, UINT16_MAX, true));
   BindParm(*new DispBVParm);
   BindParm(*new OstreamOptParm);
}

fixed_string RolloverExpl = "clear history prior to this interval? (default=f)";

fixed_string RolloverTextStr = "rollover";
fixed_string RolloverTextExpl = "starts a new interval";

RolloverText::RolloverText() : CliText(RolloverTextExpl, RolloverTextStr)
{
   BindParm(*new CliBoolParm(RolloverExpl, true));
}

constexpr id_t StatsGroupsIndex = 1;
constexpr id_t StatsShowIndex = 2;
constexpr id_t StatsRolloverIndex = 3;

fixed_string StatsActionExpl = "subcommand...";

StatsAction::StatsAction() : CliTextParm(StatsActionExpl)
{
   BindText(*new CliText(GroupsTextExpl, GroupsTextStr), StatsGroupsIndex);
   BindText(*new StatsShowText, StatsShowIndex);
   BindText(*new RolloverText, StatsRolloverIndex);
}

fixed_string StatisticsStr = "stats";
fixed_string StatisticsExpl = "Supports performance statistics.";

StatisticsCommand::StatisticsCommand() :
   CliCommand(StatisticsStr, StatisticsExpl)
{
   BindParm(*new StatsAction);
}

fn_name StatisticsCommand_ProcessCommand = "StatisticsCommand.ProcessCommand";

word StatisticsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(StatisticsCommand_ProcessCommand);

   auto rc = 0;
   auto reg = Singleton< StatisticsRegistry >::Instance();
   id_t index;
   word gid, mid = 0;
   bool all, first, v = false;
   string title;
   Flags options;
   ostream* stream = cli.obuf.get();

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case StatsGroupsIndex:
      if(!cli.EndOfInput()) return -1;
      reg->Output(*cli.obuf, 2, false);
      break;

   case StatsShowIndex:
   {
      switch(GetIntParmRc(gid, cli))
      {
      case None: all = true; break;
      case Ok: all = false; break;
      default: return -1;
      }

      if(GetIntParmRc(mid, cli) == Error) return -1;
      if(GetBV(*this, cli, v) == Error) return -1;
      if(!GetFileName(title, cli)) title.clear();
      if(!cli.EndOfInput()) return -1;

      auto yield = (title.empty() ? false : cli.GenerateReportPreemptably());
      FunctionGuard guard(Guard_MakePreemptable, yield);

      if(!title.empty())
      {
         stream = cli.FileStream();
         if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      }

      options = (v ? VerboseOpt : NoFlags);

      if(all)
      {
         reg->DisplayStats(*stream, options);
      }
      else
      {
         auto group = reg->GetGroup(gid);

         if(group != nullptr)
            group->DisplayStats(*stream, mid, options);
         else
            return cli.Report(-2, NoStatsGroupExpl);
      }

      if(title.empty()) return 0;
      title += ".stats.txt";
      cli.SendToFile(title, true);
      break;
   }

   case StatsRolloverIndex:
      if(GetBoolParmRc(first, cli) == Error) return -1;
      if(!cli.EndOfInput()) return -1;
      reg->StartInterval(first);
      break;

   default:
      Debug::SwLog(StatisticsCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return rc;
}

//------------------------------------------------------------------------------
//
//  The STATUS command.
//
fixed_string StatusStr = "status";
fixed_string StatusExpl = "Displays system statistics.";

StatusCommand::StatusCommand() : CliCommand(StatusStr, StatusExpl) { }

void StatusCommand::Patch(sel_t selector, void* arguments)
{
   CliCommand::Patch(selector, arguments);
}

fixed_string HeapsHeader =
"Alloc  Low kB     kB       Bytes                            Memory        Prot\n"
"Fails   Avail  Avail      In Use     Allocs      Frees        Type  RWX  Chngs";
//        1         2         3         4         5         6         7
//234567890123456789012345678901234567890123456789012345678901234567890123456789

fixed_string PoolsHeader =
   "Alloc  Lowest    Curr    Curr\n"
   "Fails   Avail   Avail  In Use     Allocs      Frees  Expands   Pool Name";
// 0         1         2         3         4         5         6         7
// 01234567890123456789012345678901234567890123456789012345678901234567890123456

word StatusCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("StatusCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;

   *cli.obuf << "STATUS REPORT: " << Element::strTimePlace() << CRLF;
   *cli.obuf << "MEMORY USAGE" << CRLF;
   *cli.obuf << HeapsHeader << CRLF;

   for(auto m = 0; m < MemoryType_N; ++m)
   {
      auto heap = Memory::AccessHeap(MemoryType(m));

      if(heap != nullptr)
      {
         *cli.obuf << setw(5) << heap->FailCount();

         auto size = heap->Size();
         if(size == 0)
         {
            *cli.obuf << setw(7) << "---";
            *cli.obuf << setw(8) << "---";
         }
         else
         {
            *cli.obuf << setw(7) << ((size - heap->MaxBytesInUse()) / kBs);
            *cli.obuf << setw(8) << ((size - heap->BytesInUse()) / kBs);
         }

         *cli.obuf << setw(12) << heap->BytesInUse();
         *cli.obuf << setw(11) << heap->AllocCount();
         *cli.obuf << setw(11) << heap->FreeCount();
         *cli.obuf << setw(12) << heap->Type();
         *cli.obuf << setw(5) << heap->GetAttrs();
         *cli.obuf << setw(7) << heap->ChangeCount() << CRLF;
      }
   }

   *cli.obuf << CRLF;
   *cli.obuf << "OBJECT POOLS" << CRLF;
   *cli.obuf << PoolsHeader << CRLF;

   auto& objpools = Singleton< ObjectPoolRegistry >::Instance()->Pools();

   for(auto p = objpools.First(); p != nullptr; objpools.Next(p))
   {
      auto low = p->LowAvailCount();

      *cli.obuf << setw(5) << p->FailCount();
      if(low == LowWatermark::Initial)
         *cli.obuf << setw(8) << '*';
      else
         *cli.obuf << setw(8) << low;
      *cli.obuf << setw(8) << p->AvailCount();
      *cli.obuf << setw(8) << p->InUseCount();
      *cli.obuf << setw(11) << p->AllocCount();
      *cli.obuf << setw(11) << p->FreeCount();
      *cli.obuf << setw(9) << p->Expansions();
      *cli.obuf << spaces(3) << p->Name() << CRLF;
   }

   *cli.obuf << CRLF;
   *cli.obuf << std::setprecision(1) << std::fixed;
   *cli.obuf << "CPU IDLE TIME: " << Thread::PercentIdle() << '%' << CRLF;

   *cli.obuf << CRLF;
   *cli.obuf << "ACTIVE ALARMS" << CRLF;

   auto prefix = spaces(2);
   auto active = false;
   auto& alarms = Singleton< AlarmRegistry >::Instance()->Alarms();

   for(auto a = alarms.First(); a != nullptr; alarms.Next(a))
   {
      if(a->Status() != NoAlarm)
      {
         a->Display(*cli.obuf, prefix, NoFlags);
         active = true;
      }
   }

   if(!active) *cli.obuf << prefix << "No active alarms." << CRLF;
   return 0;
}

//------------------------------------------------------------------------------
//
//  The STOP command.
//
class StopCommand : public CliCommand
{
public:
   StopCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string StopStr = "stop";
fixed_string StopExpl = "Stops tracing.";

StopCommand::StopCommand() : CliCommand(StopStr, StopExpl) { }

word StopCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("StopCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;
   Singleton< TraceBuffer >::Instance()->StopTracing();
   return ExplainTraceRc(cli, TraceOk);
}

//------------------------------------------------------------------------------
//
//  The SYMBOLS command.
//
class SymbolsListText : public CliText
{
public: SymbolsListText();
};

class SymbolsSetText : public CliText
{
public: SymbolsSetText();
};

class SymbolsAssignText : public CliText
{
public: SymbolsAssignText();
};

class SymbolsAction : public CliTextParm
{
public: SymbolsAction();
};

class SymbolsCommand : public CliCommand
{
public:
   SymbolsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SymbolOptNameExpl = "symbol's name (lists all if omitted)";

fixed_string SymbolMandNameExpl = "symbol's name";

fixed_string SymbolValueExpl = "symbol's value (symbol deleted if omitted)";

fixed_string SymbolsListStr = "list";
fixed_string SymbolsListExpl = "lists symbols";

SymbolsListText::SymbolsListText() : CliText(SymbolsListExpl, SymbolsListStr)
{
   BindParm(*new CliTextParm(SymbolOptNameExpl, true, 0));
}

fixed_string SymbolsSetStr = "set";
fixed_string SymbolsSetExpl = "sets a symbol's value";

SymbolsSetText::SymbolsSetText() : CliText(SymbolsSetExpl, SymbolsSetStr)
{
   BindParm(*new CliTextParm(SymbolMandNameExpl, false, 0));
   BindParm(*new CliTextParm(SymbolValueExpl, true, 0));
}

fixed_string SymbolsAssignStr = "assign";
fixed_string SymbolsAssignsExpl =
   "sets a symbol's value to a configuration parameter's";

SymbolsAssignText::SymbolsAssignText() :
   CliText(SymbolsAssignsExpl, SymbolsAssignStr)
{
   BindParm(*new CliTextParm(SymbolMandNameExpl, false, 0));
   BindParm(*new CliTextParm(CfgParmNameExpl, false, 0));
}

constexpr id_t SymbolsListIndex = 1;
constexpr id_t SymbolsSetIndex = 2;
constexpr id_t SymbolsAssignIndex = 3;

fixed_string SymbolsActionExpl = "subcommand...";

SymbolsAction::SymbolsAction() : CliTextParm(SymbolsActionExpl)
{
   BindText(*new SymbolsListText, SymbolsListIndex);
   BindText(*new SymbolsSetText, SymbolsSetIndex);
   BindText(*new SymbolsAssignText, SymbolsAssignIndex);
}

fixed_string SymbolsStr = "symbols";
fixed_string SymbolsExpl = "Supports symbols.";

SymbolsCommand::SymbolsCommand() : CliCommand(SymbolsStr, SymbolsExpl)
{
   BindParm(*new SymbolsAction);
}

fn_name SymbolsCommand_ProcessCommand = "SymbolsCommand.ProcessCommand";

word SymbolsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(SymbolsCommand_ProcessCommand);

   id_t index;
   string name, value, key;
   bool all, del;
   auto preg = Singleton< CfgParmRegistry >::Instance();
   auto sreg = Singleton< SymbolRegistry >::Instance();
   auto& syms = sreg->Symbols();
   Symbol* sym;
   size_t count = 0;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case SymbolsListIndex:
      all = (GetStringRc(name, cli) == None);
      if(!cli.EndOfInput()) return -1;

      if(!all)
      {
         sym = sreg->FindSymbol(name);
         if(sym == nullptr) return cli.Report(-2, NoSymbolExpl);
         return cli.Report(0, sym->GetValue());
      }

      for(sym = syms.First(); sym != nullptr; syms.Next(sym))
      {
         *cli.obuf << spaces(2);

         if(sym->IsLocked())
            *cli.obuf << "# ";
         else
            *cli.obuf << "  ";

         *cli.obuf << sym->Name() << " : " << sym->GetValue() << CRLF;

         if(++count >= 10)
         {
            ThisThread::PauseOver(90);
            count = 1;
         }
      }

      if(count == 0) return cli.Report(-2, NoSymbolExpl);
      break;

   case SymbolsSetIndex:
      if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
         Symbol::InvalidInitialChars())) return -1;
      del = (GetStringRc(value, cli) == None);
      if(!cli.EndOfInput()) return -1;

      if(del)
      {
         sym = sreg->FindSymbol(name);
         if(sym == nullptr) return cli.Report(-2, NoSymbolExpl);
         if(sym->IsLocked()) return cli.Report(-4, SymbolLockedExpl);
         delete sym;
         sym = nullptr;
      }
      else
      {
         sym = sreg->EnsureSymbol(name);
         if(sym == nullptr) return cli.Report(-7, SymbolOverflowExpl);
         if(!sym->SetValue(value, false))
            return cli.Report(-4, SymbolLockedExpl);
      }

      return cli.Report(0, SuccessExpl);

   case SymbolsAssignIndex:
      if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
         Symbol::InvalidInitialChars())) return -1;
      if(!GetString(key, cli)) return -1;
      if(!cli.EndOfInput()) return -1;

      sym = sreg->EnsureSymbol(name);
      if(sym == nullptr) return cli.Report(-7, SymbolOverflowExpl);
      if(!preg->GetValue(key, value)) return cli.Report(-2, NoCfgParmExpl);
      if(!sym->SetValue(value, false)) return cli.Report(-4, SymbolLockedExpl);
      return cli.Report(0, SuccessExpl);

   default:
      Debug::SwLog(SymbolsCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The THREADS command.
//
class ThreadsCommand : public CliCommand
{
public:
   ThreadsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ThreadsStr = "threads";
fixed_string ThreadsExpl = "Counts or displays threads.";

ThreadsCommand::ThreadsCommand() : CliCommand(ThreadsStr, ThreadsExpl)
{
   BindParm(*new ThreadIdOptParm);
   BindParm(*new DispCBVParm);
}

word ThreadsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ThreadsCommand.ProcessCommand");

   word tid;
   bool all, c, v;

   switch(GetIntParmRc(tid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto size = ThreadRegistry::Size();
   auto reg = Singleton< ThreadRegistry >::Instance();

   if(c)
   {
      *cli.obuf << spaces(2) << size << CRLF;
   }
   else if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto thr = reg->GetThread(tid);
      if(thr == nullptr) return cli.Report(-2, NoThreadExpl);
      thr->Output(*cli.obuf, 2, v);
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The TOOLS command.
//
fixed_string ToolHeaderStr = "  Tool Name          Abbr  Explanation";
//                           0         1         2        3
//                           012345678901234567890134567890123456789

class ToolsCommand : public CliCommand
{
public:
   ToolsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ToolsStr = "tools";
fixed_string ToolsExpl = "Lists available debugging tools.";

ToolsCommand::ToolsCommand() : CliCommand(ToolsStr, ToolsExpl) { }

word ToolsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ToolsCommand.ProcessCommand");

   auto& tools = Singleton< ToolRegistry >::Instance()->Tools();

   //  Display the available tools.  If a tool's CLI character is not
   //  printable, it is not supported through the CLI.  If a tool is
   //  not field-safe, only display it in the lab.
   //
   *cli.obuf << ToolHeaderStr << CRLF;

   for(auto t = tools.First(); t != nullptr; tools.Next(t))
   {
      auto c = t->CliChar();
      if(!isprint(c)) continue;
      if(!t->IsSafe()) continue;

      string name(t->Name());
      if(name.size() > 17) name.erase(17);
      *cli.obuf << spaces(2) << std::left << setw(17) << name;
      *cli.obuf << spaces(2) << std::right << setw(4) << c;

      string expl(t->Expl());
      if(expl.size() > 52) expl.erase(52);
      *cli.obuf << spaces(2) << std::left << expl << CRLF;
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The NodeBase increment.
//
fixed_string RootStr = "nb";
fixed_string RootExpl = "NodeBase Increment";

NbIncrement::NbIncrement() : CliIncrement(RootStr, RootExpl, 48)
{
   Debug::ft("NbIncrement.ctor");

   BindCommand(*new HelpCommand);
   BindCommand(*new QuitCommand);
   BindCommand(*new IncrsCommand);
   BindCommand(*new SendCommand);
   BindCommand(*new ReadCommand);
   BindCommand(*new EchoCommand);
   BindCommand(*new CfgParmsCommand);
   BindCommand(*new LogsCommand);
   BindCommand(*new AlarmsCommand);
   BindCommand(*new SymbolsCommand);
   BindCommand(*new StatisticsCommand);
   BindCommand(*new ModulesCommand);
   BindCommand(*new PoolsCommand);
   BindCommand(*new AuditCommand);
   BindCommand(*new SchedCommand);
   BindCommand(*new ThreadsCommand);
   BindCommand(*new DaemonsCommand);
   BindCommand(*new MutexesCommand);
   BindCommand(*new BuffersCommand);
   BindCommand(*new PsignalsCommand);
   BindCommand(*new SingletonsCommand);
   BindCommand(*new HeapsCommand);
   BindCommand(*new StatusCommand);
   BindCommand(*new ToolsCommand);
   BindCommand(*new SetCommand);
   BindCommand(*new IncludeCommand);
   BindCommand(*new ExcludeCommand);
   BindCommand(*new QueryCommand);
   BindCommand(*new ClearCommand);
   BindCommand(*new StartCommand);
   BindCommand(*new StopCommand);
   BindCommand(*new SaveCommand);
   BindCommand(*new IfCommand);
   BindCommand(*new DelayCommand);
   BindCommand(*new DisplayCommand);
   BindCommand(*new DumpCommand);
   BindCommand(*new RestartCommand);
}

//------------------------------------------------------------------------------

NbIncrement::~NbIncrement()
{
   Debug::ftnt("NbIncrement.dtor");
}

//------------------------------------------------------------------------------

void NbIncrement::Patch(sel_t selector, void* arguments)
{
   CliIncrement::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void NbIncrement::Startup(RestartLevel level)
{
   Debug::ftnt("NbIncrement.Startup");

   CliIncrement::Startup(level);

   //  Define symbols related to memory types.
   //
   auto reg = Singleton< SymbolRegistry >::Instance();
   reg->BindSymbol("mem.temp", MemTemporary);
   reg->BindSymbol("mem.dyn", MemDynamic);
   reg->BindSymbol("mem.pers", MemPersistent);
   reg->BindSymbol("mem.prot", MemProtected);
   reg->BindSymbol("mem.perm", MemPermanent);
   reg->BindSymbol("mem.imm", MemImmutable);
}
}
