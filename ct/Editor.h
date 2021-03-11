//==============================================================================
//
//  Editor.h
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
#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include "Lexer.h"
#include <cstddef>
#include <string>
#include <vector>
#include "CodeTypes.h"
#include "CodeWarning.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliThread;
}

namespace CodeTools
{
   struct FuncDeclAttrs;
   struct FuncDefnAttrs;
}

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Source code editor.  It structures a file as follows:
//
//  <prolog>          comments containing file name and copyright notice
//  <#include-guard>  if a header file
//  <#include-bases>  #includes of headers that define base classes or
//                    declare functions that this file defines
//  <#include-exts>   #includes for external headers
//  <#include-ints>   #includes for internal headers
//  <forwards>        forward declarations
//  <usings>          using statements
//  <code>            declarations and/or definitions
//
//  The Editor holds its file's source code in the string source_, so here are
//  two common causes of bugs:
//  o Finding a position in the code, editing the code, and then reusing that
//    position after the underlying text has shifted.  An edit can even change
//    CodeWarning.Pos(), so it may need to be reread or accessed later.
//  o Manipulating source_ using string functions such as erase, insert, or
//    replace instead of analogous Editor functions.  The latter invoke the
//    function UpdatePos to update the positions of the C++ items that were
//    created during parsing.  If this is not done, errors can occur during
//    subsequent edits.  It is only safe to manipulate source_ directly when
//    replacing one string with another of the same size.  This is done, for
//    example, when mangling/demangling #include directives.
//
class Editor : public Lexer
{
public:
   //  Creates an editor for FILE, whose code and warnings are loaded
   //
   Editor();

   //  Initializes the editor.
   //
   void Setup(CodeFile* file);

   //  Returns true if the editor has been set up.
   //
   bool IsInitialized() const { return (file_ != nullptr); }

   //  Interactively fixes warnings in the code detected by Check().  If
   //  an error occurs, a non-zero value is returned and EXPL is updated
   //  with an explanation.
   //
   word Fix(CliThread& cli, const FixOptions& opts, string& expl);

   //  Formats the code.  Returns 0 if the file was unchanged, a positive
   //  number after successful changes, and a negative number on failure,
   //  in which case EXPL provides an explanation.
   //
   word Format(string& expl);

   //  Returns the number of commits made during >fix or >format commands.
   //
   static size_t CommitCount() { return Commits_; }

   //  Returns the log, if any, whose .warning matches LOG, whose .offset
   //  matches OFFSET, and whose .item matches ITEM.
   //
   CodeWarning* FindLog
      (const CodeWarning& log, const CxxNamed* item, word offset);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Invokes Write on each editor whose file has changed.  Returns false
   //  if an error occurrs, after updating EXPL with an explanation.
   //
   static bool Commit(const CliThread& cli, string& expl);

   //  Writes out the editor's file.  Returns 0 if the file was successfully
   //  written; other values indicate failure.  Updates EXPL with a reason
   //  for any failure or a message that indicates which file was written.
   //
   word Write(string& expl);

   //  Returns the status of LOG.
   //  o NotFixed: will try to fix
   //  o Pending: previously fixed but not committed
   //  o Fixed: previously fixed and committed, or fixing not supported
   //
   WarningStatus FixStatus(const CodeWarning& log) const;

   //  Displays the code associated with LOG on the CLI.  FILE is set if the
   //  name of the file in which LOG occurs should be displayed.  Returns false
   //  if the code associated with LOG could not be found.
   //
   bool DisplayLog(const CliThread& cli, const CodeWarning& log, bool file) const;

   //  Fixes LOG.  Returns 0 on success.  A return value of -1 means that the
   //  file should be skipped; other values denote more serious errors.  EXPL
   //  is updated to provide any explanation, even when returning 0.  When the
   //  code has been edited, EXPL usually contains the revised line of code.
   //
   word FixWarning(CliThread& cli, const CodeWarning& log, string& expl);

   //  Invokes FixWarning if LOG's status is NotFixed and updates its status
   //  to Pending on success.
   //
   word FixLog(CliThread& cli, CodeWarning& log, string& expl);

