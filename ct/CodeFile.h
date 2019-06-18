//==============================================================================
//
//  CodeFile.h
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
#ifndef CODEFILE_H_INCLUDED
#define CODEFILE_H_INCLUDED

#include "LibraryItem.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "CxxFwd.h"
#include "Lexer.h"
#include "LibraryTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

namespace CodeTools
{
   struct CxxUsageSets;
}

namespace NodeBase
{
   class CliThread;
}

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Provides access to a .h or .cpp file.
//
class CodeFile: public LibraryItem
{
public:
   //  Creates an instance for the file identified by NAME, which is located
   //  in DIR.  DIR is nullptr if the directory is unknown.  This occurs when
   //  NAME appears in an #include directive before its file is processed.
   //
   CodeFile(const std::string& name, CodeDir* dir);

   //  Not subclassed.
   //
   ~CodeFile();

   //  Returns the file's identifier.
   //
   NodeBase::id_t Fid() const { return fid_.GetId(); }

   //  Returns the file's path.  If FULL is not set, the path to the source
   //  code directory is removed from the front of the path.
   //
   std::string Path(bool full = true) const;

   //  Returns the file's directory.  If the file is first encountered in an
   //  #include directive, its directory is unknown until the file is found.
   //  If the file's directory was not included by >import, this function will
   //  always return nullptr.
   //
   CodeDir* Dir() const { return dir_; }

   //  Sets the file's directory.
   //
   void SetDir(CodeDir* dir);

   //  Returns true if the file is a .h file.
   //
   bool IsHeader() const { return isHeader_; }

   //  Returns true if the file is a .cpp file.
   //
   bool IsCpp() const { return !isHeader_; }

   //  Returns true if this is an external file.  An external file is one that
   //  appears in an #include but whose directory has not yet been added to the
   //  build by an >import command.
   //
   bool IsExtFile() const { return dir_ == nullptr; }

   //  Returns true if this is a substitute file.  A substitute file resides in
   //  the subs/ directory and declares a subset of the items that the code base
   //  uses from files that are external to RSC.
   //
   bool IsSubsFile() const { return isSubsFile_; }

   //  Returns true if the file defines a class template.
   //
   bool IsTemplateHeader() const;

   //  Returns the using statement, if any, that makes ITEM visible within
   //  this file or SCOPE because it matches fqName to at least PREFIX.
   //
   Using* FindUsingFor(const std::string& fqName, size_t prefix,
      const CxxScoped* item, const CxxScope* scope) const;

   //  Returns the files #included by this file.
   //
   const SetOfIds& InclList() const { return inclIds_; }

   //  Returns the files that #include this file.
   //
   const SetOfIds& UserList() const { return userIds_; }

   //  Returns implIds_ (the files that implement this one), constructing
   //  it first if necessary.
   //
   const SetOfIds& Implementers();

   //  Returns affecterIds_ (the files that affect this one), constructing
   //  it first if necessary.
   //
   const SetOfIds& Affecters() const;

   //  Returns the file's code items.
   //
   const ClassVector* Classes() const { return &classes_; }
   const DataVector* Datas() const { return &data_; }
   const EnumVector* Enums() const { return &enums_; }
   const FunctionVector* Funcs() const { return &funcs_; }
   const TypedefVector* Types() const { return &types_; }

   //  Adds the item to those defined in this file.
   //
   void InsertInclude(IncludePtr& incl);
   Include* InsertInclude(const std::string& fn);
   bool InsertDirective(DirectivePtr& dir);
   void InsertClass(Class* cls);
   void InsertData(Data* data);
   void InsertEnum(Enum* item);
   void InsertForw(Forward* forw);
   void InsertFunc(Function* func);
   void InsertMacro(Macro* macro);
   void InsertType(Typedef* type);
   void InsertUsing(Using* use);

   //  Records that ITEM was used in the file's executable code.
   //
   void AddUsage(const CxxNamed* item);

   //  Invoked when the file defines a function template or a function
   //  in a class template.
   //
   void SetTemplate(TemplateType type);

   enum ParseState
   {
      Unparsed,
      Failed,
      Passed
   };

