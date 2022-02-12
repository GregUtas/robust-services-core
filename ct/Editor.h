//==============================================================================
//
//  Editor.h
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
//  The Editor accesses its source code via Lexer.code_, and its file via
//  Lexer.file_.  Here are two common causes of bugs:
//  o Finding a position in the code, editing the code, and then using that
//    position after the underlying text has shifted as the result of using
//    the Editor functions Erase, Insert, or Replace, or invoking Rename on
//    a C++ item.  An edit can even change CodeWarning.Pos(), so it may also
//    need to be reread or accessed later.  The function UpdateAfterErase
//    can be used to update a string position after each code erasure.
//  o Manipulating code_ using string functions such as erase, insert, or
//    replace instead of analogous Editor functions.  The latter invoke the
//    function UpdatePos to update the positions of the C++ items that were
//    created during parsing.  If this is not done, errors can occur during
//    subsequent edits.  It is only safe to manipulate code_ directly when
//    replacing one string with another of the same size.  However, the goal
//    is always to synchronize changes with the underlying C++ items so that
//    >check and >export produce correct output if re-executed after editing.
//    In fact, comparing .lib files before and after an editing session is a
//    good way to check that this has occurred, and new or modified code is
//    incrementally compiled for this reason.
//
class Editor : public Lexer
{
public:
   //  Creates an editor for FILE, whose code and warnings are loaded
   //
   Editor();

   //  Interactively fixes warnings in the code detected by Check().  If
   //  an error occurs, a non-zero value is returned and EXPL is updated
   //  with an explanation.
   //
   word Fix(CliThread& cli, const FixOptions& opts, string& expl) const;

   //  Formats the code.  Returns a negative value on failure, in which
   //  case EXPL provides provides an explanation.
   //
   word Format(string& expl);

   //  Changes oldName, which appears at or shortly after POS, to newName.
   //
   void Rename(size_t pos, const string& oldName, const string& newName);

   //  Updates any call to Debug::ft when FUNC or its scope was renamed.
   //
   static void UpdateDebugFt(Function* func);

   //  Invokes Write on each editor whose file has changed.
   //
   static void Commit(CliThread& cli);

   //  Returns the number of commits made during >fix or >format commands.
   //
   static size_t CommitCount();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Writes out the editor's file.
   //
   word Write() const;

   //  Returns the status of LOG.
   //  o NotFixed: will try to fix
   //  o Pending: previously fixed but not committed
   //  o Fixed: previously fixed and committed, or fixing not supported
   //
   WarningStatus FixStatus(const CodeWarning& log) const;

   //  Displays the code associated with LOG.  FILE is set if the name of
   //  the file in which LOG occurs should be displayed.  Returns false if
   //  the code associated with LOG could not be found.
   //
   bool DisplayLog(const CodeWarning& log, bool file) const;

   //  Fixes LOG.  Returns 0 on success.  A return value of -1 means that the
   //  file should be skipped; other values denote more serious errors.
   //
   word FixWarning(const CodeWarning& log);

   //  Invokes FixWarning if LOG's status is NotFixed and updates its status
   //  to Pending on success.
   //
   word FixLog(const CodeWarning& log);

   //  Invoked when fixing LOG returned RC.
   //
   static void ReportFix(const CodeWarning* log, word rc);

   //  Invoked when LOG also changed another file, whose edit returned RC.
   //
   void ReportFixInFile(const CodeWarning* log, word rc) const;

   //  Most of the editing functions attempt to fix the warning reported in LOG.
   //
   word AdjustIndentation(const CodeWarning& log);
   word AdjustOperator(const CodeWarning& log);
   word AdjustPunctuation(const CodeWarning& log);
   word AdjustTags(const CodeWarning& log);
   word ChangeAccess(const CodeWarning& log, Cxx::Access acc);
   word ChangeAccess(CxxToken* item, ItemDeclAttrs& attrs);
   word ChangeAssignmentToCtorCall(const CodeWarning& log);
   word ChangeCast(const CodeWarning& log);
   word ChangeClassToNamespace(const CodeWarning& log);
   word ChangeClassToStruct(const CodeWarning& log);
   static word ChangeMemberToFree(const CodeWarning& log);
   word ChangeOperator(const CodeWarning& log);
   word ChangeStructToClass(const CodeWarning& log);
   word EraseAdjacentSpaces(const CodeWarning& log);
   word EraseAccessControl(const CodeWarning& log);
   word EraseBlankLine(const CodeWarning& log);
   word EraseCast(const CodeWarning& log);
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
   word InsertDebugFtCall(const CodeWarning& log);
   word InsertDisplay(const CodeWarning& log);
   word InsertEnumName(const CodeWarning& log);
   word InsertFallthrough(const CodeWarning& log);
   word InsertForward(const CodeWarning& log);
   word InsertInclude(const CodeWarning& log);
   word InsertIncludeGuard(const CodeWarning& log);
   word InsertLineBreak(const CodeWarning& log);
   word InsertMemberInit(const CodeWarning& log);
   word InsertPatch(const CodeWarning& log);
   word InsertPODCtor(const CodeWarning& log);
   word InsertPureVirtual(const CodeWarning& log);
   word InsertUsing(const CodeWarning& log);
   word MoveDefine(const CodeWarning& log);
   word MoveMemberInit(const CodeWarning& log);
   word RenameArgument(const CodeWarning& log);
   word RenameDebugFtArgument(const CodeWarning& log);
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