   //  Most of the editing functions attempt to fix the warning reported in LOG,
   //  returning 0 on success.  Any other result indicates an error, in which
   //  case EXPL provides an explanation.  A return value of -1 means that the
   //  file should be skipped; other values denote more serious errors.
   //
   word AdjustLineIndentation(const CodeWarning& log, string& expl);
   word AdjustTags(const CodeWarning& log, string& expl);
   word AlignArgumentNames(const CodeWarning& log, string& expl);
   word ChangeAccess(const CodeWarning& log, Cxx::Access acc, string& expl);
   word ChangeClassToNamespace(const CodeWarning& log, string& expl);
   word ChangeClassToStruct(const CodeWarning& log, string& expl);
   word ChangeDebugFtName(CliThread& cli, const CodeWarning& log, string& expl);
   word ChangeOperator(const CodeWarning& log, string& expl);
   word ChangeStructToClass(const CodeWarning& log, string& expl);
   word EraseAdjacentSpaces(const CodeWarning& log, string& expl);
   word EraseAccessControl(const CodeWarning& log, string& expl);
   word EraseBlankLine(const CodeWarning& log, string& expl);
   word EraseClass(const CodeWarning& log, string& expl);
   word EraseConst(const CodeWarning& log, string& expl);
   word EraseData(const CliThread& cli, const CodeWarning& log, string& expl);
   word EraseExplicitTag(const CodeWarning& log, string& expl);
   word EraseForward(const CodeWarning& log, string& expl);
   word EraseLineBreak(const CodeWarning& log, string& expl);
   word EraseMutableTag(const CodeWarning& log, string& expl);
   word EraseOverrideTag(const CodeWarning& log, string& expl);
   word EraseSemicolon(const CodeWarning& log, string& expl);
   word EraseVirtualTag(const CodeWarning& log, string& expl);
   word EraseVoidArgument(const CodeWarning& log, string& expl);
   word InlineDebugFtName(const CodeWarning& log, string& expl);
   word InitByCtorCall(const CodeWarning& log, string& expl);
   word InsertBlankLine(const CodeWarning& log, string& expl);
   word InsertCopyCtorCall(const CodeWarning& log, string& expl);
   word InsertDataInit(const CodeWarning& log, string& expl);
   word InsertDebugFtCall(CliThread& cli, const CodeWarning& log, string& expl);
   word InsertDefaultFunction(const CodeWarning& log, string& expl);
   word InsertDisplay(CliThread& cli, const CodeWarning& log, string& expl);
   word InsertEnumName(const CodeWarning& log, string& expl);
   word InsertForward(const CodeWarning& log, string& expl);
   word InsertInclude(const CodeWarning& log, string& expl);
   word InsertIncludeGuard(const CodeWarning& log, string& expl);
   word InsertLineBreak(const CodeWarning& log, string& expl);
   word InsertMemberInit(const CodeWarning& log, string& expl);
   word InsertPatch(CliThread& cli, const CodeWarning& log, string& expl);
   word InsertPODCtor(const CodeWarning& log, string& expl);
   word InsertPureVirtual(const CodeWarning& log, string& expl);
   word InsertUsing(const CodeWarning& log, string& expl);
   word MoveDefine(const CodeWarning& log, string& expl);
   word MoveFunction(const CodeWarning& log, string& expl);
   word MoveMemberInit(const CodeWarning& log, string& expl);
   word RenameIncludeGuard(const CodeWarning& log, string& expl);
   word ReplaceHeading(const CodeWarning& log, string& expl);
   word ReplaceName(const CodeWarning& log, string& expl);
   word ReplaceNull(const CodeWarning& log, string& expl);
   word ReplaceSlashAsterisk(const CodeWarning& log, string& expl);
   word ReplaceUsing(const CodeWarning& log, string& expl);
   word TagAsConstData(const CodeWarning& log, string& expl);
   word TagAsConstPointer(const CodeWarning& log, string& expl);
   word TagAsExplicit(const CodeWarning& log, string& expl);
   word TagAsOverride(const CodeWarning& log, string& expl);
   word TagAsVirtual(const CodeWarning& log, string& expl);

   //  Removes trailing blanks.  Invoked by Format and before writing out code
   //  that was changed.
   //
   word EraseTrailingBlanks();

   //  Replaces multiple blank lines with a single blank line.  Invoked by
   //  Format and before writing out code that was changed.
   //
   word EraseBlankLinePairs();