   //  Returns the file's parse status.
   //
   ParseState ParseStatus() const { return parsed_; }

   //  Updates the file's parse status.
   //
   void SetParsed(bool passed) { parsed_ = (passed ? Passed : Failed); }

   //  Classifies a line of code (S) and updates WARNINGS with any warnings
   //  that were found.
   //
   static LineType ClassifyLine(std::string s, std::set< Warning >& warnings);

   //  Returns the LineType for line N.  Returns LineType_N if N is out
   //  of range.
   //
   LineType GetLineType(size_t n) const;

   //  Returns the level of indentation for a line.
   //
   int8_t GetDepth(size_t line) const;

   //  Returns a standard name for an #include guard.  Returns EMPTY_STR
   //  if the file is not a header file.
   //
   std::string MakeGuardName() const;

   //  Returns the group to which the file specified by FILE, FN, or
   //  INCL belongs:
   //    group 1: an external file in declIds_
   //    group 2: an interal file in declIds_
   //    group 3: an external file in baseIds_
   //    group 4: an interal file in baseIds_
   //    group 5: an external file (one in angle brackets)
   //    group 6: an internal file (one in quotes)
   //  Returns 0 if an error occurred.
   //
   int CalcGroup(const CodeFile* file) const;
   int CalcGroup(const std::string& fn) const;
   int CalcGroup(const Include& incl) const;

   //  Reads the file into code_ and preprocesses it.
   //
   void Scan();

   //  Checks the file after it has been parsed, looking for additional
   //  warnings when a report is to be generated.
   //
   void Check();

   //  Generates a report in STREAM about which #include statements are
   //  required and which symbols require qualification to remove using
   //  statements.  Also invoked by Check, with STREAM as nullptr.
   //
   void Trim(std::ostream* stream);

   //  Invokes the editor to interactively fix warnings found by Check().
   //
   NodeBase::word Fix(NodeBase::CliThread& cli,
      const FixOptions& opts, std::string& expl) const;

   //  Invokes the editor to format the file's source code.
   //
   NodeBase::word Format(std::string& expl) const;

   //  Returns a pointer to the original source code.
   //
   const std::string& GetCode() const { return code_; }

   //  Provides read-only access to the lexer.
   //
   const Lexer& GetLexer() const { return lexer_; }

   //  Provides access to the editor, creating it if necessary.  Updates
   //  EXPL with an error message if the editor could not be created.
   //
   Editor* GetEditor(std::string& expl) const;

   //  Returns a stream for reading the file.
   //
   NodeBase::istreamPtr InputStream() const;

   //  Logs WARNING, which occurred at POS.  OFFSET and INFO are specific to
   //  WARNING.
   //
   void LogPos(size_t pos, Warning warning,
      const CxxNamed* item = nullptr, size_t offset = 0,
      const std::string& info = std::string(NodeBase::EMPTY_STR),
      bool hide = false) const;

   //  Invokes FindLog(LOG, ITEM, OFFSET) on the file's editor to find the
   //  log whose .warning matches LOG, whose .offset matches OFFSET, and
   //  whose .item matches ITEM.  Returns that log.  Updates EXPL with an
   //  error message if the editor could not be created.
   //
   CodeWarning* FindLog(const CodeWarning& log,
      const CxxNamed* item, NodeBase::word offset, std::string& expl) const;

   //  Generates a report in STREAM (if not nullptr) for the files in SET.  The
   //  report includes line type counts and warnings found during parsing and
   //  "execution".
   //
   static void GenerateReport(std::ostream* stream, const SetOfIds& set);

   //  Adds the file's line types to the global count.
   //
   void GetLineCounts() const;

   //  Displays the file's C++ items in STREAM.  The characters in OPTS control
   //  formatting options.
   //
   void DisplayItems(std::ostream& stream, const std::string& opts) const;

   //  Shrinks containers.
   //
   void Shrink();

   //  Returns the offset to link_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Adds FILE as one that #includes this file.
   //
   void AddUser(const CodeFile* file);

