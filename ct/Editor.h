//==============================================================================
//
//  Editor.h
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
#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include "Lexer.h"
#include <cstddef>
#include <string>
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
   struct ItemOffsets;
   struct ItemDeclAttrs;
   struct ItemDefnAttrs;
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

   //  Formats the code.  Returns a negative value on failure, in which
   //  case EXPL provides provides an explanation.
   //
   word Format(string& expl);

   //  Returns the start of any comments that precede POS, including an
   //  fn_name definition if funcName is set.  Returns POS if it is not
   //  preceded by any such items.
   //
   size_t IntroStart(size_t pos, bool funcName) const;

   //  Returns the number of commits made during >fix or >format commands.
   //
   static size_t CommitCount();

   //  Returns the log, if any, that matches WARNING, ITEM, and OFFSET.
   //
   CodeWarning* FindLog(Warning warning, const CxxToken* item, word offset);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Invokes Write on each editor whose file has changed.
   //
   static void Commit(CliThread& cli);

   //  Writes out the editor's file.
   //
   word Write();

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
   bool DisplayLog
      (const CliThread& cli, const CodeWarning& log, bool file) const;

   //  Fixes LOG.  Returns 0 on success.  A return value of -1 means that the
   //  file should be skipped; other values denote more serious errors.
   //
   word FixWarning(CliThread& cli, CodeWarning& log);

   //  Invokes FixWarning if LOG's status is NotFixed and updates its status
   //  to Pending on success.
   //
   word FixLog(CliThread& cli, CodeWarning& log);

   //  Invoked when fixing LOG returned RC.
   //
   static void ReportFix(CliThread& cli, CodeWarning* log, word rc);

   //  Invoked when LOG also changed another file, whose edit returned RC.
   //
   void ReportFixInFile(CliThread& cli, CodeWarning* log, word rc) const;

   //  Most of the editing functions attempt to fix the warning reported in LOG.
   //
   word AdjustIndentation(const CodeWarning& log);
   word AdjustOperator(const CodeWarning& log);
   word AdjustPunctuation(const CodeWarning& log);
   word AdjustTags(const CodeWarning& log);
   word ChangeAccess(const CodeWarning& log, Cxx::Access acc);
   word ChangeAccess(const CxxToken* item, ItemDeclAttrs& attrs);
   word ChangeAssignmentToCtorCall(const CodeWarning& log);
   word ChangeClassToNamespace(const CodeWarning& log);
   word ChangeClassToStruct(const CodeWarning& log);
   word ChangeOperator(const CodeWarning& log);
   word ChangeStructToClass(const CodeWarning& log);
   word EraseAdjacentSpaces(const CodeWarning& log);
   word EraseAccessControl(const CodeWarning& log);
   word EraseBlankLine(const CodeWarning& log);
   word EraseClass(const CodeWarning& log);
   word EraseConst(const CodeWarning& log);
   word EraseExplicitTag(const CodeWarning& log);
   word EraseForward(const CodeWarning& log);
   word EraseLineBreak(const CodeWarning& log);
   word EraseMutableTag(const CodeWarning& log);
   word EraseOverrideTag(const CodeWarning& log);
   word EraseSemicolon(const CodeWarning& log);
   word EraseScope(const CodeWarning& log);
   word EraseVirtualTag(const CodeWarning& log);
   word EraseVoidArgument(const CodeWarning& log);
   word InlineDebugFtArgument(const CodeWarning& log);
   word InsertBlankLine(const CodeWarning& log);
   word InsertCopyCtorCall(const CodeWarning& log);
   word InsertDataInit(const CodeWarning& log);
   word InsertDebugFtCall(CliThread& cli, const CodeWarning& log);
   word InsertDisplay(CliThread& cli, const CodeWarning& log);
   word InsertEnumName(const CodeWarning& log);
   word InsertForward(const CodeWarning& log);
   word InsertInclude(const CodeWarning& log);
   word InsertIncludeGuard(const CodeWarning& log);
   word InsertLineBreak(const CodeWarning& log);
   word InsertMemberInit(const CodeWarning& log);
   word InsertPatch(CliThread& cli, const CodeWarning& log);
   word InsertPODCtor(const CodeWarning& log);
   word InsertPureVirtual(const CodeWarning& log);
   word InsertUsing(const CodeWarning& log);
   word MoveDefine(const CodeWarning& log);
   word MoveMemberInit(const CodeWarning& log);
   word RenameArgument(CliThread& cli, const CodeWarning& log);
   word RenameDebugFtArgument(CliThread& cli, const CodeWarning& log);
   word RenameIncludeGuard(const CodeWarning& log);
   word ReplaceHeading(const CodeWarning& log);
   word ReplaceName(const CodeWarning& log);
   word ReplaceNull(const CodeWarning& log);
   word ReplaceSlashAsterisk(const CodeWarning& log);
   word ReplaceUsing(const CodeWarning& log);
   word SortFunctions(const CodeWarning& log);
   word SortOverrides(const CodeWarning& log);
   word TagAsExplicit(const CodeWarning& log);
   word TagAsOverride(const CodeWarning& log);
   word TagAsVirtual(const CodeWarning& log);

   //  Removes trailing blanks.  Invoked by Format and before writing out code
   //  that was changed.
   //
   word EraseTrailingBlanks();

   //  Adds and removes lines to realign vertical spacing.  Invoked by Format
   //  and before writing out code that was changed.
   //
   word AdjustVertically();

   //  Converts tabs to spaces.  Invoked by Format and before writing out code
   //  that was changed.
   //
   word ConvertTabsToBlanks();

   //  Deletes the line break at the end of the line referenced by POS if the
   //  following line will also fit within LineLengthMax.  Returns true if the
   //  line break was deleted.
   //
   bool EraseLineBreak(size_t pos);

   //  Indents POS's line.
   //
   word Indent(size_t pos);

   //  Inserts PREFIX at POS.  The prefix replaces as many blanks as its
   //  length.  If there are fewer blanks, the rest of the line will get
   //  shifted right.  Returns POS.
   //
   size_t InsertPrefix(size_t pos, const string& prefix);

   //  Inserts a line break at POS and indents the new line accordingly.
   //
   word InsertLineBreak(size_t pos);

   //  Inserts the string "//" followed by repetitions of C to fill out the
   //  line.  POS is where to insert.  Returns POS.
   //
   size_t InsertRule(size_t pos, char c);

   //  Adjust the spacing around source_[POS, POS + LEN) based on SPACING.
   //  Returns true if an adjustment occurred.
   //
   bool AdjustHorizontally(size_t pos, size_t len, const string& spacing);

   //  Returns the first line that follows comments and blanks.
   //
   size_t PrologEnd() const;

   //  Returns the location of the first #include.  Returns end() if no
   //  #include was found.
   //
   size_t IncludesBegin() const;

   //  Find the first line of code (other than #include directives, forward
   //  declarations, and using statements).  Moves up past any comments that
   //  precede that point, and returns the position where these comments
   //  begin.  This could be, for example, the line after a namespace
   //  definition's left brace.
   //
   size_t CodeBegin() const;

   //  Returns true if the code referenced by POS is followed by more code.
   //
   bool CodeFollowsImmediately(size_t pos) const;

   //  Sorts #include directives in standard order.
   //
   word SortIncludes();

   //  Invoked after removing a forward declaration at POS.  NS is the namespace
   //  that contained the declaration.  If it no longer contains any items, it
   //  is erased.
   //
   word EraseEmptyNamespace(const SpaceDefn* ns);

   //  Inserts a FORWARD declaration at POS, which is a namespace definition
   //  that should include FORWARD.
   //
   word InsertForward(size_t pos, const string& forward);

   //  Inserts a FORWARD declaration at POS.  It is the first declaration in
   //  NSPACE, so it must be enclosed in a new namespace scope.
   //
   word InsertNamespaceForward(size_t pos,
      const string& nspace, const string& forward);

   //  Qualifies symbols so that using statements can be removed.
   //
   word ResolveUsings();

   //  Returns the items within ITEM that were accessed via a using statement.
   //
   CxxNamedSet FindUsingReferents(CxxToken* item) const;

   //  Within ITEM, qualifies occurrences of REF.
   //
   void QualifyReferent(const CxxToken* item, CxxNamed* ref);

   //  Change ITEM from a class/struct (FROM) to a struct/class (TO).
   //
   static void ChangeForwards
      (const CxxToken* item, Cxx::ClassTag from, Cxx::ClassTag to);

   //  Fixes LOG, which also involves modifying a data definition.
   //
   static word FixDatas(CliThread& cli, CodeWarning& log);

   //  Fixes LOG, which is associated with DATA.
   //
   word FixData(const Data* data, const CodeWarning& log);

   //  Fixes DATA or ITEM, which references DATA.
   //
   word ChangeDataToFree(const CxxNamed* item, const Data* data);
   word TagAsConstData(const Data* data);
   word TagAsConstPointer(const Data* data);

   //  Fixes LOG, which also involves modifying all references to data.
   //
   static word FixReferences(CliThread& cli, CodeWarning& log);

   //  Fixes LOG, which involves modifying ITEM.
   //
   word FixReference(const CxxNamed* item, const CodeWarning& log);

   //  ITEM has a log that requires adding a special member function.  Looks
   //  for other logs that also require this and fixes them together.
   //
   word InsertSpecialFunctions(CliThread& cli, const CxxToken* item);

   //  Adds the special member function specified by ROLE to CLS.
   //
   word InsertSpecialFuncDecl
      (CliThread& cli, const Class* cls, FunctionRole role);

   //  Inserts a shell for implementing a special member function in CLS,
   //  based on ATTRS, when it cannot be defaulted or deleted.
   //
   void InsertSpecialFuncDefn(const Class* cls, const ItemDefnAttrs& attrs);

   //  Fixes LOG, which involves changing or inserting a special member
   //  function.
   //
   word ChangeSpecialFunction(CliThread& cli, const CodeWarning& log);

   //  Fixes LOG, which involves deletig a special member function.
   //
   word DeleteSpecialFunction(CliThread& cli, const CodeWarning& log);

   //  Fixes LOG, which also involves modifying overrides of a function.
   //
   static word FixFunctions(CliThread& cli, CodeWarning& log);

   //  Fixes LOG, which is associated with FUNC.
   //
   word FixFunction(const Function* func, const CodeWarning& log);

   //  Fixes LOG, which also involves modifying invokers and overrides
   //  of a function.
   //
   word FixInvokers(CliThread& cli, const CodeWarning& log);

   //  Fixes LOG, which is associated with invoking FUNC.
   //
   word FixInvoker(const Function* func, const CodeWarning& log);

   //  Fixes FUNC.  OFFSET is log.offset_ when the original log is
   //  associated with one of FUNC's arguments.
   //
   word ChangeFunctionToFree(const Function* func);
   word ChangeFunctionToMember(const Function* func, word offset);
   word ChangeInvokerToFree(const Function* func);
   word ChangeInvokerToMember(const Function* func, word offset);
   word EraseArgument(const Function* func, word offset);
   word EraseDefaultValue(const Function* func, word offset);
   word EraseParameter(const Function* func, word offset);
   word EraseNoexceptTag(const Function* func);
   word InsertArgument(const Function* func, word offset);
   word SplitVirtualFunction(const Function* func);
   word TagAsConstArgument(const Function* func, word offset);
   word TagAsConstFunction(const Function* func);
   word TagAsConstReference(const Function* func, word offset);
   word TagAsDefaulted(const Function* func);
   word TagAsNoexcept(const Function* func);
   word TagAsStaticFunction(const Function* func);

   //  Returns the location of the right parenthesis after a function's
   //  argument list.  Returns string::npos on failure.
   //
   size_t FindArgsEnd(const Function* func) const;

   //  Returns the location of the semicolon after a function's declaration
   //  or the left brace that begins its definition.  Returns string::npos
   //  on failure.
   //
   size_t FindSigEnd(const CodeWarning& log);
   size_t FindSigEnd(const Function* func);

   //  Returns the line that follows ITEM.
   //
   size_t LineAfterItem(const CxxToken* item) const;

   //  Updates ATTRS with the location where the item CLS::NAME, of TYPE,
   //  should be declared and whether it should be offset with a blank line
   //  or comment.
   //
   word FindItemDeclLoc
      (const Class* cls, const string& name, ItemDeclAttrs& attrs) const;

   //  Updates ATTRS with the location in CLS where the special member
   //  function specified in ATTRS should be inserted and whether that
   //  function should be offset with a blank line and comment.
   //
   word FindSpecialFuncDeclLoc
      (CliThread& cli, const Class* cls, ItemDeclAttrs& attrs) const;

   //  Updates ATTRS with the location where an item's declaration should be
   //  added in CLS, after PREV and/or before NEXT, and whether it should be
   //  offset with a blank line or comment.
   //
   word UpdateItemDeclLoc(const Class* cls,
      const CxxToken* prev, const CxxToken* next, ItemDeclAttrs& attrs) const;

   //  Updates ATTRS based on ITEM.
   //
   word UpdateItemDeclAttrs(const CxxToken* item, ItemDeclAttrs& attrs) const;

   //  ATTRS has been updated with the position where a new item should be
   //  inserted in CLS.  Determine whether the item's access control needs
   //  to be inserted and whether an access control for the following item
   //  needs to be inserted.
   //
   word UpdateItemControls(const Class* cls, ItemDeclAttrs& attrs) const;

   //  Inserts what goes below a function declaration.
   //
   void InsertBelowItemDecl(const ItemDeclAttrs& attrs);

   //  Inserts what goes above a function declaration.  COMMENT is any comment.
   //
   void InsertAboveItemDecl(const ItemDeclAttrs& attrs, const string& comment);

   //  Updates ATTRS with the position where the function AREA::NAME should
   //  be defined and whether it should be offset with a rule and blank line.
   //
   word FindFuncDefnLoc(const CodeFile* file, const CxxArea* area,
      const string& name, ItemDefnAttrs& attrs) const;

   //  Updates ATTRS with the offsets to be used when ITEM will be added (if
   //  nullptr) or moved (if not nullptr) between PREV and/or NEXT.
   //
   void UpdateItemDefnAttrs(const CxxToken* prev,
      const CxxToken* item, const CxxToken* next, ItemDefnAttrs& attrs) const;

   //  Updates ATTRS with the offsets and position to be used when ITEM will
   //  be added (if nullptr) or moved (if not nullptr) between PREV and/or NEXT.
   //
   word UpdateItemDefnLoc(const CxxToken* prev,
      const CxxToken* item, const CxxToken* next, ItemDefnAttrs& attrs) const;

   //  Updates OFFSET to indicate whether ITEM is preceded and/or followed
   //  by a blank line and/or rule.
   //
   ItemOffsets GetOffsets(const CxxToken* item) const;

   //  Inserts what goes below an item definition based on ATTRS.
   //
   void InsertBelowItemDefn(const ItemDefnAttrs& attrs);

   //  Inserts what goes above an item definition based on ATTRS.
   //
   void InsertAboveItemDefn(const ItemDefnAttrs& attrs);

   //  Moves ITEM to DEST.  PREV, if provided, directly precedes ITEM.  If ITEM
   //  precedes DEST, DEST is updated, because it moves up when ITEM is cut.
   //
   void MoveItem(const CxxNamed* item, size_t& dest, const CxxScoped* prev);

   //  Returns true if functions in the area associated with LOG have
   //  already been sorted as the result of fixing another log.
   //
   bool FunctionsWereSorted(const CodeWarning& log) const;

   //  Returns true if the overrides in the class associated with LOG have
   //  already been sorted as the result of fixing another log.
   //
   bool OverridesWereSorted(const CodeWarning& log) const;

   //  Returns items that immediately precede FUNC and that FUNC uses.  The
   //  items are sorted by position.
   //
   CxxNamedVector GetItemsForFuncDefn(const Function* func) const;

   //  Moves FUNC's definition so it appears in standard order among SORTED.
   //
   void MoveFuncDefn(FunctionVector& sorted, const Function* func);

   //  Returns the code for a Debug::Ft invocation with an inline string
   //  literal (FNAME).
   //
   string DebugFtCode(const string& fname) const;

   //  Inserts the declaration for a Patch override based on ATTRS.
   //
   void InsertPatchDecl(const ItemDeclAttrs& attrs);

   //  Inserts the definition for a Patch override in CLS based on ATTRS.
   //
   void InsertPatchDefn(const Class* cls, const ItemDefnAttrs& attrs);

   //  Returns true if a trailing comment starts at or after POS.
   //
   bool CommentFollows(size_t pos) const;

   //  Updates BEGIN and END to include ITEM's preceding whitespace and
   //  comments, as well as trailing whitespace.
   //
   void GetSpan(const CxxToken* item, size_t& begin, size_t& end);

   //  Cuts and returns the code associated with ITEM in CODE.  Comments on
   //  preceding lines, up to the next line of code, are also erased if a
   //  comment or left brace follows the erased code.  Returns the location
   //  that immediately follows the cut.  Returns string::npos on failure.
   //
   size_t CutCode(const CxxToken* item, string& code);

   //  Deletes ITEM after erasing its code.
   //
   word EraseItem(const CxxToken* item);

   //  Deletes the assignment statement for ITEM after erasing its code.
   //
   word EraseAssignment(const CxxToken* item);

   //  Erases POS's line and returns the start of the line that followed it.
   //
   size_t EraseLine(size_t pos);

   //  Inserts CODE at POS.  Adds an endline if one isn't found.  Returns POS.
   //
   size_t InsertLine(size_t pos, const string& code);

   //  Erases COUNT characters starting at POS and then inserts CODE.  Should
   //  not be used if an erased susbstring is reinserted by CODE.
   //
   size_t Replace(size_t pos, size_t count, const string& code);

   //  Adds the editor to Editors_ and returns 0.
   //
   word Changed();

   //  Invokes SetExpl with POS's line of code, adds the editor to the set of
   //  those that need their files written out, and returns EditSucceeded.
   //
   word Changed(size_t pos);

   //  Erases COUNT characters starting at POS.  Returns POS.
   //
   size_t Erase(size_t pos, size_t count);

   //  Inserts CODE at POS.  Returns POS..
   //
   size_t Insert(size_t pos, const string& code);

   //  Pastes CODE, which originally started at FROM, at POS.  Returns POS.
   //
   size_t Paste(size_t pos, const string& code, size_t from);

   //  Updates the positions of lines, items, and warnings after an edit.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from = string::npos);

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
};
}
#endif
