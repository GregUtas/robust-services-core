//==============================================================================
//
//  Editor.h
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
#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <cstddef>
#include <list>
#include <string>
#include "CodeTypes.h"
#include "CodeWarning.h"
#include "CxxFwd.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliThread;
}

using std::string;
using NodeBase::word;

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
class Editor
{
public:
   //  Creates an editor for the source code in FILE, which is read from INPUT.
   //
   Editor(const CodeFile* file, NodeBase::istreamPtr& input);

   //  Not subclassed.
   //
   ~Editor() = default;

   //  Interactively fixes warnings in the code detected by Check().  If
   //  an error occurs, a non-zero value is returned and EXPL is updated
   //  with an explanation.
   //
   word Fix(NodeBase::CliThread& cli, string& expl);

   //  Formats the code.  Returns 0 if the file was unchanged, a positive
   //  number after successful changes, and a negative number on failure,
   //  in which case EXPL provides an explanation.
   //
   word Format(string& expl);
private:
   //  Reads the source code.  If an error occurs, a non-zero value is returned
   //  and EXPL is updated with an explanation.
   //
   word Read(string& expl);

   //  Writes out the file to PATH if it was changed during editing.  Returns 0
   //  if the file had not been changed, 1 if it was successfully written, and a
   //  negative value if an error occurred.
   //
   word Write(const string& path, string& expl);

   //  Returns the status of LOG.
   //  o NotFixed: will try to fix
   //  o Pending: previously fixed but not committed
   //  o Fixed: previously fixed and committed, or fixing not supported
   //
   WarningStatus FixStatus(const WarningLog& log) const;

   //  Displays the code associated with LOG on the CLI.  FILE is set if the
   //  name of the file in which LOG occurs should be displayed.
   //
   void DisplayLog
      (const NodeBase::CliThread& cli, const WarningLog& log, bool file);

   //  Fixes LOG.  Returns 0 on success.  A return value of -1 means that the
   //  file should be skipped; other values denote more serious errors.  EXPL
   //  is updated to provide any explanation, even when returning 0.  When the
   //  code has been edited, EXPL usually contains the revised line of code.
   //
   word FixWarning(const WarningLog& log, string& expl);

   //  Most of the editing functions attempt to fix the warning reported in LOG,
   //  returning 0 on success.  Any other result indicates an error, in which
   //  case EXPL provides an explanation.  A return value of -1 means that the
   //  file should be skipped; other values denote more serious errors.
   //
   word ReplaceSlashAsterisk(const WarningLog& log, string& expl);
   word ReplaceNull(const WarningLog& log, string& expl);
   word AdjustTags(const WarningLog& log, string& expl);
   word EraseSemicolon(const WarningLog& log, string& expl);
   word EraseConst(const WarningLog& log, string& expl);
   word InsertIncludeGuard(const WarningLog& log, string& expl);
   word SortIncludes(const WarningLog& log, string& expl);
   word InsertInclude(const WarningLog& log, string& expl);
   word EraseInclude(const WarningLog& log, string& expl);
   word ResolveUsings(const WarningLog& log, string& expl);
   word ReplaceUsing(const WarningLog& log, string& expl);
   word InsertUsing(const WarningLog& log, string& expl);
   word EraseUsing(const WarningLog& log, string& expl);
   word InsertForward(const WarningLog& log, string& expl);
   word EraseForward(const WarningLog& log, string& expl);
   word ChangeClassToStruct(const WarningLog& log, string& expl);
   word ChangeStructToClass(const WarningLog& log, string& expl);
   word EraseAccessControl(const WarningLog& log, string& expl);
   word TagAsConstData(const WarningLog& log, string& expl);
   word TagAsConstPointer(const WarningLog& log, string& expl);
   word UntagAsMutable(const WarningLog& log, string& expl);
   word TagAsExplicit(const WarningLog& log, string& expl);
   word TagAsVirtual(const WarningLog& log, string& expl);
   word TagAsOverride(const WarningLog& log, string& expl);
   word EraseVoidArgument(const WarningLog& log, string& expl);
   word AlignArgumentNames(const WarningLog& log, string& expl);
   word TagAsConstReference(const WarningLog& log, string& expl);
   word TagAsConstArgument(const WarningLog& log, string& expl);
   word TagAsConstFunction(const WarningLog& log, string& expl);
   word TagAsStaticFunction(const WarningLog& log, string& expl);
   word AdjustLineIndentation(const WarningLog& log, string& expl);
   word EraseAdjacentSpaces(const WarningLog& log, string& expl);
   word InsertBlankLine(const WarningLog& log, string& expl);
   word EraseBlankLine(const WarningLog& log, string& expl);
   word InsertLineBreak(const WarningLog& log, string& expl);
   word RenameIncludeGuard(const WarningLog& log, string& expl);
   word InsertDebugFtCall(const WarningLog& log, string& expl);
   word ChangeDebugFtName(const WarningLog& log, string& expl);
   word TagAsDefaulted(const WarningLog& log, string& expl);
   word InitByConstructor(const WarningLog& log, string& expl);

