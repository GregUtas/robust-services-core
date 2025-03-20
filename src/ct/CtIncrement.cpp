//==============================================================================
//
//  CtIncrement.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "CtIncrement.h"
#include "CliCommand.h"
#include "CliText.h"
#include "CliTextParm.h"
#include <cstddef>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include "CliBoolParm.h"
#include "CliBuffer.h"
#include "CliIntParm.h"
#include "CliThread.h"
#include "CodeCoverage.h"
#include "CodeDir.h"
#include "CodeDirSet.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CodeTypes.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Library.h"
#include "LibraryTypes.h"
#include "NbCliParms.h"
#include "Parser.h"
#include "Singleton.h"
#include "Symbol.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Parameters used by more than one command.
//
fixed_string CodeFileExpl = "filename (including extension)";

fixed_string CodeSetExprExpl = "a set of code files or directories";

fixed_string FileSetExprExpl = "a set of code files";

fixed_string SetExprExpl = "a set of code files or directories";

fixed_string VarMandNameExpl = "variable name";

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
   static LibrarySet* Evaluate(CliThread& cli);
protected:
   //  The arguments are from the base class.  Protected because this
   //  class is virtual.
   //
   LibraryCommand(c_string comm, c_string help);
};

LibraryCommand::LibraryCommand(c_string comm, c_string help) :
   CliCommand(comm, help) { }

LibrarySet* LibraryCommand::Evaluate(CliThread& cli)
{
   Debug::ft("LibraryCommand.Evaluate");

   string expr;

   auto pos = cli.Prompt().size() + cli.ibuf->Pos();
   cli.ibuf->Read(expr);
   if(!cli.EndOfInput()) return nullptr;

   auto result = Singleton<Library>::Instance()->Evaluate(cli, expr, pos);
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
   BindParm(*new CliTextParm(VarMandNameExpl, false, 0));
   BindParm(*new CliTextParm(CodeSetExprExpl, false, 0));
}

word AssignCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("AssignCommand.ProcessCommand");

   string name, expr, expl;

   if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
      Symbol::InvalidInitialChars())) return -1;
   auto pos = cli.Prompt().size() + cli.ibuf->Pos();
   cli.ibuf->Read(expr);
   if(!cli.EndOfInput()) return -1;

   auto lib = Singleton<Library>::Instance();
   auto rc = lib->Assign(cli, name, expr, pos, expl);
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
   BindParm(*new CliTextParm(FileSetExprExpl, false, 0));
}

word CheckCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CheckCommand.ProcessCommand");

   string title;

   if(!GetFileName(title, cli)) return -1;

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return -1;

   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   string expl;
   auto rc = set->Check(cli, stream, expl);

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
fixed_string CoverageLoadTextStr = "load";
fixed_string CoverageLoadTextExpl =
   "reads the database from InputPath/coverage.db.txt";

fixed_string CoverageQueryTextStr = "query";
fixed_string CoverageQueryTextExpl =
   "displays information about the loaded database";

class CoverageUnderText : public CliText
{
public: CoverageUnderText();
};

fixed_string MinTestsParmExpl = "value of N";

fixed_string CoverageUnderTextStr = "under";
fixed_string CoverageUnderTextExpl =
   "displays functions invoked by fewer than N tests";

CoverageUnderText::CoverageUnderText() :
   CliText(CoverageUnderTextExpl, CoverageUnderTextStr)
{
   BindParm(*new CliIntParm(MinTestsParmExpl, 1, 10));
}

class CoverageEraseText : public CliText
{
public: CoverageEraseText();
};

fixed_string FuncNameParmExpl = "name of function to remove";

fixed_string CoverageEraseTextStr = "erase";
fixed_string CoverageEraseTextExpl = "removes a function from the database";

CoverageEraseText::CoverageEraseText() :
   CliText(CoverageEraseTextExpl, CoverageEraseTextStr)
{
   BindParm(*new CliTextParm(FuncNameParmExpl, false, 0));
}