   //  Adjust the spacing around code_[POS, POS + LEN) based on SPACING.
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
   word EraseEmptyNamespace(SpaceDefn* ns);

   //  Inserts a FORWARD declaration at POS, which is a namespace definition
   //  for NSPACE that should include FORWARD.
   //
   word InsertForward(size_t pos, const string& nspace, const string& forward);

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
   static word FixDatas(const CodeWarning& log);

   //  Fixes LOG, which is associated with DATA.
   //
   word FixData(Data* data, const CodeWarning& log);

   //  Fixes DATA or ITEM, which references DATA.
   //
   word TagAsConstData(const Data* data);
   word TagAsConstPointer(const Data* data);
   word TagAsStaticData(Data* data);

   //  Fixes LOG, which also involves modifying all references to data.
   //
   static word FixReferences(const CodeWarning& log);

   //  Fixes LOG, which involves modifying ITEM.
   //
   word FixReference(const CxxToken* item, const CodeWarning& log);

   //  Returns the code for DEFN.  If DEFN contains a call to Debug::ft,
   //  updates FTARG to its argument.
   //
   std::string GetDefnCode(const CxxScope* defn, CxxToken*& ftarg) const;

   //  DECL is a class member (data or function), and CODE defines a static
   //  namespace item to replace it.  Updates CODE to qualify any members
   //  that belong to DEFN's class or one of its base classes.
   //
   void QualifyClassItems(CxxScope* decl, string& code) const;

   //  Updates CODE by going through ITEMS and qualifying each name defined
   //  in CLS (or one of its base classes).
   //
   static void QualifyClassItems
      (const Class* cls, const CxxNamedSet& items, string& code);

   //  A static data or function declaration with NAME is replacing a class
   //  member that resides in SPACE.  Its definition was cut from POS and must
   //  be placed between MIN and MAX.  Updates ATTRS with the location for the
   //  new item and how it should be set off.
   //
   void FindFreeItemPos(const Namespace* space, const string& name,
      size_t pos, size_t min, size_t max, ItemDefnAttrs& attrs) const;

   //  Updates BEGIN and END to the range where a new declaration for DECL can
   //  appear when changing it from a class member to a static namespace item
   //  in a .cpp.  It must appear before any item that references it and after
   //  any item that the .cpp declares and which DECL uses.  Returns any items
   //  declared above DECL's definition, which will move above the position of
   //  the new static namespace item.
   //
   CxxItemVector FindDeclRange
      (CxxScope* decl, size_t& begin, size_t& end) const;

   //  Replaces member DECL with a static namespace item.
   //
   word ChangeMemberToFree(CxxScope* decl);

   //  ITEM has a log that requires adding a special member function.  Looks
   //  for other logs that also require this and fixes them together.
   //
   word InsertSpecialFunctions(CxxToken* item);

   //  Adds the special member function specified by ROLE to CLS.
   //
   word InsertSpecialFuncDecl(Class* cls, FunctionRole role);

   //  Inserts a shell for implementing a special member function in CLS,
   //  based on ATTRS, when it cannot be defaulted or deleted.
   //
   void InsertSpecialFuncDefn(const Class* cls, const ItemDefnAttrs& attrs);

   //  Fixes LOG, which involves changing or inserting a special member
   //  function.
   //
   word ChangeSpecialFunction(const CodeWarning& log);

   //  Fixes LOG, which involves deletig a special member function.
   //
   word DeleteSpecialFunction(const CodeWarning& log);

   //  Fixes LOG, which also involves modifying overrides of a function.
   //
   static word FixFunctions(const CodeWarning& log);

   //  Fixes LOG, which is associated with FUNC.
   //
   word FixFunction(Function* func, const CodeWarning& log);