   //  Replaces multiple blank lines with a single blank line.  Always invoked
   //  on source that was changed.
   //
   word EraseBlankLinePairs();

   //  Removes trailing blanks.  Always invoked on source that was changed.
   //
   word EraseTrailingBlanks();

   //  Converts tabs to spaces.  Always invoked on source that was changed.
   //
   word ConvertTabsToBlanks();

   //  Stores a line of code.
   //
   struct SourceLine
   {
      SourceLine(const string& code, size_t line) : line(line), code(code) { }

      //  The code's line number (the first line is 0, the same as CodeWarning
      //  and Lexer).  A line added by the editor has a line number of SIZE_MAX.
      //
      const size_t line;

      //  The code.
      //
      string code;
   };

   //  The code is kept in a list.
   //
   typedef std::list< SourceLine > SourceList;

   //  Iterator for the source code.  This is an editor, so it doesn't bother
   //  with const iterators.
   //
   typedef std::list< SourceLine >::iterator Iter;

   //  Identifies a line of code and a position within that line.
   //
   struct CodeLocation
   {
      Iter iter;   // location in SourceList
      size_t pos;  // position in iter->code

      CodeLocation(const Iter& i, size_t p) : iter(i), pos(p) { }
   };

   //  Returns the position after CURR.
   //
   CodeLocation NextPos(const CodeLocation& curr);

   //  Returns the position before CURR.
   //
   CodeLocation PrevPos(const CodeLocation& curr);

   //  Returns the type of line referenced by ITER.
   //
   LineType GetLineType(const Iter& iter) const;

   //  Reads in the file's source code.
   //
   word GetCode(string& expl);

   //  Adds a line of source code from the file.  NEVER used to add new code.
   //
   void PushBack(const string& code);

   //  Returns the location of LINE.  Returns source_.end() if LINE is
   //  not found.
   //
   Iter FindLine(size_t line);

   //  Returns the location of LINE.  If LINE is not found, sets EXPL to
   //  an error message and returns source_.end().
   //
   Iter FindLine(size_t line, string& expl);

   //  Converts POS in the original source code to a line number and
   //  and offset.
   //
   CodeLocation FindPos(size_t pos);

   //  Looks for STR starting at iter->code[off].  If STR is found, returns
   //  its location, else returns {source_.end(), string::npos}.
   //
   CodeLocation Find(Iter iter, const string& str, size_t off = 0);

   //  The same as Find(iter, s, off), but searches backwards.
   //
   CodeLocation Rfind(Iter iter, const string& str, size_t off = string::npos);

   //  Returns the location of ID, starting at iter->code[pos].  Returns
   //  {source_.end(), string::npos} if ID was not found.  ID must be an
   //  identifier or keyword that is delimited by punctuation.  The search
   //  spans RANGE lines; if RANGE is nullptr, only one line is searched.
   //
   CodeLocation FindWord
      (Iter iter, size_t pos, const string& id, size_t* range = nullptr);

   //  Returns the location of the first non-blank character starting at
   //  iter->code[pos].  Returns {source_.end(), string::npos} if no such
   //  character was found.
   //
   CodeLocation FindNonBlank(Iter iter, size_t pos);

   //  Returns the location of the first non-blank character starting at
   //  iter->code[pos], reversing.  Returns {source_.end(), string::npos}
   //  if no such character was found.
   //
   CodeLocation RfindNonBlank(Iter iter, size_t pos);

   //  Returns the first occurrence of a character in CHARS, starting at
   //  iter->code[off].  Returns {source_.end(), string::npos} if none
   //  of those characters was found.
   //
   CodeLocation FindFirstOf(Iter iter, size_t off, const string& chars);

   //  Inserts CODE at ITER and returns its location.
   //
   Iter Insert(Iter iter, const string& code);

