//==============================================================================
//
//  CodeWarning.h
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
#ifndef CODEWARNING_H_INCLUDED
#define CODEWARNING_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "CodeTypes.h"
#include "CxxFwd.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

using NodeBase::fixed_string;
using NodeBase::SPACE;
using NodeBase::word;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Attributes of a Warning.
//
struct WarningAttrs
{
   //  Set if the editor can fix the warning.
   //
   const bool fixable;

   //  The warning's sort order.  A warning with a lower value is fixed
   //  before one with a higher value.  This helps simplify the editor.
   //
   const uint8_t order;

   //  Set to suppress the warning.
   //
   bool suppressed;

   //  A string that explains the warning.
   //
   fixed_string expl;

   //  Constructs a warning with the specified attributes.
   //
   WarningAttrs(bool fix, uint8_t order, fixed_string expl) noexcept;
};

//------------------------------------------------------------------------------
//
//  Inserts a string for WARNING into STREAM.
//
std::ostream& operator<<(std::ostream& stream, Warning warning);

//  Whether a warning has been fixed.
//
enum WarningStatus
{
   NotSupported,  // editor does not support fixing this warning
   NotFixed,      // code not changed
   Pending,       // code changed but not written to file
   Fixed          // code changed and written to file
};

//------------------------------------------------------------------------------
//
//  Used to log a warning.
//
class CodeWarning
{
   friend class Editor;
public:
   //  Almost every member is supplied to the constructor.
   //
   CodeWarning(Warning warning, const CodeFile* file,
      size_t line, size_t pos, const CxxNamed* item,
      word offset, const std::string& info, bool hide = false);

   //  Initializes the Attrs map.
   //
   static void Initialize();

   //  Returns the explanation for warning W.
   //
   static fixed_string Expl(Warning w) { return Attrs_.at(w).expl; }

   //  Adds the log to the global set of warnings unless it is a duplicate
   //  or suppressed.
   //
   void Insert() const;

   //  Adds N to the number of line types of type T.
   //
   static void AddLineType(LineType t, size_t n) { LineTypeCounts_[t] += n; }

   //  Generates a report in STREAM for the files in SET.  The report
   //  includes line type counts and warnings found during parsing and
   //  "execution".
   //
   static void GenerateReport(std::ostream* stream, const SetOfIds& set);
private:
   //  Comparision operators.
   //
   bool operator==(const CodeWarning& that) const;
   bool operator!=(const CodeWarning& that) const;

   //  Returns true if the log should be suppressed.
   //
   bool Suppress() const;

   //  Returns true if the log has code to display.
   //
   bool HasCodeToDisplay() const
      { return ((line_ != 0) || info_.empty()); }

   //  Returns true if .info should be displayed.
   //
   bool HasInfoToDisplay() const
      { return (info_.find_first_not_of(SPACE) != std::string::npos); }

   //  Returns the logs that need to be fixed to resolve this log.
   //  The log itself is included in the result unless it does not
   //  need to be fixed.
   //
   std::vector< CodeWarning* > LogsToFix(std::string& expl);

   //  Returns the other log associated with a warning that invovles
   //  fixing both a declaration and a definition.
   //
   CodeWarning* FindMateLog(std::string& expl) const;

   //  If this log indicates that a compiler-provided special member
   //  function was invoked, it returns the log associated with the
   //  class that does not define that special member function.
   //
   CodeWarning* FindRootLog(std::string& expl);

   //  Updates WARNINGS with those that were logged in FILE.
   //
   static void GetWarnings
      (const CodeFile* file, std::vector< CodeWarning >& warnings);

   //  Returns true if LOG2 > LOG1 when sorting by file/line/reverse pos.
   //
   static bool IsSortedToFix(const CodeWarning& log1, const CodeWarning& log2);

   //  Returns LOG's index if it has already been reported, else -1.
   //
   static NodeBase::word FindWarning(const CodeWarning& log);

   //  Returns true if LOG2 > LOG1 when sorting by file/warning/line.
   //
   static bool IsSortedByFile(const CodeWarning& log1, const CodeWarning& log2);

   //  Returns true if LOG2 > LOG1 when sorting by warning/file/line.
   //
   static bool IsSortedByType(const CodeWarning& log1, const CodeWarning& log2);

   //  For inserting elements into the attributes map.
   //
   typedef std::pair< Warning, WarningAttrs > WarningPair;

   //  A type for mapping warnings to their attributes.
   //
   typedef std::map< Warning, WarningAttrs > AttrsMap;

   //  The type of warning.
   //
   Warning warning_;

   //  The file in which the warning occurred.
   //
   const CodeFile* file_;

   //  The line in FILE on which the warning occurred.
   //
   size_t line_;

   //  The position in FILE where the warning occurred.
   //
   size_t pos_;

   //  The C++ item associated with the warning.
   //
   const CxxNamed* item_;

   //  Warning specific; displayed if > 0.
   //
   word offset_;

   //  Warning specific.
   //
   std::string info_;

   //  If set, prevents a warning from being displayed.
   //
   bool hide_;

   //  Whether a warning can be, or has been, fixed by the Editor.
   //
   WarningStatus status;

   //  Maps a warning to its attributes.
   //
   static AttrsMap Attrs_;

   //  Warnings found in all files.
   //
   static std::vector< CodeWarning > Warnings_;

   //  The total number of warnings of each type, globally.
   //
   static size_t WarningCounts_[Warning_N];

   //  The number of lines of each type, globally.
   //
   static size_t LineTypeCounts_[LineType_N];
};
}
#endif