fixed_string CoverageUpdateStr = "update";
fixed_string CoverageUpdateExpl =
   "updates database with modified functions and rerun tests";

class CoverageAction : public CliTextParm
{
public: CoverageAction();
};

constexpr id_t CoverageLoadIndex = 1;
constexpr id_t CoverageQueryIndex = 2;
constexpr id_t CoverageUnderIndex = 3;
constexpr id_t CoverageEraseIndex = 4;
constexpr id_t CoverageUpdateIndex = 5;

fixed_string CoverageActionExpl = "subcommand...";

CoverageAction::CoverageAction() : CliTextParm(CoverageActionExpl)
{
   BindText(*new CliText
      (CoverageLoadTextExpl, CoverageLoadTextStr), CoverageLoadIndex);
   BindText(*new CliText
      (CoverageQueryTextExpl, CoverageQueryTextStr), CoverageQueryIndex);
   BindText(*new CoverageUnderText, CoverageUnderIndex);
   BindText(*new CoverageEraseText, CoverageEraseIndex);
   BindText(*new CliText
      (CoverageUpdateExpl, CoverageUpdateStr), CoverageUpdateIndex);
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

word CoverageCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CoverageCommand.ProcessCommand");

   auto database = Singleton<CodeCoverage>::Instance();
   id_t index;
   word min;
   string name, expl;
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
   BindParm(*new CliTextParm(SetExprExpl, false, 0));
}

word CountCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CountCommand.ProcessCommand[>ct]");

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string result;
   auto rc = set->Count(result);
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
   BindParm(*new CliTextParm(FileSetExprExpl, false, 0));
}

word CountlinesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CountlinesCommand.ProcessCommand");

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string result;
   auto rc = set->Countlines(result);
   return cli.Report(rc, result);
}

//------------------------------------------------------------------------------
//
//  The EXPLAIN command.
//
fixed_string WarningIdExpl = "warning number";

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
   BindParm(*new CliIntParm(WarningIdExpl, 1, Warning_N - 1));
}

word ExplainCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ExplainCommand.ProcessCommand");

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
fixed_string ViewsExpl = "options (enter \">help export full\" for details)";

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
   BindParm(*new CliTextParm(ViewsExpl, true, 0));
}

static const string& DefaultExportOptions()
{
   static string DefaultOpts;

   if(DefaultOpts.empty())
   {
      DefaultOpts.push_back(NamespaceView);
      DefaultOpts.push_back(CanonicalFileView);
      DefaultOpts.push_back(ClassHierarchyView);
      DefaultOpts.push_back(ItemStatistics);
      DefaultOpts.push_back(FileSymbolUsage);
      DefaultOpts.push_back(CrossReferenceBrief);
   }

   return DefaultOpts;
}

static const string& ValidExportOptions()
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
      ValidOpts.push_back(CrossReferenceVerbose);
      ValidOpts.push_back(CrossReferenceBrief);
      ValidOpts.push_back(CodeComments);
   }

   return ValidOpts;
}

word ExportCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ExportCommand.ProcessCommand");

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
   else if(!ValidateOptions(opts, ValidExportOptions(), expl))
   {
      return cli.Report(-1, expl);
   }

   if((opts.find(NamespaceView) != string::npos) ||
      (opts.find(CanonicalFileView) != string::npos) ||
      (opts.find(OriginalFileView) != string::npos) ||
      (opts.find(ClassHierarchyView) != string::npos))
   {
      Debug::Progress(string("Exporting parsed code...") + CRLF);
      auto stream = cli.FileStream();
      if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      Singleton<Library>::Instance()->Export(*stream, opts);
      auto filename = title + ".lib.txt";
      cli.SendToFile(filename, true);
   }

   if(opts.find(FileSymbolUsage) != string::npos)
   {
      Debug::Progress(string("Exporting file symbol usage...") + CRLF);
      auto stream = cli.FileStream();
      if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      auto filename = title + ".trim.txt";
      Singleton<Library>::Instance()->Trim(*stream);
      cli.SendToFile(filename, true);
   }

   if((opts.find(CrossReferenceBrief) != string::npos) ||
      (opts.find(CrossReferenceVerbose) != string::npos))
   {
      Debug::Progress(string("Exporting cross-reference...") + CRLF);
      auto stream = cli.FileStream();
      if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);
      auto filename = title + ".xref.txt";
      Singleton<CxxSymbols>::Instance()->DisplayXref(*stream, opts);
      cli.SendToFile(filename, true);
   }

   if(opts.find(CodeComments) != string::npos)
   {
      Debug::Progress(string("Exporting comments...") + CRLF);
      auto stream = cli.FileStream();
      Singleton<Library>::Instance()->DisplayComments(*stream);
      auto filename = title + ".comments.txt";
      cli.SendToFile(filename, true);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The FILEINFO command.
//
class FileInfoCommand : public CliCommand
{
public:
   FileInfoCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FileInfoStr = "fileinfo";
fixed_string FileInfoExpl = "Displays information about a code file.";

FileInfoCommand::FileInfoCommand() : CliCommand(FileInfoStr, FileInfoExpl)
{
   BindParm(*new CliTextParm(CodeFileExpl, false, 0));
}

word FileInfoCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("FileInfoCommand.ProcessCommand");

   string name;

   if(!GetString(name, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto file = Singleton<Library>::Instance()->FindFile(name);
   if(file == nullptr) return cli.Report(-2, NoFileExpl);
   file->Display(*cli.obuf, spaces(2), VerboseOpt);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The FIX command.
//
fixed_string WarningExpl = "warning number from Wnnn (0 = all warnings)";

fixed_string PromptExpl = "prompt before fixing?";

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
   BindParm(*new CliIntParm(WarningExpl, 0, Warning_N - 1));
   BindParm(*new CliBoolParm(PromptExpl));
   BindParm(*new CliTextParm(FileSetExprExpl, false, 0));
}

word FixCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("FixCommand.ProcessCommand");

   word warning;
   FixOptions options;

   if(!GetIntParm(warning, cli)) return -1;
   if(!GetBoolParm(options.prompt, cli)) return -1;
   options.warning = static_cast<Warning>(warning);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string expl;
   auto rc = set->Fix(cli, options, expl);
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
   BindParm(*new CliTextParm(CodeSetExprExpl, false, 0));
}

word FormatCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("FormatCommand.ProcessCommand");

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string expl;
   auto rc = set->Format(expl);
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The IMPORT command.
//
class ImportCommand : public CliCommand
{
public:
   ImportCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DirMandNameExpl = "directory name";

fixed_string PathMandExpl = "path within SourcePath configuration parameter";

fixed_string ImportStr = "import";
fixed_string ImportExpl = "Adds a directory to the code base.";

ImportCommand::ImportCommand() : CliCommand(ImportStr, ImportExpl)
{
   BindParm(*new CliTextParm(DirMandNameExpl, false, 0));
   BindParm(*new CliTextParm(PathMandExpl, false, 0));
}

word ImportCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ImportCommand.ProcessCommand");

   string name, subdir, expl;

   if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
      Symbol::InvalidInitialChars())) return -1;
   if(!GetString(subdir, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto lib = Singleton<Library>::Instance();
   string path(lib->SourcePath());
   if(!subdir.empty()) path += PATH_SEPARATOR + subdir;
   auto rc = lib->Import(name, path, expl);
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The ITEMS command.
//
class ItemsCommand : public LibraryCommand
{
public:
   ItemsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ItemsStr = "items";
fixed_string ItemsExpl = "Displays the C++ items in a file, by position.";

ItemsCommand::ItemsCommand() : LibraryCommand(ItemsStr, ItemsExpl)
{
   BindParm(*new CliTextParm(CodeFileExpl, false, 0));
}

word ItemsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ItemsCommand.ProcessCommand");

   string name;

   if(!GetFileName(name, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto file = Singleton<Library>::Instance()->FindFile(name);
   if(file == nullptr) return cli.Report(-2, NoFileExpl);
   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   const auto& lexer = file->GetLexer();

   for(auto p = lexer.NextPos(0); p != string::npos; p = lexer.NextPos(p + 1))
   {
      auto item = file->PosToItem(p);

      if((item != nullptr) && !item->IsInternal())
      {
         auto str = lexer.Substr(p, 16);
         *stream << p << ": " << std::setw(16) << str;
         *stream << spaces(3) << strObj(item) << CRLF;
      }
   }

   auto filename = name + ".items.txt";
   cli.SendToFile(filename, true);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The LINETYPES command.
//
class LineTypesCommand : public LibraryCommand
{
public:
   LineTypesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string LineTypesStr = "linetypes";
fixed_string LineTypesExpl =
   "Displays the count of line types in the specified files.";

LineTypesCommand::LineTypesCommand() :
   LibraryCommand(LineTypesStr, LineTypesExpl)
{
   BindParm(*new OstreamMandParm);
   BindParm(*new CliTextParm(FileSetExprExpl, false, 0));
}

word LineTypesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("LineTypesCommand.ProcessCommand");

   string title;

   if(!GetFileName(title, cli)) return -1;

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return -1;

   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   string expl;
   auto rc = set->LineTypes(cli, stream, expl);

   if(rc == 0)
   {
      title += ".lines.txt";
      cli.SendToFile(title, true);
   }

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
   BindParm(*new CliTextParm(CodeSetExprExpl, false, 0));
}

word ListCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ListCommand.ProcessCommand");

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   auto rc = set->List(*cli.obuf);
   return rc;
}

//------------------------------------------------------------------------------
//
//  The PARSE command.
//
fixed_string ParseOptionsExpl =
   "options (enter \">help parse full\" for details)";

fixed_string DefineFileExpl =
   "file for #define symbols (.txt in input directory)";

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
   BindParm(*new CliTextParm(ParseOptionsExpl, false, 0));
   BindParm(*new CliTextParm(DefineFileExpl, false, 0));
   BindParm(*new CliTextParm(FileSetExprExpl, false, 0));
}

static const string& ValidParseOptions()
{
   static string ValidOpts;

   if(ValidOpts.empty())
   {
      ValidOpts.push_back(TemplateLogs);
      ValidOpts.push_back(TraceParsing);
      ValidOpts.push_back(TraceCompilation);
      ValidOpts.push_back(TraceFunctions);
      ValidOpts.push_back(TraceInstantiation);
   }

   return ValidOpts;
}

word ParseCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ParseCommand.ProcessCommand");

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

   auto rc = Singleton<CxxRoot>::Instance()->DefineSymbols(name, expl);
   if(rc != 0) return cli.Report(rc, expl);

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   rc = set->Parse(expl, opts);
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
   BindParm(*new CliTextParm(VarMandNameExpl, false, 0));
}

word PurgeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("PurgeCommand.ProcessCommand");

   string name, expl;

   if(!GetIdentifier(name, cli, Symbol::ValidNameChars(),
      Symbol::InvalidInitialChars())) return -1;
   if(!cli.EndOfInput()) return -1;

   auto rc = Singleton<Library>::Instance()->Purge(name, expl);
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The RENAME command.
//
fixed_string OldNameExpl =
   "name of C++ item (enter \">help rename full\" for details)";

fixed_string NewNameExpl = "new name for C++ item";

class RenameCommand : public CliCommand
{
public:
   RenameCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string RenameStr = "rename";
fixed_string RenameExpl = "Renames a C++ item.";

RenameCommand::RenameCommand() : CliCommand(RenameStr, RenameExpl)
{
   BindParm(*new CliTextParm(OldNameExpl, false, 0));
   BindParm(*new CliTextParm(NewNameExpl, false, 0));
}

word RenameCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("RenameCommand.ProcessCommand");

   string oldName, newName, expl;

   if(!GetString(oldName, cli)) return -1;
   if(!GetString(newName, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto rc = Library::Rename(cli, oldName, newName, expl);
   return cli.Report(rc, expl);
}

//------------------------------------------------------------------------------
//
//  The SCAN command.
//
class ScanCommand : public CliCommand
{
public:
   ScanCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string StringPatternExpl = "target string (in quotes; '$' = wildcard)";

fixed_string ScanStr = "scan";
fixed_string ScanExpl = "Scans files for lines that contain a string.";

ScanCommand::ScanCommand() : CliCommand(ScanStr, ScanExpl)
{
   BindParm(*new CliTextParm(FileSetExprExpl, false, 0));
   BindParm(*new CliTextParm(StringPatternExpl, false, 0));
}

word ScanCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ScanCommand.ProcessCommand");

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

   auto set = Singleton<Library>::Instance()->Evaluate(cli, expr, pos);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   auto rc = set->Scan(*cli.obuf, pattern, expl);
   if(rc != 0) return cli.Report(rc, expl);
   return rc;
}

//------------------------------------------------------------------------------
//
//  The SHOW command.
//
class ShowWhatParm : public CliTextParm
{
public: ShowWhatParm();
};

class ShowCommand : public LibraryCommand
{
public:
   static const id_t DirsIndex = 1;
   static const id_t FailIndex = 2;
   static const id_t StatsIndex = 3;

   ShowCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DirsTextStr = "dirs";
fixed_string DirsTextExpl = "code directories";

fixed_string FailTextStr = "failed";
fixed_string FailTextExpl = "code files that failed to parse";

fixed_string StatsTextStr = "stats";
fixed_string StatsTextExpl = "parser statistics";

fixed_string ShowWhatExpl = "what to show...";

ShowWhatParm::ShowWhatParm() : CliTextParm(ShowWhatExpl)
{
   BindText(*new CliText(DirsTextExpl, DirsTextStr), ShowCommand::DirsIndex);
   BindText(*new CliText(FailTextExpl, FailTextStr), ShowCommand::FailIndex);
   BindText(*new CliText(StatsTextExpl, StatsTextStr), ShowCommand::StatsIndex);
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
      const auto& dirs = Singleton<Library>::Instance()->Directories().Items();

      for(auto d = dirs.cbegin(); d != dirs.cend(); ++d)
      {
         auto dir = static_cast<CodeDir*>(*d);
         auto h = dir->HeaderCount();
         *cli.obuf << setw(11) << dir->Name();
         *cli.obuf << setw(6) << h;
         auto c = dir->CppCount();
         *cli.obuf << setw(6) << c;
         *cli.obuf << spaces(2) << dir->Path() << CRLF;
         hdrs += h;
         cpps += c;
      }

      *cli.obuf << setw(11) << "TOTAL" << setw(6) << hdrs
         << setw(6) << cpps << CRLF;
      break;
   }

   case FailIndex:
   {
      auto found = false;
      const auto& files = Singleton<Library>::Instance()->Files().Items();

      for(auto f = files.cbegin(); f != files.cend(); ++f)
      {
         auto file = static_cast<CodeFile*>(*f);
         if(file->ParseStatus() == CodeFile::Failed)
         {
            *cli.obuf << spaces(2) << file->Name() << CRLF;
            found = true;
         }
      }

      if(!found) return cli.Report(0, "No files failed to parse.");
      break;
   }

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
   BindParm(*new CliTextParm(FileSetExprExpl, false, 0));
}

word SortCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SortCommand.ProcessCommand");

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string expl;
   auto rc = set->Sort(*cli.obuf, expl);
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

fixed_string FileNameExpl = "name of source code file";

fixed_string LineNumberExpl = "line number (must contain source code)";

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

fixed_string StartTextStr = "start";
fixed_string StartTextExpl = "start tracing (preconfigure trace using >set)";

fixed_string StopTextStr = "stop";
fixed_string StopTextExpl = "stop tracing";

fixed_string ModeExpl = "action at tracepoint...";

ModeParm::ModeParm() : CliTextParm(ModeExpl)
{
   BindText(*new CliText(BreakTextExpl, BreakTextStr), Break);
   BindText(*new CliText(StartTextExpl, StartTextStr), Start);
   BindText(*new CliText(StopTextExpl, StopTextStr), Stop);
}

class InsertText : public CliText
{
public: InsertText();
};

class RemoveText : public CliText
{
public: RemoveText();
};

class TraceAction : public CliTextParm
{
public:
   TraceAction();

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
   BindParm(*new CliTextParm(FileNameExpl, false, 0));
   BindParm(*new CliIntParm(LineNumberExpl, 0, 999999));
}

fixed_string RemoveTextStr = "remove";
fixed_string RemoveTextExpl = "delete tracepoint";

RemoveText::RemoveText() : CliText(RemoveTextExpl, RemoveTextStr)
{
   BindParm(*new ModeParm);
   BindParm(*new CliTextParm(FileNameExpl, false, 0));
   BindParm(*new CliIntParm(LineNumberExpl, 0, 999999));
}

fixed_string ClearTextStr = "clear";
fixed_string ClearTextExpl = "delete all tracepoints";

fixed_string ListTextStr = "list";
fixed_string ListTextExpl = "list tracepoints";

fixed_string ActionExpl = "subcommand...";

TraceAction::TraceAction() : CliTextParm(ActionExpl)
{
   BindText(*new InsertText, Insert);
   BindText(*new RemoveText, Remove);
   BindText(*new CliText(ClearTextExpl, ClearTextStr), Clear);
   BindText(*new CliText(ListTextExpl, ListTextStr), List);
}

TraceCommand::TraceCommand() : CliCommand(TraceStr, TraceExpl)
{
   BindParm(*new TraceAction);
}

word TraceCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TraceCommand.ProcessCommand");

   id_t action;
   id_t mode;
   string filename;
   word line;

   if(!GetTextIndex(action, cli)) return -1;

   switch(action)
   {
   case TraceAction::Insert:
   case TraceAction::Remove:
      break;

   case TraceAction::Clear:
      if(!cli.EndOfInput()) return -1;
      Context::ClearTracepoints();
      return cli.Report(0, SuccessExpl);

   case TraceAction::List:
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

   auto file = Singleton<Library>::Instance()->FindFile(filename);

   if(file == nullptr)
   {
      return cli.Report(-2, "Source code file not found.");
   }

   if(action == TraceAction::Remove)
   {
      Context::EraseTracepoint(file, line - 1, Tracepoint::Action(mode));
      return cli.Report(0, SuccessExpl);
   }

   const auto& lexer = file->GetLexer();
   auto source = lexer.GetNthLine(line - 1);
   auto type = lexer.LineToType(line - 1);

   if(!LineTypeAttr::Attrs[type].isParsePos)
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
   BindParm(*new CliTextParm(SetExprExpl, false, 0));
}

word TypeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TypeCommand.ProcessCommand");

   auto set = LibraryCommand::Evaluate(cli);
   if(set == nullptr) return cli.Report(-7, AllocationError);

   string result;
   auto rc = set->Show(result);
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

word ExpCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ExpCommand.ProcessCommand");

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

CtIncrement::CtIncrement() : CliIncrement(CtStr, CtExpl)
{
   Debug::ft("CtIncrement.ctor");

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
   BindCommand(*new TraceCommand);
   BindCommand(*new ParseCommand);
   BindCommand(*new CheckCommand);
   BindCommand(*new ExplainCommand);
   BindCommand(*new FixCommand);
   BindCommand(*new FormatCommand);
   BindCommand(*new ExportCommand);
   BindCommand(*new RenameCommand);
   BindCommand(*new LineTypesCommand);
   BindCommand(*new CoverageCommand);
   BindCommand(*new ItemsCommand);
   BindCommand(*new ExpCommand);

   Parser::ResetStats();
}

//------------------------------------------------------------------------------

CtIncrement::~CtIncrement()
{
   Debug::ftnt("CtIncrement.dtor");
}
}