   //  Fixes LOG, which also involves modifying invokers and overrides
   //  of a function.
   //
   word FixInvokers(const CodeWarning& log);

   //  Fixes LOG, which is associated with invoking FUNC.
   //
   word FixInvoker(const Function* func, const CodeWarning& log);

   //  Fixes FUNC.  OFFSET is log.offset_ when the original log is
   //  associated with one of FUNC's arguments.
   //
   word ChangeFunctionToMember(const Function* func, word offset);
   word ChangeInvokerToMember(const Function* func, word offset);
   word EraseArgument(const Function* func, word offset);
   word EraseDefaultValue(const Function* func, word offset);
   word EraseParameter(const Function* func, word offset);
   word EraseNoexceptTag(Function* func);
   word InsertArgument(const Function* func, word offset);
   word SplitVirtualFunction(const Function* func);
   word TagAsConstArgument(const Function* func, word offset);
   word TagAsConstFunction(Function* func);
   word TagAsConstReference(const Function* func, word offset);
   word TagAsDefaulted(Function* func);
   word TagAsNoexcept(Function* func);
   word TagAsStaticClassFunction(Function* func);
   word TagAsStaticFreeFunction(Function* func);

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
   word FindSpecialFuncDeclLoc(const Class* cls, ItemDeclAttrs& attrs) const;

   //  Updates ATTRS with the location where an item's declaration should
   //  be added in CLS, and whether it should be offset with a blank line
   //  or comment.
   //
   word UpdateItemDeclLoc(const Class* cls, ItemDeclAttrs& attrs) const;

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
   void MoveItem(const CxxToken* item, size_t& dest, const CxxScoped* prev);

   //  Returns true if functions in the area associated with LOG have
   //  already been sorted as the result of fixing another log.
   //
   bool FunctionsWereSorted(const CodeWarning& log) const;

   //  Returns true if the overrides in the class associated with LOG have
   //  already been sorted as the result of fixing another log.
   //
   bool OverridesWereSorted(const CodeWarning& log) const;

   //  Returns items that immediately precede DEFN and that DEFN uses.  The
   //  items are sorted by position.
   //
   CxxItemVector GetItemsForDefn(const CxxScope* defn) const;

   //  Moves FUNC's definition so it appears in standard order among SORTED.
   //
   void MoveFuncDefn(const FunctionVector& sorted, const Function* func);

   //  Inserts the declaration for a Patch override in CLS, based on ATTRS.
   //
   void InsertPatchDecl(Class* cls, const ItemDeclAttrs& attrs);

   //  Inserts the definition for a Patch override in CLS based on ATTRS.
   //
   void InsertPatchDefn(const Class* cls, const ItemDefnAttrs& attrs);

   //  Returns true if a trailing comment starts at or after POS.
   //
   bool CommentFollows(size_t pos) const;

   //  Updates BEGIN and END to include ITEM's preceding whitespace and
   //  comments, as well as trailing whitespace.
   //
   bool GetSpan(const CxxToken* item, size_t& begin, size_t& end);

   //  Cuts and returns the code associated with ITEM in CODE.  Comments on
   //  preceding lines, up to the next line of code, are also erased if a
   //  comment or left brace follows the erased code.  Returns the location
   //  that immediately follows the cut.  Returns string::npos on failure.
   //
   size_t CutCode(const CxxToken* item, string& code);

   //  Deletes ITEM after erasing its code.
   //
   static word EraseItem(CxxToken* item);

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

   //  Deletes and reparses FUNC's implementation after edits have occurred.
   //  This is usually easier than trying to update the implementation when
   //  items have been changed or added.  Returns true on success.
   //
   bool ReplaceImpl(Function* func) const;

   //  Parses the item inserted at file scope at or after POS in SPACE.
   //  If SPACE is nullptr, the item is added to the global namespace.
   //  Returns the C++ item created by the parse, or nullptr on failure.
   //
   CxxToken* ParseFileItem(size_t pos, Namespace* ns) const;

   //  Parses the item inserted in the definition of CLS at or after POS.
   //  ACCESS is the item's access control.  Returns the C++ item created
   //  by the parse, or nullptr on failure.
   //
   CxxToken* ParseClassItem(size_t pos, Class* cls, Cxx::Access access) const;

   //  Displays a message when parsing at POS failed.  Returns nullptr.
   //
   CxxToken* ParseFailed(size_t pos) const;

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

   //  Updates POS, a position in this file, after erasing code.
   //
   void UpdateAfterErase(size_t& pos) const;

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
   size_t lastErasePos_;

   //  The number of characters erased or inserted during the last edit.
   //
   size_t lastEraseSize_;
};
}
#endif
