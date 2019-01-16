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
#include "CxxFwd.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Source code editor.
//
//  A source code file must be laid out as follows:
//
//  <heading>         file name and copyright notice
//  <blank-line>
//  <#include-guard>  for a .h
//  <#include-bases>  #includes for baseIds_ and declIds_
//  <#include-exts>   #includes for other external headers
//  <#include-ints>   #includes for other internal headers
//  <blank-line>
//  <forwards>        forward declarations
//  <blank-line>
//  <usings>          using statements
//  <blank-line>
//  <rule>            single rule (or double rule)
//  <blank-line>
//  <code>            declarations and/or definitions
//
class Editor
{
public:
   //  Creates an editor for the source code in FILE, which is read from INPUT.
   //
   Editor(CodeFile* file, NodeBase::istreamPtr& input);

   //  All of the public editing functions attempt to fix the warning reported
   //  in LOG.  They return 0 on success.  Any other result indicates an error,
   //  in which case EXPL provides an explanation.  A return value of -1 means
   //  that the file should be skipped; other values denote more serious errors.
   //
   NodeBase::word Read(std::string& expl);
   NodeBase::word SortIncludes(const WarningLog& log, std::string& expl);
   NodeBase::word AddInclude(const WarningLog& log, std::string& expl);
   NodeBase::word RemoveInclude(const WarningLog& log, std::string& expl);
   NodeBase::word AddForward(const WarningLog& log, std::string& expl);
   NodeBase::word RemoveForward(const WarningLog& log, std::string& expl);
   NodeBase::word AddUsing(const WarningLog& log, std::string& expl);
   NodeBase::word RemoveUsing(const WarningLog& log, std::string& expl);
   NodeBase::word ReplaceUsing(const WarningLog& log, std::string& expl);
   NodeBase::word ResolveUsings(const WarningLog& log, std::string& expl);
   NodeBase::word InsertBlankLine(const WarningLog& log, std::string& expl);

   //  Replaces multiple blank lines with a single blank line.  Always invoked
   //  on source that was changed.
   //
   NodeBase::word EraseBlankLinePairs();

   //  Removes trailing spaces.  Always invoked on source that was changed.
   //
   NodeBase::word EraseTrailingBlanks();

   //  Converts tabs to blanks.  Always invoked on source that was changed.
   //
   NodeBase::word ConvertTabsToBlanks();

   //  Writes out the file to PATH if it was changed during editing.  Returns 0
   //  if the file had not been changed, 1 if it was successfully written, and a
   //  negative value if an error occurred.
   //
   NodeBase::word Write(const std::string& path, std::string& expl);
private:
   //  Stores a line of code.
   //
   struct SourceLine
   {
      SourceLine(const std::string& code, size_t line) :
         line(line), code(code) { }

      //  The code's line number (the first line is 0, the same as CodeWarning
      //  and Lexer.  A line added by the editor has a line number of SIZE_MAX.
      //
      const size_t line;

      //  The code.
      //
      std::string code;
   };

   //  The code is kept in a list.
   //
   typedef std::list< SourceLine > SourceList;

   //  Iterator for the source code.  This is an editor, so it doesn't bother
   //  with const iterators.
   //
   typedef std::list< SourceLine >::iterator Iter;

   //  Reads in the file's prolog (everything up to the first #include.)
   //
   NodeBase::word GetProlog(std::string& expl);

   //  Reads in the remaining #include directives.
   //
   NodeBase::word GetIncludes(std::string& expl);

   //  Reads in the rest of the file.
   //
   NodeBase::word GetEpilog();

   //  Adds a line of source code from the file.  NEVER used to add new code.
   //
   void PushBack(SourceList& list, const std::string& source);

   //  Adds an #include from the file.  NEVER used to add a new #include.
   //
   NodeBase::word PushInclude(std::string& source, std::string& expl);

   //  Returns the list that contains LINE and updates ITER to reference
   //  that line.  Returns nullptr if LINE is not found.
   //
   SourceList* Find(size_t line, Iter& iter);

