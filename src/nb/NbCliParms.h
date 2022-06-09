//==============================================================================
//
//  NbCliParms.h
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
#ifndef NBCLIPARMS_H_INCLUDED
#define NBCLIPARMS_H_INCLUDED

#include "CliCharParm.h"
#include "CliIntParm.h"
#include "CliText.h"
#include "CliTextParm.h"
#include <string>
#include "SysTypes.h"
#include "ToolTypes.h"

namespace NodeBase
{
   class CliCommand;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Strings used by commands in the NodeBase increment.
//
extern fixed_string AllocationError;
extern fixed_string AlreadyInIncrement;
extern fixed_string BadObjectPtrWarning;
extern fixed_string BadParameterValue;
extern fixed_string CommandAbortedExpl;
extern fixed_string ConsoleAutomaticExpl;
extern fixed_string ContinuePrompt;
extern fixed_string CreateStreamFailure;
extern fixed_string DelayFailure;
extern fixed_string EmptySet;
extern fixed_string EndOfFreeQueue;
extern fixed_string NoAlarmExpl;
extern fixed_string NoBuffersExpl;
extern fixed_string NoCfgParmExpl;
extern fixed_string NoCommandExpl;
extern fixed_string NoDaemonExpl;
extern fixed_string NoDiscardsExpl;
extern fixed_string NoFileExpl;
extern fixed_string NoIncrExpl;
extern fixed_string NoLogExpl;
extern fixed_string NoLogGroupExpl;
extern fixed_string NoModuleExpl;
extern fixed_string NoMutexExpl;
extern fixed_string NoPoolExpl;
extern fixed_string NoPosixSignalExpl;
extern fixed_string NoStatsGroupExpl;
extern fixed_string NoSymbolExpl;
extern fixed_string NoThreadExpl;
extern fixed_string NoToolExpl;
extern fixed_string NotImplementedExpl;
extern fixed_string NotInFieldExpl;
extern fixed_string NullPtrInvalid;
extern fixed_string ParameterIgnored;
extern fixed_string ParameterInvalid;
extern fixed_string RestartWarning;
extern fixed_string ReturnFalse;
extern fixed_string ReturnTrue;
extern fixed_string SendingToConsoleExpl;
extern fixed_string SizesHeader;
extern fixed_string StopTracingPrompt;
extern fixed_string SuccessExpl;
extern fixed_string SymbolLockedExpl;
extern fixed_string SymbolOverflowExpl;
extern fixed_string SystemErrorExpl;
extern fixed_string TestFailedExpl;
extern fixed_string TooManyInputStreams;
extern fixed_string TooManyOutputStreams;
extern fixed_string TraceReportPrompt;
extern fixed_string UnknownSignalExpl;

//------------------------------------------------------------------------------
//
//  Optional parameter for specifying whether to display an object briefly (the
//  default) or verbosely.
//
class DispBVParm : public CliCharParm
{
public: DispBVParm();
};

//  Obtains the value of a DispBVParm.  COMM is the command invoking this
//  function, and CLI is the CLI thread.  Sets V to true if a "v" was entered
//  Returns the result of GetCharParmRc.
//
CliParm::Rc GetBV(const CliCommand& comm, CliThread& cli, bool& v);

//------------------------------------------------------------------------------
//
//  Optional parameter for specifying whether to display the number of objects
//  of a given type or to display them briefly (the default) or verbosely.
//
class DispCBVParm : public CliCharParm
{
public: DispCBVParm();
};

//  Obtains the value of a DispCBVParm (DISP).  COMM is the command invoking
//  this function, and CLI is the CLI thread.  Returns false on invalid input.
//
bool GetDisp(const CliCommand& comm, CliThread& cli, char& disp);

//------------------------------------------------------------------------------
//
//  Optional parameter for specifying whether to display
//    c: the number of objects in a registry
//    s: a one-line summary of each object in a registry
//    b: briefly (a registry's objects)
//    v: verbosely (a registry's objects)
//  It is followed by an optional identifier.  If no identifier or 0 (the nil
//  identifier) is entered, the default is 's'.  If an identifier is provided,
//  the default is 'v' when displaying the object with that identifier.
//
class DispCSBVParm : public CliCharParm
{
public: DispCSBVParm();
};

//  Obtains the value of an optional identifier (ID) and DispCSBVParm (DISP).
//  COMM is the command invoking this function, and CLI is the CLI thread.
//  Sets ID to 0 if no identifier was found, in which case C defaults to 's'.
//  If an identifier was found, C defaults to 'v' if not found.  Returns
//  false on invalid input.
//
bool GetIdAndDisp(const CliCommand& comm, CliThread& cli, word& id, char& disp);

//------------------------------------------------------------------------------
//
//  If a character in OPTS does not appear in VALID, returns false and update
//  EXPL with an error message and list of invalid characters.  Returns true
//  if all of the characters in OPTS appear in VALID.
//
bool ValidateOptions
   (const std::string& opts, const std::string& valid, std::string& expl);

//------------------------------------------------------------------------------
//
//  Explains a TraceRc result.  Usage is "return ExplainTraceRc(cli, rc)".
//
word ExplainTraceRc(const CliThread& cli, TraceRc rc);

//------------------------------------------------------------------------------
//
//  Parameters for a Faction.
//
class FactionMandParm : public CliIntParm
{
public: FactionMandParm();
};

class FactionOptParm : public CliIntParm
{
public: FactionOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for an id_t.
//
class IdMandParm : public CliIntParm
{
public: IdMandParm();
};

class IdOptParm : public CliIntParm
{
public: IdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a mandatory filename for input.
//
class IstreamMandParm : public CliTextParm
{
public: IstreamMandParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a LogBuffer.
//
class LogBufferIdParm : public CliIntParm
{
public: LogBufferIdParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a LogGroup.
//
class LogGroupMandParm : public CliTextParm
{
public: LogGroupMandParm();
};

class LogGroupOptParm : public CliTextParm
{
public: LogGroupOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a mandatory LogId.
//
class LogIdMandParm : public CliIntParm
{
public: LogIdMandParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a MemoryType.
//
class MemoryTypeMandParm : public CliIntParm
{
public: MemoryTypeMandParm();
};

class MemoryTypeOptParm : public CliIntParm
{
public: MemoryTypeOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a ModuleId.
//
class ModuleIdOptParm : public CliIntParm
{
public: ModuleIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for an ObjectPoolId.
//
class ObjPoolIdMandParm : public CliIntParm
{
public: ObjPoolIdMandParm();
};

class ObjPoolIdOptParm : public CliIntParm
{
public: ObjPoolIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters for a filename for output.
//
class OstreamMandParm : public CliTextParm
{
public: OstreamMandParm();
};

class OstreamOptParm : public CliTextParm
{
public: OstreamOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an optional signal_t.
//
class PosixSignalParm : public CliIntParm
{
public: PosixSignalParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a relational operator.
//
class RelationParm : public CliTextParm
{
public:
   RelationParm();

