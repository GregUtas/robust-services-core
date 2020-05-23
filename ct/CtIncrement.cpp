//==============================================================================
//
//  CtIncrement.cpp
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
#include "CtIncrement.h"
#include "CliBoolParm.h"
#include "CliCommand.h"
#include "CliIntParm.h"
#include "CliText.h"
#include "CliTextParm.h"
#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include "CliBuffer.h"
#include "CliThread.h"
#include "CodeCoverage.h"
#include "CodeDir.h"
#include "CodeFile.h"
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxSymbols.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Library.h"
#include "LibrarySet.h"
#include "LibraryTypes.h"
#include "NbCliParms.h"
#include "Parser.h"
#include "Registry.h"
#include "Singleton.h"
#include "Symbol.h"
#include "SysFile.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Parameters used by more than one command.
//
class CodeSetExprParm : public CliTextParm
{
public: CodeSetExprParm();
};

class FileSetExprParm : public CliTextParm
{
public: FileSetExprParm();
};

class SetExprParm : public CliTextParm
{
public: SetExprParm();
};

class VarMandName : public CliTextParm
{
public: VarMandName();
};

fixed_string CodeSetExprExpl = "a set of code files or directories";

CodeSetExprParm::CodeSetExprParm() : CliTextParm(CodeSetExprExpl, false, 0) { }

fixed_string FileSetExprExpl = "a set of code files";

FileSetExprParm::FileSetExprParm() : CliTextParm(FileSetExprExpl, false, 0) { }

fixed_string SetExprExpl = "a set of code files or directories";

SetExprParm::SetExprParm() : CliTextParm(SetExprExpl, false, 0) { }

fixed_string VarMandNameExpl = "variable name";

VarMandName::VarMandName() : CliTextParm(VarMandNameExpl, false, 0) { }

//------------------------------------------------------------------------------
//
//  Base class for library commands that evaluate an expression.
//
class LibraryCommand : public CliCommand
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~LibraryCommand() = default;

   //  Reads the rest of the input line and returns the result of
   //  evaluating it.
   //
   static LibrarySet* Evaluate(const CliThread& cli);
protected:
   //  The arguments are from the base class.  Protected because this
   //  class is virtual.
   //
   LibraryCommand(c_string comm, c_string help);
};

LibraryCommand::LibraryCommand(c_string comm, c_string help) :
   CliCommand(comm, help) { }

fn_name LibraryCommand_Evaluate = "LibraryCommand.Evaluate";

LibrarySet* LibraryCommand::Evaluate(const CliThread& cli)
{
   Debug::ft(LibraryCommand_Evaluate);

   string expr;

   auto pos = cli.Prompt().size() + cli.ibuf->Pos();
   cli.ibuf->Read(expr);
   if(!cli.EndOfInput()) return nullptr;

   auto result = Singleton< Library >::Instance()->Evaluate(expr, pos);
   return result;
}

//------------------------------------------------------------------------------
//
//  The ASSIGN command.
//
class AssignCommand : public CliCommand
{
public:
   AssignCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string AssignStr = "assign";
fixed_string AssignExpl =
   "Assigns a set of files or directories to a variable.";

AssignCommand::AssignCommand() : CliCommand(AssignStr, AssignExpl)
{
   BindParm(*new VarMandName);
   BindParm(*new CodeSetExprParm);
}

fn_name AssignCommand_ProcessCommand = "AssignCommand.ProcessCommand";

word AssignCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(AssignCommand_ProcessCommand);

   string name, expr, expl;

   if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
      Symbol::InvalidInitialChars())) return -1;
   auto pos = cli.Prompt().size() + cli.ibuf->Pos();
   cli.ibuf->Read(expr);
   if(!cli.EndOfInput()) return -1;

   auto rc = Singleton< Library >::Instance()->Assign(name, expr, pos, expl);
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The CHECK command.
//
class CheckCommand : public LibraryCommand
{
public:
   CheckCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CheckStr = "check";
fixed_string CheckExpl = "Checks if code follows C++ guidelines.";

CheckCommand::CheckCommand() : LibraryCommand(CheckStr, CheckExpl)
{
   BindParm(*new OstreamMandParm);
   BindParm(*new FileSetExprParm);
}

fn_name CheckCommand_ProcessCommand = "CheckCommand.ProcessCommand";

word CheckCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CheckCommand_ProcessCommand);

   string title;

   if(!GetFileName(title, cli)) return -1;

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return -1;

   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   string expl;
   auto rc = set->Check(cli, stream, expl);
   set->Release();

   if(rc == 0)
   {
      title += ".check.txt";
      cli.SendToFile(title, true);
   }

   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The COVERAGE command.
//
class CoverageLoadText : public CliText
{
public: CoverageLoadText();
};

fixed_string CoverageLoadTextStr = "load";
fixed_string CoverageLoadTextExpl =
   "reads the database from InputPath/coverage.db.txt";