   //  Classifies the Nth line of code and looks for some warnings.
   //
   LineType ClassifyLine(size_t n);

   //  Checks for the standard lines that should appear at the top
   //  of each file.
   //
   void CheckProlog();

   //  Looks for and checks an #include guard.
   //
   void CheckIncludeGuard();

   //  Looks for #include directives that should be removed.
   //
   void CheckIncludeOrder() const;

   //  Looks for using statements that should be removed.
   //
   void CheckUsings() const;

   //  Checks vertical separation.
   //
   void CheckSeparation();

   //  Checks for unnecessary line breaks.
   //
   void CheckLineBreaks();

   //  Checks if functions are implemented alphabetically.
   //
   void CheckFunctionOrder() const;

   //  Checks invocations of Debug::ft.
   //
   void CheckDebugFt() const;

   //  Returns the data member identified by NAME.
   //
   Data* FindData(const std::string& name) const;

   //  Returns the using statement, if any, that makes ITEM visible within
   //  SCOPE (in this file) because it matches fqName to at least PREFIX.
   //
   Using* GetUsingFor(const std::string& fqName, size_t prefix,
      const CxxNamed* item, const CxxScope* scope) const;

   //  Returns true if the file has a forward declaration for ITEM.
   //
   bool HasForwardFor(const CxxNamed* item) const;

   //  Logs WARNING, which occurred on LINE.  OFFSET and INFO are specific
   //  to WARNING.
   //
   void LogLine(size_t line, Warning warning, size_t offset = 0,
      const std::string& info = std::string(NodeBase::EMPTY_STR),
      bool hide = false) const;

   //  Logs WARNING, which occurred on LINE and POS within ITEM (which may be
   //  nullptr).  OFFSET and INFO arespecific to WARNING.
   //
   void LogCode(Warning warning, size_t line, size_t pos,
      const CxxNamed* item, size_t offset = 0,
      const std::string& info = std::string(NodeBase::EMPTY_STR),
      bool hide = false) const;

   //  Returns false if >trim does not apply to this file (e.g. a template
   //  header).
   //
   bool CanBeTrimmed() const;

   //  Finds the identifiers of files that declare items that this file
   //  (if a .cpp) defines.
   //
   void FindDeclIds();

   //  Records, in the global cross-reference, that the file used ITEMS.
   //
   void InsertInXref(const CxxNamedSet& items) const;

   //  Saves the identifiers of files that define direct base classes used
   //  by this file.  BASES is from CxxUsageSets.bases.
   //
   void SaveBaseIds(const CxxNamedSet& bases);

   //  Updates SYMBOLS with information about symbols used in this file.
   //
   void GetUsageInfo(CxxUsageSets& symbols) const;

   //  Removes, from SET, items that this file declared.
   //
   void EraseInternals(CxxNamedSet& set) const;

   //  Updates inclSet by adding types that this file used directly, which
   //  includes types used directly (in DIRECTS) or in executable code.
   //
   void AddDirectTypes(const CxxNamedSet& directs, CxxNamedSet& inclSet) const;

   //  Updates inclSet by adding types that this file used indirectly (in
   //  INDIRECTS) and that are defined within the code base.
   //
   void AddIndirectExternalTypes
      (const CxxNamedSet& indirects, CxxNamedSet& inclSet) const;

   //  Resets BASES to base classes for those *declared* in this file.
   //
   void GetDeclaredBaseClasses(CxxNamedSet& bases) const;

   //  Adds the files that declare items in inclSet to inclIds, excluding
   //  this file.
   //
   void AddIncludeIds(const CxxNamedSet& inclSet, SetOfIds& inclIds) const;

   //  Updates inclIds by removing the files that are #included by any
   //  file in declIds_.  This applies to a .cpp only, where declIds_ is
   //  the set of headers that declare items that the .cpp defines.
   //
   void RemoveHeaderIds(SetOfIds& inclIds) const;

   //  Removes forward declaration candidates from addForws based on various
   //  criteria.  FORWARDS contains the forward declarations already used by
   //  the file, and inclIds identifies the files that it should #include.
   //
   void PruneForwardCandidates(const CxxNamedSet& forwards,
      const SetOfIds& inclIds, CxxNamedSet& addForws) const;