   //  Removes a separator that is preceded or followed by a brace or separator.
   //  Invoked by Format and before writing out code that was changed.
   //
   word EraseEmptySeparators();

   //  Removes a blank line that
   //  o precedes or follows an access control
   //  o precedes or follows a left brace
   //  o precedes a right brace
   //  Invoked by Format and before writing out code that was changed.
   //
   word EraseOffsetBlankLines();

   //  Converts tabs to spaces.  Invoked by Format and before writing out code
   //  that was changed.
   //
   word ConvertTabsToBlanks();

   //  Deletes the line break at the end of the line referenced by POS if the
   //  following line will also fit within LineLengthMax.  Returns true if the
   //  line break was deleted.
   //
   bool EraseLineBreak(size_t pos);

   //  Indents POS's line.  Returns POS.
   //
   size_t Indent(size_t pos);

   //  Inserts PREFIX at POS.  The prefix replaces as many blanks as its
   //  length.  If there are fewer blanks, the rest of the line will get
   //  shifted right.  Returns POS.
   //
   size_t InsertPrefix(size_t pos, const string& prefix);

   //  Inserts a line break at POS, indents the new line accordingly, and
   //  returns POS.  Returns string::npos if a line break was not inserted.
   //
   size_t InsertLineBreak(size_t pos);

   //  Inserts the string "//" followed by repetitions of C to fill out the
   //  line.  POS is where to insert.  Returns POS.
   //
   size_t InsertRule(size_t pos, char c);

   //  Returns the first line that follows comments and blanks.
   //
   size_t PrologEnd() const;

   //  Returns true if POS is the start of HASH, a preprocessor directive.
   //
   bool IsDirective(size_t pos, fixed_string hash) const;

   //  Returns the location of the first #include.  Returns end() if no
   //  #include was found.
   //
   size_t IncludesBegin() const;

   //  Returns the location of the statement that follows the last #include.
   //  Returns string::npos if the last line was an #include.
   //
   size_t IncludesEnd() const;

   //  Find the first line of code (other than #include directives, forward
   //  declarations, and using statements).  Moves up past any comments that
   //  precede this point, and returns the position where these comments
   //  begin.
   //
   size_t CodeBegin();

   //  Returns true if the code referenced by POS is followed by more code,
   //  without an intervening comment or right brace.
   //
   bool CodeFollowsImmediately(size_t pos) const;

   //  Returns the start of any comments that precede POS, including an
   //  fn_name definition if funcName is set.  Returns POS if it is not
   //  preceded by any such items.
   //
   size_t IntroStart(size_t pos, bool funcName) const;

   //  This simplifies sorting by replacing the characters that enclose the
   //  file name's in an #include directive.
   //
   word MangleInclude(string& include, string& expl) const;

   //  Before inserting an #include or sorting all #includes, this is invoked
   //  to make it easier to sort them.
   //
   void MangleIncludes();

   //  Sorts #include directives in standard order.
   //
   word SortIncludes(string& expl);

   //  Inserts an INCLUDE directive.
   //
   word InsertInclude(string& include, string& expl);

   //  Searches for INCL starting at POS.  If it is found, it is erased and its
   //  position is returned.  Returns string::npos is INCL is not found.
   //
   size_t FindAndCutInclude(size_t pos, const string& incl);

   //  Invoked after removing a forward declaration at POS.  If the declaration
   //  was in a namespace that is now empty, erases the "namespace <name> { }".
   //
   word EraseEmptyNamespace(size_t pos);

   //  Inserts a FORWARD declaration at POS, which is a namespace definition
   //  that should include FORWARD.
   //
   word InsertForward(size_t pos, const string& forward, string& expl);

   //  Inserts a FORWARD declaration at POS.  It is the first declaration in
   //  NSPACE, so it must be enclosed in a new namespace scope.
   //
   word InsertNamespaceForward(size_t pos,
      const string& nspace, const string& forward, string& expl);

   //  Qualifies symbols so that using statements can be removed.
   //
   word ResolveUsings();

   //  Returns the items within ITEM that were accessed via a using statement.
   //
   CxxNamedSet FindUsingReferents(CxxNamed* item) const;

   //  Qualifies names used within ITEM in order to remove using statements.
   //
   void QualifyUsings(CxxNamed* item);