CoverageLoadText::CoverageLoadText() :
   CliText(CoverageLoadTextExpl, CoverageLoadTextStr) { }

class CoverageQueryText : public CliText
{
public: CoverageQueryText();
};

fixed_string CoverageQueryTextStr = "query";
fixed_string CoverageQueryTextExpl =
   "displays information about the loaded database";

CoverageQueryText::CoverageQueryText() :
   CliText(CoverageQueryTextExpl, CoverageQueryTextStr) { }

class CoverageUnderText : public CliText
{
public: CoverageUnderText();
};

class MinTestsParm : public CliIntParm
{
public: MinTestsParm();
};

fixed_string MinTestsParmExpl = "value of N";

MinTestsParm::MinTestsParm() :
   CliIntParm(MinTestsParmExpl, 1, 10) { }

fixed_string CoverageUnderTextStr = "under";
fixed_string CoverageUnderTextExpl =
   "lists functions invoked by fewer than N testcases";

CoverageUnderText::CoverageUnderText() :
   CliText(CoverageUnderTextExpl, CoverageUnderTextStr)
{
   BindParm(*new MinTestsParm);
}

class CoverageEraseText : public CliText
{
public: CoverageEraseText();
};

class FuncNameParm : public CliTextParm
{
public: FuncNameParm();
};

fixed_string FuncNameParmExpl = "name of function to remove";

FuncNameParm::FuncNameParm() : CliTextParm(FuncNameParmExpl, false, 0) { }

fixed_string CoverageEraseTextStr = "erase";
fixed_string CoverageEraseTextExpl = "removes a function from the database";

CoverageEraseText::CoverageEraseText() :
   CliText(CoverageEraseTextExpl, CoverageEraseTextStr)
{
   BindParm(*new FuncNameParm);
}

class CoverageUpdateText : public CliText
{
public: CoverageUpdateText();
};

fixed_string CoverageUpdateStr = "update";
fixed_string CoverageUpdateExpl =
   "updates database with modified functions and rerun tests";

CoverageUpdateText::CoverageUpdateText() :
   CliText(CoverageUpdateExpl, CoverageUpdateStr) { }

class CoverageAction : public CliTextParm
{
public: CoverageAction();
};

const id_t CoverageLoadIndex = 1;
const id_t CoverageQueryIndex = 2;
const id_t CoverageUnderIndex = 3;
const id_t CoverageEraseIndex = 4;
const id_t CoverageUpdateIndex = 5;

fixed_string CoverageActionExpl = "subcommand...";

CoverageAction::CoverageAction() : CliTextParm(CoverageActionExpl)
{
   BindText(*new CoverageLoadText, CoverageLoadIndex);
   BindText(*new CoverageQueryText, CoverageQueryIndex);
   BindText(*new CoverageUnderText, CoverageUnderIndex);
   BindText(*new CoverageEraseText, CoverageEraseIndex);
   BindText(*new CoverageUpdateText, CoverageUpdateIndex);
}

