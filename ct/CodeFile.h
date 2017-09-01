//==============================================================================
//
//  CodeFile.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   struct LineInfo;
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

   //  Returns the number of lines in the file.
   //
   size_t LineCount() const { return lines_; }

   //  Returns true if the file defines a class template.
   //
   bool IsTemplateHeader() const;

   //  Returns true if ITEM is visible within this file or SCOPE because
   //  of a using statement that matches NAME to at least PREFIX.
   //
   UsingMode FindUsingFor(const std::string& name, size_t prefix,
      const CxxScoped* item, const CxxScope* scope) const;

   //  Returns the source code as a string.
   //
   const std::string* GetCode() const { return &code_; }

   //  Returns the files #included by this file.  Used for iteration.
   //
   const SetOfIds& InclList() const { return inclIds_; }

   //  Returns the files that #include this file.  Used for iteration.
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

   //  NOTE: Internally, line numbers start at 0.  When a line number is to
   //  ====  be displayed, it must be incremented.  Similarly, a line number
   //        obtained externally (e.g. via the CLI) must be decremented.
   //
   //  Returns the line number on which POS occurs.  Returns string::npos if
   //  POS is out of range.
   //
   size_t GetLineNum(size_t pos) const;

   //  Sets S to the string for the Nth line of code, excluding the endline,
   //  or EMPTY_STR if N was out of range.  Returns true if N was valid.
   //
   bool GetNthLine(size_t n, std::string& s) const;

   //  Returns the string for the Nth line of code.  Returns EMPTY_STR if N
   //  is out of range.
   //
   std::string GetNthLine(size_t n) const;

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
   void Trim(std::ostream& stream) const;

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
   //  Overridden to prohibit copying.
   //
   CodeFile(const CodeFile& that);
   void operator=(const CodeFile& that);

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
   void CheckProlog() const;

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

   //  Returns TRUE if NAME is made visible by one of the file's using
   //  statements that matches NAME at least to PREFIX.
   //
   bool HasUsingFor(const std::string& name, size_t prefix) const;

   //  Returns TRUE if the file has a forward declaration for ITEM.
   //
   bool HasForwardFor(const CxxNamed* item) const;

   //  Logs WARNING, which occurred on line N.  OFFSET is specific to
   //  WARNING.
   //
   void LogLine(size_t n, Warning warning, size_t offset = 0) const;

   //  Removes, from SET, items that this file declared.
   //
   void EraseInternals(CxxNamedSet& set) const;

   //  Displays, in STREAM, the symbols in SET and where they are defined.
   //  If FQ is set, fully qualified names are displayed.  TITLE describes
   //  the contents of SET.
   //
   static void DisplaySymbols(std::ostream& stream,
      const CxxNamedSet& set, bool fq, const std::string& title);

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

   //  The number of characters in code_.
   //
   size_t size_;

   //  The number of lines in code_.
   //
   size_t lines_;

   //  Information about each line in code_.
   //
   LineInfo* info_;

   //  Assists with parsing code_.
   //
   Lexer lexer_;

   //  The first file #included by this one.
   //
   id_t firstIncl_;

   //  The identifiers of #included files.
   //
   SetOfIds inclIds_;

   //  The identifiers of file that #include this one.
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