   //  Within ITEM, qualifies occurrences of REF.
   //
   void QualifyReferent(const CxxNamed* item, const CxxNamed* ref);

   //  Fixes LOG, which also involves modifying overrides of a function.
   //  Updates EXPL with any explanation.
   //
   static word FixFunctions(CliThread& cli, const CodeWarning& log, string& expl);

   //  Fixes LOG, which is associated with FUNC, and updates EXPL with any
   //  explanation.
   //
   word FixFunction(const Function* func, const CodeWarning& log, string& expl);

   //  Fixes LOG, which also involves modifying invokers and overrides of
   //  a function.  Updates EXPL with any explanation.
   //
   word FixInvokers(CliThread& cli, const CodeWarning& log, string& expl);

   //  Fixes LOG, which is associated with invoking FUNC, and updates EXPL with
   //  any explanation.
   //
   word FixInvoker(const Function* func, const CodeWarning& log, string& expl);

   //  Fixes FUNC and updates EXPL with any explanation.  OFFSET is log.offset_
   //  when the original log is associated with one of FUNC's arguments.
   //
   word ChangeFunctionToFree(const Function* func, string& expl);
   word ChangeFunctionToMember(const Function* func, word offset, string& expl);
   word ChangeInvokerToFree(const Function* func, string& expl);
   word ChangeInvokerToMember(const Function* func, word offset, string& expl);
   word EraseArgument(const Function* func, word offset, string& expl);
   word EraseDefault(const Function* func, word offset, string& expl);
   word EraseParameter(const Function* func, word offset, string& expl);
   word EraseNoexceptTag(const Function* func, string& expl);
   word InsertArgument(const Function* func, word offset, string& expl);
   word SplitVirtualFunction(const Function* func, string& expl);
   word TagAsConstArgument(const Function* func, word offset, string& expl);
   word TagAsConstFunction(const Function* func, string& expl);
   word TagAsConstReference(const Function* func, word offset, string& expl);
   word TagAsDefaulted(const Function* func, string& expl);
   word TagAsNoexcept(const Function* func, string& expl);
   word TagAsStaticFunction(const Function* func, string& expl);

   //  Returns the location of the right parenthesis after a function's
   //  argument list.  Returns string::npos on failure.
   //
   size_t FindArgsEnd(const Function* func);

   //  Returns the location of the semicolon after a function's declaration
   //  or the left brace that begins its definition.  Returns string::npos
   //  on failure.
   //
   size_t FindSigEnd(const CodeWarning& log);
   size_t FindSigEnd(const Function* func);

   //  Returns the line that follows FUNC.
   //
   size_t LineAfterFunc(const Function* func) const;

   //  Returns the location where the special member function required to
   //  fix LOG should be inserted.  Updates ATTRS to specify whether the
   //  function should be offset with a blank line and, if so, commented.
   //
   size_t FindSpecialFuncLoc
      (const CodeWarning& log, FuncDeclAttrs& attrs) const;

   //  Returns the location where the function CLS::NAME should be declared.
   //  Returns string::npos if the user decides not to add the function.
   //  Updates ATTRS if the function should be offset with a blank line or
   //  comment.
   //
   size_t FindFuncDeclLoc
      (const Class* cls, const string& name, FuncDeclAttrs& attrs) const;

   //  Returns the location where a new function declaration should be added
   //  after PREV and/or before NEXT.  Updates ATTRS if the function should
   //  be offset with a blank line or comment.
   //
   size_t UpdateFuncDeclLoc
      (const Function* prev, const Function* next, FuncDeclAttrs& attrs) const;

   //  Updates ATTRS based on FUNC.
   //
   void UpdateFuncDeclAttrs(const Function* func, FuncDeclAttrs& attrs) const;

   //  Inserts what goes after a function declaration.  POS is where to insert.
   //  Returns POS.
   //
   size_t InsertAfterFuncDecl(size_t pos, const FuncDeclAttrs& attrs);

   //  Inserts what goes before a function declaration.  POS is where to insert.
   //  point, and COMMENT is any comment.  Returns POS.
   //
   size_t InsertBeforeFuncDecl
      (size_t pos, const FuncDeclAttrs& attrs, const string& comment);