class CoverageCommand : public LibraryCommand
{
public:
   CoverageCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CoverageStr = "coverage";
fixed_string CoverageExpl = "Supports code coverage.";

CoverageCommand::CoverageCommand() : LibraryCommand(CoverageStr, CoverageExpl)
{
   BindParm(*new CoverageAction);
}

fn_name CoverageCommand_ProcessCommand = "CoverageCommand.ProcessCommand";

word CoverageCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CoverageCommand_ProcessCommand);

   auto database = Singleton< CodeCoverage >::Instance();
   id_t index;
   word min;
   string name, path, expl;
   word rc;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case CoverageLoadIndex:
      if(!cli.EndOfInput()) return -1;
      rc = database->Load(expl);
      break;

   case CoverageQueryIndex:
      rc = database->Query(expl);
      break;

   case CoverageUnderIndex:
      if(!GetIntParm(min, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = database->Under(min, expl);
      break;

   case CoverageEraseIndex:
      if(!GetString(name, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = database->Erase(name, expl);
      break;

   case CoverageUpdateIndex:
      if(!cli.EndOfInput()) return -1;
      rc = database->Update(expl);
      break;

   default:
      return cli.Report(index, SystemErrorExpl);
   }

   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The COUNT command.
//
class CountCommand : public LibraryCommand
{
public:
   CountCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CountStr = "count";
fixed_string CountExpl = "Counts the items in a set.";

CountCommand::CountCommand() : LibraryCommand(CountStr, CountExpl)
{
   BindParm(*new SetExprParm);
}

fn_name CountCommand_ProcessCommand = "CountCommand.ProcessCommand[>ct]";

word CountCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CountCommand_ProcessCommand);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string result;

   auto rc = set->Count(result);
   set->Release();
   return cli.Report(rc, result);
}

//------------------------------------------------------------------------------
//
//  The COUNTLINES command.
//
class CountlinesCommand : public LibraryCommand
{
public:
   CountlinesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CountlinesStr = "countlines";
fixed_string CountlinesExpl = "Counts the number of lines of code.";

CountlinesCommand::CountlinesCommand() :
   LibraryCommand(CountlinesStr, CountlinesExpl)
{
   BindParm(*new FileSetExprParm);
}

fn_name CountlinesCommand_ProcessCommand = "CountlinesCommand.ProcessCommand";

word CountlinesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CountlinesCommand_ProcessCommand);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string result;

   auto rc = set->Countlines(result);
   set->Release();
   return cli.Report(rc, result);
}

//------------------------------------------------------------------------------
//
//  The EXPLAIN command.
//
class WarningIdParm : public CliIntParm
{
public: WarningIdParm();
};

fixed_string WarningIdExpl = "warning number";

WarningIdParm::WarningIdParm() : CliIntParm(WarningIdExpl, 1, Warning_N - 1) { }

class ExplainCommand : public CliCommand
{
public:
   ExplainCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ExplainStr = "explain";
fixed_string ExplainExpl = "Explains a warning generated by >check.";

ExplainCommand::ExplainCommand() : CliCommand(ExplainStr, ExplainExpl)
{
   BindParm(*new WarningIdParm);
}

fn_name ExplainCommand_ProcessCommand = "ExplainCommand.ProcessCommand";

word ExplainCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ExplainCommand_ProcessCommand);

   word id;

   if(!GetIntParm(id, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   string key = 'W' + std::to_string(id);
   auto path = Element::HelpPath() + PATH_SEPARATOR + "cppcheck.txt";
   auto rc = cli.DisplayHelp(path, key);

   switch(rc)
   {
   case -1:
      return cli.Report(-1, "This warning has not been documented.");
   case -2:
      return cli.Report(-2, "Failed to open file " + path);
   }

   return rc;
}

//------------------------------------------------------------------------------
//
//  The EXPORT command.
//
class ViewsParm : public CliTextParm
{
public: ViewsParm();
};

fixed_string ViewsExpl = "options (enter \">help export full\" for details)";

ViewsParm::ViewsParm() : CliTextParm(ViewsExpl, true, 0) { }

class ExportCommand : public CliCommand
{
public:
   ExportCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ExportStr = "export";
fixed_string ExportExpl = "Exports library information.";

ExportCommand::ExportCommand() : CliCommand(ExportStr, ExportExpl)
{
   BindParm(*new OstreamMandParm);
   BindParm(*new ViewsParm);
}

const string& DefaultExportOptions()
{
   static string DefaultOpts;

   if(DefaultOpts.empty())
   {
      DefaultOpts.push_back(NamespaceView);
      DefaultOpts.push_back(CanonicalFileView);
      DefaultOpts.push_back(ClassHierarchyView);
      DefaultOpts.push_back(ItemStatistics);
      DefaultOpts.push_back(FileSymbolUsage);
      DefaultOpts.push_back(GlobalCrossReference);
   }

   return DefaultOpts;
}

const string& ValidExportOptions()
{
   static string ValidOpts;

   if(ValidOpts.empty())
   {
      ValidOpts.push_back(NamespaceView);
      ValidOpts.push_back(CanonicalFileView);
      ValidOpts.push_back(OriginalFileView);
      ValidOpts.push_back(ClassHierarchyView);
      ValidOpts.push_back(ItemStatistics);
      ValidOpts.push_back(FileSymbolUsage);
      ValidOpts.push_back(GlobalCrossReference);
   }

   return ValidOpts;
}

fn_name ExportCommand_ProcessCommand = "ExportCommand.ProcessCommand";

word ExportCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ExportCommand_ProcessCommand);

   string title;
   string opts;
   string expl;

   if(!GetFileName(title, cli)) return -1;
   GetString(opts, cli);
   if(!cli.EndOfInput()) return -1;

   if(opts.empty())
   {
      opts = DefaultExportOptions();
   }
   else
   {
      if(!ValidateOptions(opts, ValidExportOptions(), expl))
      {
         return cli.Report(-1, expl);
      }
   }

   if((opts.find(NamespaceView) != string::npos) ||
      (opts.find(CanonicalFileView) != string::npos) ||
      (opts.find(OriginalFileView) != string::npos) ||
      (opts.find(ClassHierarchyView) != string::npos))
   {
      Debug::Progress(string("Exporting parsed code...") + CRLF);
      auto stream = cli.FileStream();
      if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      Singleton< Library >::Instance()->Export(*stream, opts);
      auto filename = title + ".lib.txt";
      cli.SendToFile(filename, true);
   }

   if(opts.find(FileSymbolUsage) != string::npos)
   {
      Debug::Progress(string("Exporting file symbol usage...") + CRLF);
      auto stream = cli.FileStream();
      if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      auto filename = title + ".trim.txt";
      Singleton< Library >::Instance()->Trim(*stream);
      cli.SendToFile(filename, true);
   }

   if(opts.find(GlobalCrossReference) != string::npos)
   {
      Debug::Progress(string("Exporting cross-reference...") + CRLF);
      auto stream = cli.FileStream();
      if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      auto filename = title + ".xref.txt";
      Singleton< CxxSymbols >::Instance()->DisplayXref(*stream);
      cli.SendToFile(filename, true);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The FILEID command.
//
class FileIdMandParm : public CliIntParm
{
public: FileIdMandParm();
};

class FileIdCommand : public CliCommand
{
public:
   FileIdCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FileIdMandExpl = "file's identifier";

FileIdMandParm::FileIdMandParm() :
   CliIntParm(FileIdMandExpl, 1, 4095) { }

fixed_string FileIdStr = "fileid";
fixed_string FileIdExpl = "Displays information about a code file.";

FileIdCommand::FileIdCommand() : CliCommand(FileIdStr, FileIdExpl)
{
   BindParm(*new FileIdMandParm);
}

fn_name FileIdCommand_ProcessCommand = "FileIdCommand.ProcessCommand";

word FileIdCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(FileIdCommand_ProcessCommand);

   word fid;

   if(!GetIntParm(fid, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto file = Singleton< Library >::Instance()->Files().At(fid);
   if(file == nullptr) return cli.Report(-2, NoFileExpl);
   file->Display(*cli.obuf, spaces(2), VerboseOpt);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The FILEINFO command.
//
class CodeFileParm : public CliTextParm
{
public: CodeFileParm();
};

class FileInfoCommand : public CliCommand
{
public:
   FileInfoCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CodeFileExpl = "filename (including extension)";

CodeFileParm::CodeFileParm() : CliTextParm(CodeFileExpl, false, 0) { }

fixed_string FileInfoStr = "fileinfo";
fixed_string FileInfoExpl = "Displays information about a code file.";

FileInfoCommand::FileInfoCommand() : CliCommand(FileInfoStr, FileInfoExpl)
{
   BindParm(*new CodeFileParm);
}

fn_name FileInfoCommand_ProcessCommand = "FileInfoCommand.ProcessCommand";

word FileInfoCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(FileInfoCommand_ProcessCommand);

   string name;

   if(!GetString(name, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto file = Singleton< Library >::Instance()->FindFile(name);
   if(file == nullptr) return cli.Report(-2, NoFileExpl);
   file->Display(*cli.obuf, spaces(2), VerboseOpt);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The FIX command.
//
class WarningParm : public CliIntParm
{
public: WarningParm();
};

fixed_string WarningExpl = "warning number from Wnnn (0 = all warnings)";

WarningParm::WarningParm() : CliIntParm(WarningExpl, 0, Warning_N - 1) { }

class PromptParm : public CliBoolParm
{
public: PromptParm();
};

fixed_string PromptExpl = "prompt before fixing?";

PromptParm::PromptParm() : CliBoolParm(PromptExpl) { }

class FixCommand : public LibraryCommand
{
public:
   FixCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FixStr = "fix";
fixed_string FixExpl = "Interactively fixes warnings detected by >check.";

FixCommand::FixCommand() : LibraryCommand(FixStr, FixExpl)
{
   BindParm(*new WarningParm);
   BindParm(*new PromptParm);
   BindParm(*new FileSetExprParm);
}

fn_name FixCommand_ProcessCommand = "FixCommand.ProcessCommand";

word FixCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(FixCommand_ProcessCommand);

   word warning;
   FixOptions options;

   if(!GetIntParm(warning, cli)) return -1;
   if(!GetBoolParm(options.prompt, cli)) return -1;
   options.warning = static_cast< Warning >(warning);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string expl;
   auto rc = set->Fix(cli, options, expl);
   set->Release();
   if(rc == 0) return 0;
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The FORMAT command.
//
class FormatCommand : public LibraryCommand
{
public:
   FormatCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FormatStr = "format";
fixed_string FormatExpl = "Reformats code files.";

FormatCommand::FormatCommand() : LibraryCommand(FormatStr, FormatExpl)
{
   BindParm(*new CodeSetExprParm);
}

fn_name FormatCommand_ProcessCommand = "FormatCommand.ProcessCommand";

word FormatCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(FormatCommand_ProcessCommand);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string expl;

   auto rc = set->Format(expl);
   set->Release();
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The IMPORT command.
//
class DirMandName : public CliTextParm
{
public: DirMandName();
};

class PathMandParm : public CliTextParm
{
public: PathMandParm();
};

class ImportCommand : public CliCommand
{
public:
   ImportCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DirMandNameExpl = "directory name";

DirMandName::DirMandName() : CliTextParm(DirMandNameExpl, false, 0) { }

fixed_string PathMandExpl = "path within SourcePath configuration parameter";

PathMandParm::PathMandParm() : CliTextParm(PathMandExpl, false, 0) { }

fixed_string ImportStr = "import";
fixed_string ImportExpl = "Adds a directory to the code base.";

ImportCommand::ImportCommand() : CliCommand(ImportStr, ImportExpl)
{
   BindParm(*new DirMandName);
   BindParm(*new PathMandParm);
}

fn_name ImportCommand_ProcessCommand = "ImportCommand.ProcessCommand";

word ImportCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ImportCommand_ProcessCommand);

   string name, subdir, expl;

   if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
      Symbol::InvalidInitialChars())) return -1;
   if(!GetString(subdir, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto lib = Singleton< Library >::Instance();
   string path(lib->SourcePath());
   if(!subdir.empty()) path += PATH_SEPARATOR + subdir;
   auto rc = lib->Import(name, path, expl);
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The LIST command.
//
class ListCommand : public LibraryCommand
{
public:
   ListCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ListStr = "list";
fixed_string ListExpl = "Displays the items in a set, one per line.";

ListCommand::ListCommand() : LibraryCommand(ListStr, ListExpl)
{
   BindParm(*new CodeSetExprParm);
}

fn_name ListCommand_ProcessCommand = "ListCommand.ProcessCommand";

word ListCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ListCommand_ProcessCommand);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string expl;

   auto rc = set->List(*cli.obuf, expl);
   set->Release();
   if(rc != 0) return cli.Report(rc, expl);
   return rc;
}

//------------------------------------------------------------------------------
//
//  The PARSE command.
//
class ParseOptionsParm : public CliTextParm
{
public: ParseOptionsParm();
};

fixed_string ParseOptionsExpl =
   "options (enter \">help parse full\" for details)";

ParseOptionsParm::ParseOptionsParm() :
   CliTextParm(ParseOptionsExpl, false, 0) { }

class DefineFileParm : public CliTextParm
{
public: DefineFileParm();
};

fixed_string DefineFileExpl =
   "file for #define symbols (.txt in input directory)";

DefineFileParm::DefineFileParm() : CliTextParm(DefineFileExpl, false, 0) { }

class ParseCommand : public LibraryCommand
{
public:
   ParseCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ParseStr = "parse";
fixed_string ParseExpl = "Parses code files.";

ParseCommand::ParseCommand() : LibraryCommand(ParseStr, ParseExpl)
{
   BindParm(*new ParseOptionsParm);
   BindParm(*new DefineFileParm);
   BindParm(*new FileSetExprParm);
}

const string& ValidParseOptions()
{
   static string ValidOpts;

   if(ValidOpts.empty())
   {
      ValidOpts.push_back(TemplateLogs);
      ValidOpts.push_back(TraceParse);
      ValidOpts.push_back(SaveParseTrace);
      ValidOpts.push_back(TraceCompilation);
      ValidOpts.push_back(TraceFunctions);
      ValidOpts.push_back(TraceImmediate);
   }

   return ValidOpts;
}

fn_name ParseCommand_ProcessCommand = "ParseCommand.ProcessCommand";

word ParseCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ParseCommand_ProcessCommand);

   string opts;
   string name;
   string expl;

   if(!GetString(opts, cli)) return -1;
   if(!GetString(name, cli)) return -1;

   if(!opts.empty() && (opts != "-"))
   {
      if(!ValidateOptions(opts, ValidParseOptions(), expl))
      {
         return cli.Report(-1, expl);
      }
   }

   auto path = Element::InputPath() + PATH_SEPARATOR + name + ".txt";
   auto file = SysFile::CreateIstream(path.c_str());
   if(file == nullptr) return cli.Report(-2, NoFileExpl);
   Singleton< CxxRoot >::Instance()->DefineSymbols(*file.get());

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   auto rc = set->Parse(expl, opts);
   set->Release();
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The PURGE command.
//
class PurgeCommand : public CliCommand
{
public:
   PurgeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string PurgeStr = "purge";
fixed_string PurgeExpl = "Deletes a variable.";

PurgeCommand::PurgeCommand() : CliCommand(PurgeStr, PurgeExpl)
{
   BindParm(*new VarMandName);
}

fn_name PurgeCommand_ProcessCommand = "PurgeCommand.ProcessCommand";

word PurgeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(PurgeCommand_ProcessCommand);

   string name, expr, expl;

   if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
      Symbol::InvalidInitialChars())) return -1;
   if(!cli.EndOfInput()) return -1;

   auto rc = Singleton< Library >::Instance()->Purge(name, expl);
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The SCAN command.
//
   class StringPatternParm : public CliTextParm
   {
   public: StringPatternParm();
   };

   class ScanCommand : public CliCommand
   {
   public:
      ScanCommand();
   private:
      word ProcessCommand(CliThread& cli) const override;
   };

fixed_string StringPatternExpl = "string to look for (quoted; '$' = wildcard)";

StringPatternParm::StringPatternParm() :
   CliTextParm(StringPatternExpl, false, 0) { }

fixed_string ScanStr = "scan";
fixed_string ScanExpl = "Scans files for lines that contain a string.";

ScanCommand::ScanCommand() : CliCommand(ScanStr, ScanExpl)
{
   BindParm(*new FileSetExprParm);
   BindParm(*new StringPatternParm);
}

fn_name ScanCommand_ProcessCommand = "ScanCommand.ProcessCommand";

word ScanCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ScanCommand_ProcessCommand);

   string line, expr, pattern, expl;

   //  Read the entire line and then extract the quoted string at the end.
   //
   auto pos = cli.Prompt().size() + cli.ibuf->Pos();
   cli.ibuf->Read(line);
   if(!cli.EndOfInput()) return -1;

   auto quote1 = line.find(QUOTE);
   auto quote2 = line.rfind(QUOTE);
   if(quote1 == string::npos) return cli.Report(-2, "Quoted string missing.");
   if(quote2 == quote1) return cli.Report(-2, "Closing \" missing.");
   if(quote2 - quote1 == 1) return cli.Report(-2, "Pattern string is empty.");

   expr = line.substr(0, quote1);
   pattern = line.substr(quote1 + 1, quote2 - (quote1 + 1));

   auto set = Singleton< Library >::Instance()->Evaluate(expr, pos);
   if(set == nullptr) return cli.Report(-7, AllocationError);
   auto rc = set->Scan(*cli.obuf, pattern, expl);
   set->Release();
   if(rc != 0) return cli.Report(rc, expl);
   return rc;
}

//------------------------------------------------------------------------------
//
//  The SHOW command.
//
class DirsText : public CliText
{
public: DirsText();
};

class FailedText : public CliText
{
public: FailedText();
};

class ItemsText : public CliText
{
public: ItemsText();
};

class StatsText : public CliText
{
public: StatsText();
};

class ShowWhatParm : public CliTextParm
{
public: ShowWhatParm();
};

class ShowCommand : public LibraryCommand
{
public:
   static const id_t DirsIndex = 1;
   static const id_t FailedIndex = 2;
   static const id_t ItemsIndex = 3;
   static const id_t StatsIndex = 4;

   ShowCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DirsTextStr = "dirs";
fixed_string DirsTextExpl = "code directories";

DirsText::DirsText() : CliText(DirsTextExpl, DirsTextStr) { }

fixed_string FailedTextStr = "failed";
fixed_string FailedTextExpl = "code files that failed to parse";

FailedText::FailedText() : CliText(FailedTextExpl, FailedTextStr) { }

fixed_string ItemsTextStr = "items";
fixed_string ItemsTextExpl = "memory usage by item type";

ItemsText::ItemsText() : CliText(ItemsTextExpl, ItemsTextStr) { }

fixed_string StatsTextStr = "stats";
fixed_string StatsTextExpl = "parser statistics";

StatsText::StatsText() : CliText(StatsTextExpl, StatsTextStr) { }

fixed_string ShowWhatExpl = "what to show...";

ShowWhatParm::ShowWhatParm() : CliTextParm(ShowWhatExpl)
{
   BindText(*new DirsText, ShowCommand::DirsIndex);
   BindText(*new FailedText, ShowCommand::FailedIndex);
   BindText(*new ItemsText, ShowCommand::ItemsIndex);
   BindText(*new StatsText, ShowCommand::StatsIndex);
}

fixed_string ShowStr = "show";
fixed_string ShowExpl = "Displays library information.";

ShowCommand::ShowCommand() : LibraryCommand(ShowStr, ShowExpl)
{
   BindParm(*new ShowWhatParm);
}

fn_name ShowCommand_ProcessCommand = "ShowCommand.ProcessCommand";

word ShowCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ShowCommand_ProcessCommand);

   id_t index;

   if(!GetTextIndex(index, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   switch(index)
   {
   case DirsIndex:
      {
         //  Display the number of .h and .cpp files found in each directory.
         //
         *cli.obuf << "  Directory    .h  .cpp  Path" << CRLF;

         size_t hdrs = 0;
         size_t cpps = 0;
         auto& dirs = Singleton< Library >::Instance()->Directories();

         for(auto d = dirs.First(); d != nullptr; dirs.Next(d))
         {
            auto h = d->HeaderCount();
            *cli.obuf << setw(11) << d->Name();
            *cli.obuf << setw(6) << h;
            auto c = d->CppCount();
            *cli.obuf << setw(6) << c;
            *cli.obuf << spaces(2) << d->Path() << CRLF;
            hdrs += h;
            cpps += c;
         }

         *cli.obuf << setw(11) << "TOTAL" << setw(6) << hdrs
            << setw(6) << cpps << CRLF;
      }
      break;

   case FailedIndex:
      {
         auto found = false;
         auto& files = Singleton< Library >::Instance()->Files();

         for(auto f = files.First(); f != nullptr; files.Next(f))
         {
            if(f->ParseStatus() == CodeFile::Failed)
            {
               *cli.obuf << spaces(2) << f->Name() << CRLF;
               found = true;
            }
         }

         if(!found) return cli.Report(0, "No files failed to parse.");
      }
      break;

   case ItemsIndex:
      CxxStats::Display(*cli.obuf);
      break;

   case StatsIndex:
      Parser::DisplayStats(*cli.obuf);
      break;

   default:
      Debug::SwLog(ShowCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The SHRINK command.
//
class ShrinkCommand : public CliCommand
{
public:
   ShrinkCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

fixed_string ShrinkStr = "shrink";
fixed_string ShrinkExpl = "Shrinks the library's element containers.";

ShrinkCommand::ShrinkCommand() : CliCommand(ShrinkStr, ShrinkExpl) { }

fn_name ShrinkCommand_ProcessCommand = "ShrinkCommand.ProcessCommand";

word ShrinkCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ShrinkCommand_ProcessCommand);

   if(!cli.EndOfInput()) return -1;

   CxxStats::Shrink();
   return 0;
}

//------------------------------------------------------------------------------
//
//  The SORT command.
//
class SortCommand : public LibraryCommand
{
public:
   SortCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SortStr = "sort";
fixed_string SortExpl = "Sorts files by build dependency order.";

SortCommand::SortCommand() : LibraryCommand(SortStr, SortExpl)
{
   BindParm(*new FileSetExprParm);
}

fn_name SortCommand_ProcessCommand = "SortCommand.ProcessCommand";

word SortCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(SortCommand_ProcessCommand);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string expl;

   auto rc = set->Sort(*cli.obuf, expl);
   set->Release();
   if(rc != 0) return cli.Report(rc, expl);
   return rc;
}

//------------------------------------------------------------------------------
//
//  The TRACE command.
//
class TraceCommand : public CliCommand
{
public:
   TraceCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TraceStr = "trace";
fixed_string TraceExpl = "Manage tracepoints for >parse command.";

class FileNameParm : public CliTextParm
{
public: FileNameParm();
};

fixed_string FileNameExpl = "name of source code file";

FileNameParm::FileNameParm() : CliTextParm(FileNameExpl, false, 0) { }

class LineNumberParm : public CliIntParm
{
public: LineNumberParm();
};

fixed_string LineNumberExpl = "line number (must contain source code)";

LineNumberParm::LineNumberParm() : CliIntParm(LineNumberExpl, 0, 999999) { }

class BreakText : public CliText
{
public: BreakText();
};

class StartText : public CliText
{
public: StartText();
};

class StopText : public CliText
{
public: StopText();
};

class ModeParm : public CliTextParm
{
public:
   ModeParm();

   //  Values for the parameter.
   //
   static const id_t Break = Tracepoint::Break;
   static const id_t Start = Tracepoint::Start;
   static const id_t Stop = Tracepoint::Stop;
};

fixed_string BreakTextStr = "break";
fixed_string BreakTextExpl = "breakpoint (at Debug::noop in Context::SetPos)";

BreakText::BreakText() : CliText(BreakTextExpl, BreakTextStr) { }

fixed_string StartTextStr = "start";
fixed_string StartTextExpl = "start tracing (must preconfigure settings)";

StartText::StartText() : CliText(StartTextExpl, StartTextStr) { }

fixed_string StopTextStr = "stop";
fixed_string StopTextExpl = "stop tracing";

StopText::StopText() : CliText(StopTextExpl, StopTextStr) { }

fixed_string ModeExpl = "action at tracepoint...";

ModeParm::ModeParm() : CliTextParm(ModeExpl)
{
   BindText(*new BreakText, ModeParm::Break);
   BindText(*new StartText, ModeParm::Start);
   BindText(*new StopText, ModeParm::Stop);
}

class InsertText : public CliText
{
public: InsertText();
};

class RemoveText : public CliText
{
public: RemoveText();
};

class ClearText : public CliText
{
public: ClearText();
};

class ListText : public CliText
{
public: ListText();
};

class ActionParm : public CliTextParm
{
public:
   ActionParm();

   //  Values for the parameter.
   //
   static const id_t Insert = 1;
   static const id_t Remove = 2;
   static const id_t Clear = 3;
   static const id_t List = 4;
};

fixed_string InsertTextStr = "insert";
fixed_string InsertTextExpl = "add tracepoint";

InsertText::InsertText() : CliText(InsertTextExpl, InsertTextStr)
{
   BindParm(*new ModeParm);
   BindParm(*new FileNameParm);
   BindParm(*new LineNumberParm);
}

fixed_string RemoveTextStr = "remove";
fixed_string RemoveTextExpl = "delete tracepoint";

RemoveText::RemoveText() : CliText(RemoveTextExpl, RemoveTextStr)
{
   BindParm(*new ModeParm);
   BindParm(*new FileNameParm);
   BindParm(*new LineNumberParm);
}

fixed_string ClearTextStr = "clear";
fixed_string ClearTextExpl = "delete all tracepoints";

ClearText::ClearText() : CliText(ClearTextExpl, ClearTextStr) { }

fixed_string ListTextStr = "list";
fixed_string ListTextExpl = "list tracepoints";

ListText::ListText() : CliText(ListTextExpl, ListTextStr) { }

fixed_string ActionExpl = "subcommand...";

ActionParm::ActionParm() : CliTextParm(ActionExpl)
{
   BindText(*new InsertText, ActionParm::Insert);
   BindText(*new RemoveText, ActionParm::Remove);
   BindText(*new ClearText, ActionParm::Clear);
   BindText(*new ListText, ActionParm::List);
}

TraceCommand::TraceCommand() : CliCommand(TraceStr, TraceExpl)
{
   BindParm(*new ActionParm);
}

fn_name TraceCommand_ProcessCommand = "TraceCommand.ProcessCommand";

word TraceCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(TraceCommand_ProcessCommand);

   id_t action;
   id_t mode;
   string filename;
   word line;

   if(!GetTextIndex(action, cli)) return -1;

   switch(action)
   {
   case ActionParm::Insert:
   case ActionParm::Remove:
      break;

   case ActionParm::Clear:
      if(!cli.EndOfInput()) return -1;
      Context::ClearTracepoints();
      return cli.Report(0, SuccessExpl);

   case ActionParm::List:
      if(!cli.EndOfInput()) return -1;
      Context::DisplayTracepoints(*cli.obuf, spaces(2));
      return 0;

   default:
      return cli.Report(action, SystemErrorExpl);
   }

   if(!GetTextIndex(mode, cli)) return -1;
   if(!GetString(filename, cli)) return -1;
   if(!GetIntParm(line, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto lib = Singleton< Library >::Instance();
   auto file = lib->FindFile(filename);

   if(file == nullptr)
   {
      return cli.Report(-2, "Source code file not found.");
   }

   if(action == ActionParm::Remove)
   {
      Context::EraseTracepoint(file, line - 1, Tracepoint::Action(mode));
      return cli.Report(0, SuccessExpl);
   }

   auto source = file->GetLexer().GetNthLine(line - 1);
   auto type = file->GetLineType(line - 1);

   if(!LineTypeAttr::Attrs[type].isExecutable)
   {
      *cli.obuf << source << CRLF;
      return cli.Report(-3, "That line does not contain executable code.");
   }

   *cli.obuf << spaces(2) << source << CRLF;
   Context::InsertTracepoint(file, line - 1, Tracepoint::Action(mode));
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The TYPE command.
//
class TypeCommand : public LibraryCommand
{
public:
   TypeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TypeStr = "type";
fixed_string TypeExpl = "Displays the items in a set, separated by commas.";

TypeCommand::TypeCommand() : LibraryCommand(TypeStr, TypeExpl)
{
   BindParm(*new SetExprParm);
}

fn_name TypeCommand_ProcessCommand = "TypeCommand.ProcessCommand";

word TypeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(TypeCommand_ProcessCommand);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string result;

   auto rc = set->Show(result);
   set->Release();
   return cli.Report(rc, result);
}

//------------------------------------------------------------------------------
//
//  The EXP command (for experimental testing).
//
class ExpCommand : public CliCommand
{
public:
   ExpCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------

fixed_string ExpStr = "exp";
fixed_string ExpExpl = "Performs an experimental test.";

ExpCommand::ExpCommand() : CliCommand(ExpStr, ExpExpl) { }

fn_name ExpCommand_ProcessCommand = "ExpCommand.ProcessCommand";

word ExpCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ExpCommand_ProcessCommand);

   if(!cli.EndOfInput()) return -1;
   *cli.obuf << "This command currently does nothing." << CRLF;
   return 0;
}

//------------------------------------------------------------------------------
//
//  The source code increment.
//
fixed_string CtStr = "ct";
fixed_string CtExpl = "CodeTools Increment";

fn_name CtIncrement_ctor = "CtIncrement.ctor";

CtIncrement::CtIncrement() : CliIncrement(CtStr, CtExpl)
{
   Debug::ft(CtIncrement_ctor);

   BindCommand(*new ImportCommand);
   BindCommand(*new ShowCommand);
   BindCommand(*new TypeCommand);
   BindCommand(*new ListCommand);
   BindCommand(*new CountCommand);
   BindCommand(*new CountlinesCommand);
   BindCommand(*new ScanCommand);
   BindCommand(*new AssignCommand);
   BindCommand(*new PurgeCommand);
   BindCommand(*new SortCommand);
   BindCommand(*new FileInfoCommand);
   BindCommand(*new FileIdCommand);
   BindCommand(*new TraceCommand);
   BindCommand(*new ParseCommand);
   BindCommand(*new CheckCommand);
   BindCommand(*new ExplainCommand);
   BindCommand(*new FixCommand);
   BindCommand(*new FormatCommand);
   BindCommand(*new ExportCommand);
   BindCommand(*new CoverageCommand);
   BindCommand(*new ShrinkCommand);
   BindCommand(*new ExpCommand);

   Parser::ResetStats();
}

//------------------------------------------------------------------------------

fn_name CtIncrement_dtor = "CtIncrement.dtor";

CtIncrement::~CtIncrement()
{
   Debug::ftnt(CtIncrement_dtor);
}
}
