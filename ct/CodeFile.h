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
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "CxxFwd.h"
#include "CxxNamed.h"
#include "CxxScoped.h"
#include "Lexer.h"
#include "LibraryTypes.h"
#include "RegCell.h"
#include "SysFile.h"
#include "SysTypes.h"

namespace CodeTools
{
   class Editor;
   class CodeDir;
}

using namespace NodeBase;

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
   id_t Fid() const { return fid_.GetId(); }

   //  Returns the file's name, including its path.
   //
   std::string FullName() const;

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
   //  appears in an #include but was not parsed because its directory was not
   //  added to the build by an >import command.
   //
   bool IsExtFile() const { return dir_ == nullptr; }

   //  Returns true if this is a substitute file.  A substitute file resides in
   //  the subs/ directory and declares a subset of the items that the code base
   //  uses from an external file.
   //
   bool IsSubsFile() const { return isSubsFile_; }

   //  Returns true if the file defines a class template.
   //
   bool IsTemplateHeader() const;

   //  Returns the using statement, if any, that makes ITEM visible within
   //  this file or SCOPE because it matches NAME to at least PREFIX.
   //
   Using* FindUsingFor(const std::string& name, size_t prefix,
      const CxxScoped* item, const CxxScope* scope) const;

   //  Returns a pointer to the original source code.
   //
   const std::string* GetCode() const { return &code_; }

   //  Provides read-only access to the lexer.
   //
   const Lexer& GetLexer() const { return lexer_; }

   //  Returns the files #included by this file.
   //
   const SetOfIds& InclList() const { return inclIds_; }

   //  Returns the files that #include this file.
   //
   const SetOfIds& UserList() const { return userIds_; }

   //  Returns implIds_ (the files that implement this one), constructing
   //  it first if necessary.
   //
   const SetOfIds& Implementers() const;

   //  Returns affecterIds_ (the files that affect this one), constructing
   //  it first if necessary.
   //
   const SetOfIds& Affecters() const;

   //  Returns the file's classes.
   //
   const ClassVector* Classes() const { return &classes_; }

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
   void InsertUsing(UsingPtr& use);

   //  Records that ITEM was used in the file's executable code.
   //
   void AddUsage(const CxxNamed* item);

   //  Reads the file into code_ and preprocesses it.
   //
   void Scan();

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

   //  Invoked when the file defines a function template or a function
   //  in a class template.
   //
   void SetLocation(TemplateLocation loc);

   //  Checks the file after it has been parsed, looking for additional
   //  warnings when a report is to be generated.
   //
   void Check();

   //  Returns the LineType for line N.  Returns LineType_N if N is out
   //  of range.
   //
   LineType GetLineType(size_t n) const;

   //  Generates a report in STREAM about which #include statements are
   //  required and which symbols require qualification to remove using
   //  statements.
   //
   void Trim(std::ostream& stream);

   //  Formats the file.  Returns 0 if the file was unchanged, a positive
   //  number after successful changes, and a negative number on failure,
   //  in which case EXPL provides an explanation.
   //
   word Format(std::string& expl);

   //  Creates an #include directive for including file.
   //
   std::string MakeInclude() const;

   //  Modifications that can be applied to a file.
   //
   enum Modification
   {
      NoChange,
      AddInclude,
      RemoveInclude,
      AddForward,
      RemoveForward,
      AddUsing,
      RemoveUsing
   };

   //  Modifies the file as specified by ACT and ITEM.  Returns 0 if the file
   //  was unchanged, a positive number after successful changes, and a negative
   //  number on failure, in which case EXPL provides an explanation.
   //
   word Modify(Modification act, std::string& item, std::string& expl);

   //  Logs WARNING, which occurred at POS.  OFFSET is specific to WARNING.
   //
   void LogPos(size_t pos, Warning warning, size_t offset = 0) const;

   //  Generates a report in STREAM for the files in SET.  The report includes
   //  line type counts and warnings found during parsing and "execution".
   //
   static void GenerateReport(std::ostream& stream, const SetOfIds& set);

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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Returns the stream created for reading the file.
   //
   istreamPtr Stream() const;

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
   void CheckIncludes() const;

   //  Looks for using statements that should be removed.
   //
   void CheckUsings() const;

   //  Checks vertical separation.
   //
   void CheckSeparation();

   //  Checks if functions are implemented alphabetically.
   //
   void CheckFunctionOrder() const;

   //  Checks invocations of Debug::ft.
   //
   void CheckDebugFt() const;

   //  Returns the data member identified by NAME.
   //
   Data* FindData(const std::string& name) const;

   //  Returns the file's using statement, if any, that makes NAME
   //  visible by matching NAME at least to PREFIX.
   //
   Using* GetUsingFor(const std::string& name, size_t prefix) const;

   //  Returns TRUE if the file has a forward declaration for ITEM.
   //
   bool HasForwardFor(const CxxNamed* item) const;

   //  Logs WARNING, which occurred on line N.  OFFSET is specific to
   //  WARNING.
   //
   void LogLine(size_t n, Warning warning, size_t offset = 0) const;

   //  Returns false if >trim does not apply to this file (e.g. a template
   //  header).  STREAM is where the output for >trim is being directed.
   //
   bool CanBeTrimmed(std::ostream& stream) const;

   //  Updates declIds with the identifiers of files that declare items
   //  that this file (if a .cpp) defines.
   //
   void GetDeclIds(SetOfIds& declIds) const;

   //  Updates SYMBOLS with information about symbols used in this file.
   //  declIds is the result from GetDeclIds.
   //
   void GetUsageInfo(const SetOfIds& declIds, CxxUsageSets& symbols) const;

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

   //  Updates unclIds by removing the files that are #included by any
   //  file in declIds.  This applies to a .cpp only, where declIds is
   //  the set of headers that declare items that the .cpp defines.
   //
   void RemoveHeaderIds(const SetOfIds& declIds, SetOfIds& inclIds) const;

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

   //  Searches usingFiles for a using statement that makes USER visible.  If
   //  no such statement is found, one is created and added to addUsing.
   //
   void FindOrAddUsing(const CxxNamed* user,
      const CodeFileVector usingFiles, CxxNamedSet& addUsing);

   //  Creates an Editor object.  Returns nullptr on failure, updating RC
   //  and EXPL with an explanation.
   //
   Editor* CreateEditor(word& rc, std::string& expl) const;

   //  The file's identifier in the code base.
   //
   RegCell fid_;

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

   //  The first file #included by this one.
   //
   id_t firstIncl_;

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
   mutable SetOfIds implIds_;

   //  The files that affect this one (those that it transitively #includes).
   //  Computed when first needed, after which the cached result is returned.
   //
   mutable SetOfIds affecterIds_;

   //  The file's preprocessor directives and the C++ items that it defines.
   //
   IncludePtrVector incls_;
   DirectivePtrVector dirs_;
   UsingPtrVector usings_;
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
   TemplateLocation location_;
};
}
#endif