   //  Returns the location where the function CLS::NAME should be defined.
   //  Updates ATTRS if the function should be offset with a rule and/or a
   //  blank line.
   //
   size_t FindFuncDefnLoc(const CodeFile* file, const Class* cls,
      const string& name, string& expl, FuncDefnAttrs& attrs) const;

   //  Returns the location where a new function definition should be added
   //  after PREV and/or before NEXT.  Updates ATTRS if the function should
   //  be offset with a rule and/or a blank line.
   //
   size_t UpdateFuncDefnLoc
      (const Function* prev, const Function* next, FuncDefnAttrs& attrs) const;

   //  Updates ATTRS based on FUNC.
   //
   void UpdateFuncDefnAttrs(const Function* func, FuncDefnAttrs& attrs) const;

   //  Inserts what goes after a function definition.  POS is where to insert.
   //  Returns POS.
   //
   size_t InsertAfterFuncDefn(size_t pos, const FuncDefnAttrs& attrs);

   //  Inserts what goes before a function definition.  POS is where to insert.
   //  Returns POS.
   //
   size_t InsertBeforeFuncDefn(size_t pos, const FuncDefnAttrs& attrs);

   //  Returns the code for a Debug::Ft invocation with an inline string
   //  literal (FNAME).
   //
   string DebugFtCode(const string& fname) const;

   //  Inserts the declaration for a Patch override at POS.
   //
   void InsertPatchDecl(const size_t& pos, const FuncDeclAttrs& attrs);

   //  Inserts the definition for a Patch override in CLS at POS.
   //
   void InsertPatchDefn
      (const size_t& pos, const Class* cls, const FuncDefnAttrs& attrs);

   //  Finds the start of ITEM and backs up to find the starting point for
   //  cutting the item.  Returns string::npos on failure.
   //
   size_t FindCutBegin(const CxxNamed* item) const;

   //  Cuts and returns the code associated with ITEM in CODE.  Comments on
   //  preceding lines, up to the next line of code, are also erased if a
   //  comment or left brace follows the erased code.  Returns the location
   //  that immediately follows the cut.  Returns string::npos and updates
   //  EXPL on failure.
   //
   size_t CutCode(const CxxNamed* item, string& expl, string& code);

   //  Erases the code associated with ITEM.
   //
   word EraseCode(const CxxNamed* item, string& expl);

   //  Erases POS's line and returns the start of the line that followed it.
   //
   size_t EraseLine(size_t pos);

   //  Inserts CODE at POS.  Adds an endline if one isn't found.  Returns POS.
   //
   size_t InsertLine(size_t pos, const string& code);

   //  Erases COUNT characters starting at POS and then inserts CODE.  Should
   //  not be used if an erased susbstring is reinserted by CODE.
   //
   size_t Replace(size_t pos, size_t count, const std::string& code);

   //  Adds the editor to Editors_ and returns 0.
   //
   word Changed();

   //  Sets EXPL to POS's line of code, adds the editor to Editors_, and
   //  returns 0.
   //
   word Changed(size_t pos, string& expl);

   //  Erases COUNT characters starting at POS.
   //
   size_t Erase(size_t pos, size_t count);

   //  Inserts CODE at POS.
   //
   size_t Insert(size_t pos, const string& code);

   //  Pastes CODE, which originally started at FROM, at POS.
   //
   size_t Paste(size_t pos, const string& code, size_t from);

   //  Updates the positions of warnings after an edit.
   //
   void UpdateWarnings(EditorAction action,
      size_t begin, size_t count, size_t from = string::npos) const;

   //  The file from which the source code was obtained.
   //
   CodeFile* file_;

   //  The code being edited.
   //
   string source_;

   //  The file's warnings.
   //
   std::vector< CodeWarning* > warnings_;

   //  Set if the #include directives have been sorted.
   //
   bool sorted_;

   //  Set if type aliases for symbols that were resolved by a using
   //  statement have been added to each class.
   //
   bool aliased_;

   //  The last position from which code was erased.  Only this position
   //  can be used in a Paste operation.
   //
   size_t lastCut_;

   //  The editors that have modified their original code.  This allows an
   //  editor to modify other files (e.g. when a fix requires changes in
   //  both a function's declaration and definition).  After the changes
   //  have been made, all modified files can be committed.
   //
   static std::set< Editor* > Editors_;

   //  The number of files committed so far.
   //
   static size_t Commits_;
};
}
#endif