   //  Values for the parameter.
   //
   static const id_t Lt = 1;
   static const id_t LtEq = 2;
   static const id_t Eq = 3;
   static const id_t NEq = 4;
   static const id_t Gt = 5;
   static const id_t GtEq = 6;
};

//------------------------------------------------------------------------------
//
//  Parameter for setting a value to ON or OFF.
//
class SetHowParm : public CliTextParm
{
public:
   SetHowParm();

   //  Values for the parameter.
   //
   static const id_t On = 1;
   static const id_t Off = 2;
};

//------------------------------------------------------------------------------
//
//  Parameters for a ThreadId.
//
class ThreadIdMandParm : public CliIntParm
{
public: ThreadIdMandParm();
};

class ThreadIdOptParm : public CliIntParm
{
public: ThreadIdOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameters that support trace tools.
//
class AllActivityText : public CliText
{
public: AllActivityText();
};

class BufferText : public CliText
{
public: BufferText();
};

class FactionText : public CliText
{
public: FactionText();
};

class FactionsText : public CliText
{
public: FactionsText();
};

class SelectionsText : public CliText
{
public: SelectionsText();
};

class ThreadText : public CliText
{
public: ThreadText();
};

class ThreadsText : public CliText
{
public: ThreadsText();
};

class ToolsText : public CliText
{
public: ToolsText();
};
}
#endif