   //  Inserts PREFIX on the line identified by ITER, starting at POS.  The
   //  prefix replaces blanks but leaves at least one space between it and
   //  the first non-blank character on the line.
   //
   void InsertPrefix(const Iter& iter, size_t pos, const string& prefix);

   //  Inserts a line break before POS, indents the new line accordingly,
   //  and returns the location of the new line.  Returns source_.end() if
   //  a line break was not inserted because the new line would have been
   //  empty.
   //
   Iter InsertLineBreak(const Iter& iter, size_t pos);

   //  Deletes the line break at the end of the line referenced by ITER if
   //  the following line will also fit within LINE_LENGTH_MAX.  Returns
   //  true if the line break was deleted.
   //
   bool DeleteLineBreak(const Iter& iter);

   //  Returns the first line that follows comments and blanks.
   //
   Iter PrologEnd();

   //  Returns the location of the first #include.  Returns source_.end() if
   //  no #include was found.
   //
   Iter IncludesBegin();

   //  Returns the location of the statement that follows the last #include.
   //  Returns source_.end() if the last line was an #include.
   //
   Iter IncludesEnd();

   //  Returns true if the file contains no #include.
   //
   bool IncludesEmpty();

   //  Returns the location of the last #include.  Returns source_end() if
   //  no #includ was found.
   //
   Iter IncludesBack();

   //  Find the first line of code (other than #include directives, forward
   //  declarations, and using statements).  Moves up past any comments that
   //  precede this point, and returns the position where these comments
   //  begin.
   //
   Iter CodeBegin();

   //  If INCLUDE specifies a file in groups 1 to 4 (see CodeFile.CalcGroup),
   //  this simplifies sorting by replacing the characters that enclose the
   //  file name.
   //
   word MangleInclude(string& include, string& expl) const;

   //  If CODE is an #include directive, unmanagles and returns it, else
   //  simply returns it without any changes.
   //
   string DemangleInclude(string code) const;

   //  Inserts an INCLUDE directive.
   //
   word InsertInclude(string& include, string& expl);

   //  Inserts a FORWARD declaration at ITER.
   //
   word InsertForward(const Iter& iter, const string& forward, string& expl);

   //  Inserts a FORWARD declaration at ITER.  It is the first declaration in
   //  namespace NSPACE, so it must be enclosed in a new namespace scope.
   //
   word InsertNamespaceForward(const Iter& iter,
      const string& nspace, const string& forward);

   //  Invoked after removing a forward declaration.  If the declaration was
   //  in a namespace that is now empty, erases the "namespace <name> { }".
   //
   word EraseEmptyNamespace(const Iter& iter);

   //  Returns the items within ITEM that were accessed via a using statement.
   //
   CxxNamedSet FindUsingReferents(const CxxNamed* item) const;

   //  Qualifies names used within ITEM in order to remove using statements.
   //
   void QualifyUsings(const CxxNamed* item);

   //  Within ITEM, qualifies occurrences of REF.
   //
   void QualifyReferent(const CxxNamed* item, const CxxNamed* ref);

   //  Indents the code referenced by ITER.  SPLIT is set a line break was
   //  just inserted to create a new line.
   //
   void Indent(const Iter& iter, bool split);

   //  Supplies the code for a Debug::Ft fn_name definition and invocation.
   //
   static void DebugFtCode
      (const Function* func, std::string& defn, std::string& call);

   //  Sets changed_ and returns 0.
   //
   word Changed();

   //  Sets EXPL to iter->code, sets changed_, and returns 0.
   //
   word Changed(const Iter& iter, std::string& expl);

   //  Comparison functions for sorting #include directives.
   //
   static bool IsSorted1(const SourceLine& line1, const SourceLine& line2);
   static bool IsSorted2(const string& line1, const string& line2);

   //  The file from which the source code was obtained.
   //
   const CodeFile* const file_;

   //  The stream for reading the source code.
   //
   NodeBase::istreamPtr input_;

   //  The number of lines read so far.
   //
   size_t line_;

   //  The source code.
   //
   SourceList source_;

   //  The result of loading the source code and warnings:
   //  o 1: source code and warnings have not been Read()
   //  o 0: Read() was successful
   //  o others: result of unsuccessful Read()
   //
   word read_;

   //  Set if the source code has been altered.
   //
   bool changed_;

   //  Set if the #include directives have been sorted.
   //
   bool sorted_;

   //  Set if type aliases for symbols that were resolved by a using
   //  statement have been added to each class.
   //
   bool aliased_;

   //  The file's warnings.
   //
   WarningLogVector warnings_;
};
}
#endif
