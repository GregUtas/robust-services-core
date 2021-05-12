//==============================================================================
//
//  CodeFile.h
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
#ifndef CODEFILE_H_INCLUDED
#define CODEFILE_H_INCLUDED

#include "LibraryItem.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "CxxFwd.h"
#include "Editor.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

using std::string;

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

   //  Deleted to prohibit copying.
   //
   CodeFile(const CodeFile& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   CodeFile& operator=(const CodeFile& that) = delete;

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

   //  Finds the namespace definition that contains ITEM.
   //
   SpaceDefn* FindNamespaceDefn(const CxxToken* item) const;

   //  Returns the using statement, if any, that makes ITEM visible within
   //  this file or SCOPE because it matches fqName to at least PREFIX.
   //
   Using* FindUsingFor(const std::string& fqName, size_t prefix,
      const CxxScoped* item, const CxxScope* scope);

   //  Returns the files #included by this file.
   //
   const LibItemSet& InclList() const { return inclSet_; }

   //  Returns the files that #include this file.
   //
   const LibItemSet& UserList() const { return userSet_; }

   //  Returns implSet_ (the files that implement this one), constructing
   //  it first if necessary.
   //
   const LibItemSet& Implementers();

   //  Returns affecterSet_ (the files that affect this one), constructing
   //  it first if necessary.
   //
   const LibItemSet& Affecters();

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
   void InsertSpace(SpaceDefnPtr& space);
   void InsertClass(Class* cls);
   void InsertData(Data* data);
   void InsertEnum(Enum* item);
   void InsertForw(Forward* forw);
   void InsertFunc(Function* func);
   void InsertMacro(Macro* macro);
   void InsertType(Typedef* type);
   void InsertUsing(Using* use);
   void InsertAsm(Asm* code);
   void InsertStaticAssert(StaticAssert* assert);

   //  Records that ITEM was used in the file's executable code.
   //
   void AddUsage(CxxNamed* item);

   //  Returns true if ITEM is the last item in the file.
   //
   bool IsLastItem(const CxxNamed* item) const;

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
   void SetParsed(bool passed);

   //  Returns a standard name for an #include guard.  Returns EMPTY_STR
   //  if the file is not a header file.
   //
   std::string MakeGuardName() const;

   //  Returns the group to which the file specified by FILE, FN, or
   //  INCL belongs:
   //    group 1: an external file in declSet_
   //    group 2: an internal file in declSet_
   //    group 3: an external file in baseSet_
   //    group 4: an internal file in baseSet_
   //    group 5: an external file (one in angle brackets)
   //    group 6: an internal file (one in quotes)
   //  Returns 0 if an error occurred.
   //
   size_t CalcGroup(CodeFile* file) const;
   size_t CalcGroup(const std::string& fn) const;
   size_t CalcGroup(const Include& incl) const;

   //  Reads the file into code_ and preprocesses it.
   //
   void Scan();

   //  Includes, in the cross-reference, symbols that appear in the
   //  file's items.
   //
   void AddToXref() const;

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
      const FixOptions& opts, std::string& expl);

   //  Invokes the editor to format the file's source code.
   //
   NodeBase::word Format(std::string& expl);

   //  Updates CODE with the file's raw source code.  Returns false if
   //  the file could not be opened.
   //
   bool ReadCode(std::string& code) const;

   //  Returns the file's preprocessed source code.
   //
   const std::string& GetCode() const { return code_; }

   //  Provides read-only access to the lexer.  But if the editor has been
   //  initialized, it is returned instead.
   //
   const Lexer& GetLexer() const;

   //  Returns a stream for reading the file.
   //
   NodeBase::istreamPtr InputStream() const;

   //  Logs WARNING, which occurred at POS.  OFFSET and INFO are specific to
   //  WARNING.
   //
   void LogPos(size_t pos, Warning warning,
      const CxxToken* item = nullptr, NodeBase::word offset = 0,
      const std::string& info = std::string(NodeBase::EMPTY_STR));

   //  Logs WARNING, which occurred on LINE.
   //
   void LogLine(size_t line, Warning warning);

   //  Invokes FindLog(LOG, ITEM, OFFSET) on the file's editor to find the
   //  log whose .warning matches LOG, whose .offset matches OFFSET, and
   //  whose .item matches ITEM.  Returns that log.
   //
   CodeWarning* FindLog(const CodeWarning& log,
      const CxxToken* item, NodeBase::word offset);

   //  Adds the file's line types to the global count.
   //
   void GetLineCounts() const;

   //  Displays the file's C++ items in STREAM.  The characters in OPTS control
   //  formatting options.
   //
   void DisplayItems(std::ostream& stream, const std::string& opts) const;

   //  Provides access to the editor.
   //
   Editor& GetEditor();

   //  Updates SYMBOLS with information about symbols used in this file.
   //
   void GetUsageInfo(CxxUsageSets& symbols) const;

   //  Invoked to update the position of items when a file has been edited.
   //  Has the same interface as CxxToken::UpdatePos.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from = string::npos) const;

   //  Shrinks containers.
   //
   void Shrink();

   //  Overridden to update ITEMS with ones declared within the file.
   //
   void GetDecls(std::set< CxxNamed* >& items) override;

   //  Overridden to return the file's name.
   //
   const std::string& Name() const override { return name_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Adds FILE as one that #includes this file.
   //
   void AddUser(CodeFile* file);

   //  Returns the file's prolog (comments that should appear at the top
   //  of the file).
   //
   const stringVector& Prolog() const;

   //  Checks the file's prolog.
   //
   void CheckProlog();

   //  Checks the file's preprocessor directives.
   //
   void CheckDirectives() const;

   //  Checks that a header has an #include guard.
   //
   void CheckIncludeGuard();

   //  Looks for #include directives that follow code.
   //
   void CheckIncludes();

   //  Looks for unsorted and duplicated #include directives.
   //
   void CheckIncludeOrder() const;

   //  Looks for using statements that should be removed.
   //
   void CheckUsings() const;

   //  Checks vertical formatting.
   //
   void CheckVerticalSpacing();

   //  Checks for unnecessary line breaks.
   //
   void CheckLineBreaks();

   //  Checks if functions are implemented alphabetically.
   //
   void CheckFunctionOrder() const;

   //  Checks invocations of Debug::ft.
   //
   void CheckDebugFt();

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

   //  Logs WARNING, which occurred at POS within ITEM (which may be nullptr).
   //  OFFSET and INFO are specific to WARNING.
   //
   void LogCode(Warning warning,
      size_t pos, const CxxToken* item, NodeBase::word offset = 0,
      const std::string& info = std::string(NodeBase::EMPTY_STR));

   //  Returns false if >trim does not apply to this file (e.g. a substitute
   //  file).
   //
   bool CanBeTrimmed() const;

   //  Finds the identifiers of files that declare items that this file
   //  (if a .cpp) defines.
   //
   void FindDeclSet();

   //  Saves the identifiers of files that define direct base classes used
   //  by this file.  BASES is from CxxUsageSets.bases.
   //
   void SaveBaseSet(const CxxNamedSet& bases);

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

   //  Adds the files that declare items in declSet to inclSet, excluding
   //  this file.
   //
   void AddIncludes(const CxxNamedSet& declSet, LibItemSet& inclSet) const;

   //  Updates inclSet by removing the files that are #included by any
   //  file in declSet_.  This applies to a .cpp only, where declSet_ is
   //  the set of headers that declare items that the .cpp defines.
   //
   void RemoveHeaders(LibItemSet& inclSet) const;

   //  Removes forward declaration candidates from addForws based on various
   //  criteria.  FORWARDS contains the forward declarations already used by
   //  the file, and trimSet identifies the files that it should #include.
   //
   void PruneForwardCandidates(const CxxNamedSet& forwards,
      const LibItemSet& trimSet, CxxNamedSet& addForws) const;

   //  Returns the files that should be #included by this file.
   //
   const LibItemSet& TrimList() const { return trimSet_; }

   //  Looks at the file's existing forward declarations.  Those that are not
   //  needed are removed from addForws (if present) and added to delForws.
   //
   void PruneLocalForwards(CxxNamedSet& addForws, CxxNamedSet& delForws) const;

   //  Searches the file for a using statement that makes USER visible.  If
   //  no such statement is found, one is created and added to the file.
   //
   void FindOrAddUsing(const CxxNamed* user);

   //  Removes, from addSet, files that should not be added to this file's
   //  #include directives.
   //
   void RemoveInvalidIncludes(LibItemSet& addSet);

   //  Logs an IncludeAdd for each file in FIDS.
   //
   void LogAddIncludes(std::ostream* stream, const LibItemSet& files);

   //  Logs an IncludeRemove for each file in FIDS.
   //
   void LogRemoveIncludes(std::ostream* stream, const LibItemSet& files) const;

   //  Logs a ForwardAdd for each item in ITEMS.
   //
   void LogAddForwards(std::ostream* stream, const CxxNamedSet& items);

   //  Logs a ForwardRemove for each item in ITEMS.
   //
   void LogRemoveForwards(std::ostream* stream, const CxxNamedSet& items) const;

   //  Logs a UsingAdd for using statements that were added by FindOrAddUsing.
   //
   void LogAddUsings(std::ostream* stream);

   //  Logs a UsingRemove for each of the file's using statements that is
   //  marked for removal.
   //
   void LogRemoveUsings(std::ostream* stream) const;

   //  The file's name.
   //
   const std::string name_;

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

   //  The #included files.
   //
   LibItemSet inclSet_;

   //  The #included files.  When >trim is run on this file, this is modified
   //  to be the files that *should* be #included.
   //
   LibItemSet trimSet_;

   //  The files that #include this one.
   //
   LibItemSet userSet_;

   //  The files that implement this one.
   //
   LibItemSet implSet_;

   //  The files that declare items that this file defines.
   //
   LibItemSet declSet_;

   //  The files that define direct base classes that this file uses or
   //  implements.
   //
   LibItemSet baseSet_;

   //  The files that define transitive base classes of the classes implemented
   //  in this file.
   //
   LibItemSet classSet_;

   //  The files that affect this one (those that it transitively #includes).
   //  Computed when first needed, after which the cached result is returned.
   //
   LibItemSet affecterSet_;

   //  The file's preprocessor directives and the C++ items that it defines.
   //
   IncludePtrVector incls_;
   DirectivePtrVector dirs_;
   UsingVector usings_;
   ForwardVector forws_;
   MacroVector macros_;
   SpaceDefnPtrVector spaces_;
   ClassVector classes_;
   EnumVector enums_;
   TypedefVector types_;
   FunctionVector funcs_;
   DataVector data_;
   AsmVector assembly_;
   StaticAssertVector asserts_;

   //  The file's items, in the order in which they appeared.
   //
   std::vector< CxxNamed* > items_;

   //  The items used in the file's executable code.
   //
   CxxNamedSet usages_;

   //  The file's parse status.
   //
   ParseState parsed_;

   //  Set if >check was run on the file.
   //
   bool checked_;

   //  For editing the file's source code.
   //
   Editor editor_;
};
}
#endif
