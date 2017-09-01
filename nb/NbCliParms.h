//==============================================================================
//
//  NbCliParms.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NBCLIPARMS_H_INCLUDED
#define NBCLIPARMS_H_INCLUDED

#include "CliCharParm.h"
#include "CliIntParm.h"
#include "CliPtrParm.h"
#include "CliText.h"
#include "CliTextParm.h"
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
//e Support localization.
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
extern fixed_string NextRestartExpl;
extern fixed_string NoBuffersExpl;
extern fixed_string NoCfgParmExpl;
extern fixed_string NoCommandExpl;
extern fixed_string NoDiscardsExpl;
extern fixed_string NoFileExpl;
extern fixed_string NoIncrExpl;
extern fixed_string NoModuleExpl;
extern fixed_string NoPoolExpl;
extern fixed_string NoStatsGroupExpl;
extern fixed_string NoSymbolExpl;
extern fixed_string NoThreadExpl;
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
extern fixed_string StopTracePrompt;
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

//  Free function for obtaining the value of a DispBVParm.  COMM is the command
//  invoking this function, and CLI is the CLI thread.  Sets V to true if a "v"
//  was entered.  Returns the result of GetCharParmRc.
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

//  Free function for obtaining the value of a DispCBVParm.  COMM is the command
//  invoking this function, and CLI is the CLI thread.  Sets V to true if a "v"
//  was entered, and C to true if a "c" was entered.  Returns the result of
//  GetCharParmRc.
//
CliParm::Rc GetCBV(const CliCommand& comm, CliThread& cli, bool& c, bool& v);

//------------------------------------------------------------------------------
//
//  Free function for explaining a TraceRc result.
//
word ExplainTraceRc(CliThread& cli, TraceRc rc);

//------------------------------------------------------------------------------
//
//  Parameter for a mandatory Faction.
//
class FactionMandParm : public CliIntParm
{
public: FactionMandParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an optional Faction.
//
class FactionOptParm : public CliIntParm
{
public: FactionOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for a mandatory filename.
//
class FileMandParm : public CliTextParm
{
public: FileMandParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for an optional filename.
//
class FileOptParm : public CliTextParm
{
public: FileOptParm();
};

//------------------------------------------------------------------------------
//
//  Parameter for ModuleId.
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
//  Parameter for a pointer to an object.
//
class ObjPtrMandParm : public CliPtrParm
{
public: ObjPtrMandParm();
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
//  Parameters for time.
//
class SysTimeYearParm : public CliIntParm
{
public: SysTimeYearParm();
};

class SysTimeMonthParm : public CliIntParm
{
public: SysTimeMonthParm();
};

class SysTimeDayParm : public CliIntParm
{
public: SysTimeDayParm();
};

class SysTimeHourParm : public CliIntParm
{
public: SysTimeHourParm();
};

class SysTimeMinuteParm : public CliIntParm
{
public: SysTimeMinuteParm();
};

class SysTimeSecondParm : public CliIntParm
{
public: SysTimeSecondParm();
};

class SysTimeMsecondParm : public CliIntParm
{
public: SysTimeMsecondParm();
};

class SysTimeFieldParm : public CliTextParm
{
public: SysTimeFieldParm();
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