   //  Returns the files that should be #included by this file.
   //
   const SetOfIds& TrimList() const { return trimIds_; }

   //  Looks at the file's existing forward declarations.  Those that are not
   //  needed are removed from addForws (if present) and added to delForws.
   //
   void PruneLocalForwards(CxxNamedSet& addForws, CxxNamedSet& delForws) const;

   //  Searches the file for a using statement that makes USER visible.  If
   //  no such statement is found, one is created and added to the file.
   //
   void FindOrAddUsing(const CxxNamed* user);

   //  Logs an IncludeAdd for each file in FIDS.
   //
   void LogAddIncludes(std::ostream* stream, const SetOfIds& fids) const;

   //  Logs an IncludeRemove for each file in FIDS.
   //
   void LogRemoveIncludes(std::ostream* stream, const SetOfIds& fids) const;

   //  Logs a ForwardAdd for each item in ITEMS.
   //
   void LogAddForwards(std::ostream* stream, const CxxNamedSet& items) const;

   //  Logs a ForwardRemove for each item in ITEMS.
   //
   void LogRemoveForwards(std::ostream* stream, const CxxNamedSet& items) const;

   //  Logs a UsingAdd for using statements that were added by FindOrAddUsing.
   //
   void LogAddUsings(std::ostream* stream) const;

   //  Logs a UsingRemove for each of the file's using statements that is
   //  marked for removal.
   //
   void LogRemoveUsings(std::ostream* stream) const;

   //  Creates the editor.  On failure, returns a non-zero value and updates
   //  EXPL with an explanation.
   //
   NodeBase::word CreateEditor(std::string& expl) const;

   //  The file's identifier in the code base.
   //
   const NodeBase::RegCell fid_;

   //  The file's directory.
   //
   CodeDir* dir_;

   //  Set for a .h file.
   //
   bool isHeader_;

   //  Set for a substitute file.
   //
   bool isSubsFile_;

   //  The file's source code.
   //
   std::string code_;

   //  Assists with parsing code_.
   //
   Lexer lexer_;

   //  The type of each line in code_.
   //
   std::vector< LineType > lineType_;

   //  The identifiers of #included files.
   //
   SetOfIds inclIds_;

   //  The identifiers of #included files.  When >trim is run on this file,
   //  this is modified to the files that *should* be #included.
   //
   SetOfIds trimIds_;

   //  The identifiers of files that #include this one.
   //
   SetOfIds userIds_;

   //  The identifiers of files that implement this one.
   //
   SetOfIds implIds_;

   //  The identifiers of files that declare items that this file defines.
   //
   SetOfIds declIds_;

   //  The identifiers of files that define direct base classes that this
   //  file uses or implements.
   //
   SetOfIds baseIds_;

   //  The identifiers of files that define transitive base classes of the
   //  classes implemented in this file.
   //
   SetOfIds classIds_;

   //  The files that affect this one (those that it transitively #includes).
   //  Computed when first needed, after which the cached result is returned.
   //
   mutable SetOfIds affecterIds_;

   //  The file's preprocessor directives and the C++ items that it defines.
   //
   IncludePtrVector incls_;
   DirectivePtrVector dirs_;
   UsingVector usings_;
   ForwardVector forws_;
   MacroVector macros_;
   ClassVector classes_;
   EnumVector enums_;
   TypedefVector types_;
   FunctionVector funcs_;
   DataVector data_;

   //  The file's items, in the order in which they appeared.
   //
   NamedVector items_;

   //  The items used in the file's executable code.
   //
   CxxNamedSet usages_;

   //  Set if a /* comment is open during a lexical scan.
   //
   bool slashAsterisk_;

   //  The file's parse status.
   //
   ParseState parsed_;

   //  Whether any of the file's functions involve templates.
   //
   TemplateType template_;

   //  Set if >check was run on the file.
   //
   bool checked_;

   //  For editing the file's source code.
   //
   mutable EditorPtr editor_;
};
}
#endif