   //  Returns the location of LINE within LIST.  Returns the end of LIST
   //  if LINE is not found.
   //
   static Iter Find(SourceList& list, size_t line);

   //  Returns the location of SOURCE within LIST.  Returns the end of LIST
   //  if SOURCE is not found.  SOURCE must match an entire line of code.
   //
   static Iter Find(SourceList& list, const std::string& source);

   //  Inserts SOURCE into LIST at ITER.  Its new location is returned.
   //
   Iter Insert(SourceList& list, Iter& iter, const std::string& source);

   //  If LIST contains LINE from the original source, deletes that line and
   //  returns the line that followed it.  Returns the end of LIST if LINE
   //  was not found.
   //
   Iter Erase(SourceList& list, size_t line, std::string& expl);

   //  If LIST contains a line of code that matches SOURCE, deletes that line
   //  and returns the line that followed it.  Returns the end of LIST if LINE
   //  was not found.
   //
   Iter Erase(SourceList& list, const std::string& source, std::string& expl);

   //  If INCLUDE specifies a file in groups 1 to 4 (see CodeFile.CalcGroup),
   //  this simplifies sorting by replacing the characters that enclose the
   //  file name.
   //
   NodeBase::word MangleInclude(std::string& include, std::string& expl) const;

   //  Inserts an INCLUDE directive.
   //
   NodeBase::word InsertInclude(std::string& include, std::string& expl);

   //  Inserts a FORWARD declaration at ITER.
   //
   NodeBase::word InsertForward
      (const Iter& iter, const std::string& forward, std::string& expl);

   //  Inserts a FORWARD declaration at ITER.  It is the first declaration in
   //  namespace NSPACE, so it must be enclosed in a new namespace scope.
   //
   NodeBase::word InsertNamespaceForward(Iter& iter,
      const std::string& nspace, const std::string& forward);

   //  Invoked after removing a forward declaration.  If the declaration was
   //  in a namespace that is now empty, erases the "namespace <name> { }".
   //
   NodeBase::word EraseEmptyNamespace(const Iter& iter);

   //  Qualifies names used within ITEM in order to remove using statements.
   //
   void QualifyUsings(const CxxNamed* item);

   //  Returns the items within ITEM that were accessed via a using statement.
   //
   CxxNamedSet FindUsingReferents(const CxxNamed* item) const;

   //  Within ITEM, qualifies occurrences of REF.
   //
   void QualifyReferent(const CxxNamed* item, const CxxNamed* ref);

   //  Removes trailing spaces within code in LIST.
   //
   void EraseTrailingBlanks(SourceList& list);

   //  Converts tabs to blanks within code in LIST.
   //
   void ConvertTabsToBlanks(SourceList& list);

   //  Invoked to report TEXT, which is assigned to EXPL.  Returns RC.
   //
   static NodeBase::word Report
      (std::string& expl, NodeBase::fixed_string text, NodeBase::word rc = 0);

   //  Comparison functions for sorting #include directives.
   //
   static bool IsSorted1(const SourceLine& line1, const SourceLine& line2);
   static bool IsSorted2(const std::string& line1, const std::string& line2);

   //  The file from which the source code was obtained.
   //
   CodeFile* file_;

   //  The stream for reading the source code.
   //
   NodeBase::istreamPtr input_;

   //  The number of lines read so far.
   //
   size_t line_;

   //  The lines of source that precede the first #include directive.
   //
   SourceList prolog_;

   //  The #include directives.
   //
   SourceList includes_;

   //  The rest of the source code.
   //
   SourceList epilog_;

   //  Set if the source code has been altered.
   //
   bool changed_;

   //  Set if type aliases for symbols that were resolved by a using
   //  statement have been added to each class.
   //
   bool aliased_;

   //  Characters that enclose the file name in an #include directive,
   //  depending on the group to which it belongs.
   //
   static const std::string FrontChars;
   static const std::string BackChars;
};
}
#endif
